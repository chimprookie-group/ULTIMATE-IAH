#!/usr/bin/env python3
"""Ultimate IAH bots MCP bridge.

Newline-delimited JSON-RPC over stdio. The bridge talks to the loopback bot API
with the per-install token supplied by BotManager, so DSH agents can list bots
and delegate work without a second execution engine.
"""
import json
import os
import sys
import urllib.error
import urllib.request

BASE = os.environ.get("ULTIMATE_IAH_BOTS_URL", "http://127.0.0.1:8790").rstrip("/")
TOKEN = os.environ.get("ULTIMATE_IAH_BOTS_TOKEN", "")
SERVER = {"name": "ultimate-iah-bots", "version": "1.0.0"}

TOOLS = [
    {
        "name": "bots_list",
        "description": "List the bots available in Ultimate IAH, their role, permission and status.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "bots_messages",
        "description": "Read recent messages for one Ultimate IAH bot.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "bot": {"type": "string", "description": "Bot id or name."},
                "limit": {"type": "integer", "minimum": 1, "maximum": 80, "default": 30},
            },
            "required": ["bot"],
        },
    },
    {
        "name": "bots_send",
        "description": "Delegate a task to an Ultimate IAH bot. Returns a run id that can be polled with bots_run_status.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "bot": {"type": "string", "description": "Bot id or name."},
                "text": {"type": "string", "description": "Task or message for the bot."},
            },
            "required": ["bot", "text"],
        },
    },
    {
        "name": "bots_run_status",
        "description": "Read the status and output of an Ultimate IAH bot run.",
        "inputSchema": {
            "type": "object",
            "properties": {"run_id": {"type": "integer", "description": "Run id returned by bots_send."}},
            "required": ["run_id"],
        },
    },
]


def emit(message):
    sys.stdout.write(json.dumps(message, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def respond(request_id, value):
    emit({"jsonrpc": "2.0", "id": request_id, "result": value})


def fail(request_id, code, message):
    emit({"jsonrpc": "2.0", "id": request_id, "error": {"code": code, "message": message}})


def text_result(value, error=False):
    return {"content": [{"type": "text", "text": value}], "isError": error}


def request(path, method="GET", body=None):
    data = None if body is None else json.dumps(body).encode("utf-8")
    req = urllib.request.Request(BASE + path, data=data, method=method)
    req.add_header("Authorization", "Bearer " + TOKEN)
    req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=20) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        try:
            payload = json.loads(error.read().decode("utf-8"))
            raise RuntimeError(payload.get("error", "HTTP " + str(error.code)))
        except ValueError:
            raise RuntimeError("HTTP " + str(error.code))
    except Exception as error:
        raise RuntimeError("bot API unavailable: " + str(error))


def resolve_bot(value):
    bots = request("/api/bots")
    key = str(value).strip()
    for bot in bots:
        if str(bot.get("id")) == key or bot.get("name", "").lower() == key.lower():
            return bot
    raise RuntimeError("unknown bot: " + key)


def handle(name, arguments):
    if name == "bots_list":
        bots = request("/api/bots")
        lines = []
        for bot in bots:
            lines.append(
                "- #%s %s [%s] %s — %s"
                % (bot.get("id"), bot.get("name"), bot.get("permission"),
                   bot.get("description", ""),
                   "actif" if bot.get("enabled") else "désactivé")
            )
        return text_result("\n".join(lines) or "Aucun bot.")
    if name == "bots_messages":
        bot = resolve_bot(arguments.get("bot"))
        limit = max(1, min(80, int(arguments.get("limit", 30))))
        messages = request("/api/bots/%s/messages" % bot["id"])
        lines = []
        for message in messages[-limit:]:
            lines.append("[%s] %s: %s" % (message.get("role"), message.get("time"), message.get("content")))
        return text_result("\n".join(lines) or "Aucun message.")
    if name == "bots_send":
        bot = resolve_bot(arguments.get("bot"))
        task = str(arguments.get("text", "")).strip()
        if not task:
            raise RuntimeError("text is required")
        result = request("/api/bots/%s/messages" % bot["id"], "POST", {"text": task})
        return text_result("Run #%s queued for %s. Poll with bots_run_status." % (result.get("runId"), bot.get("name")))
    if name == "bots_run_status":
        run = request("/api/runs/%s" % int(arguments.get("run_id")))
        output = (run.get("output") or "")[-12000:]
        return text_result(
            "run #%s · %s\nbot: %s\ntask: %s\n%s"
            % (run.get("id"), run.get("status"), run.get("botName"), run.get("task"), output)
        )
    raise RuntimeError("unknown tool: " + name)


for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    try:
        message = json.loads(line)
        method = message.get("method", "")
        request_id = message.get("id")
        if method == "initialize":
            respond(request_id, {
                "protocolVersion": message.get("params", {}).get("protocolVersion", "2024-11-05"),
                "capabilities": {"tools": {}},
                "serverInfo": SERVER,
            })
        elif method == "notifications/initialized":
            continue
        elif method == "ping":
            respond(request_id, {})
        elif method == "tools/list":
            respond(request_id, {"tools": TOOLS})
        elif method == "tools/call":
            params = message.get("params", {})
            try:
                respond(request_id, handle(params.get("name", ""), params.get("arguments") or {}))
            except Exception as error:
                respond(request_id, text_result(str(error), True))
        else:
            fail(request_id, -32601, "method not found")
    except Exception as error:
        emit({"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": str(error)}})
