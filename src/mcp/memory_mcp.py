#!/usr/bin/env python3
"""Ultimate HAI memory MCP server.

Newline-delimited JSON-RPC over stdio. The database is the same SQLite/FTS5
store used by the desktop application, so memories written by the agent are
immediately visible in the UI and vice versa.
"""
import datetime
import json
import os
import sqlite3
import sys

DB = os.environ.get("ULTIMATE_HAI_MEMORY_DB", "")
DEFAULT_PROJECT = os.environ.get("ULTIMATE_HAI_MEMORY_PROJECT") or os.getcwd()
SERVER = {"name": "ultimate-hai-memory", "version": "1.0.0"}

TOOLS = [
    {
        "name": "memory_search",
        "description": "Search the persistent project memory (facts, decisions, preferences, prior task results). Use before acting on a project so you do not repeat old mistakes.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Keywords or a natural-language phrase."},
                "project": {"type": "string", "description": "Project path; defaults to the active workspace."},
                "limit": {"type": "integer", "minimum": 1, "maximum": 50, "default": 8},
            },
            "required": ["query"],
        },
    },
    {
        "name": "memory_remember",
        "description": "Persist a durable fact, decision, preference or result in the project memory. Store conclusions, not raw logs.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "title": {"type": "string", "description": "Short searchable title."},
                "body": {"type": "string", "description": "Compact content to remember."},
                "kind": {"type": "string", "enum": ["note", "fact", "decision", "preference", "task", "result"], "default": "note"},
                "project": {"type": "string", "description": "Project path; defaults to the active workspace."},
                "tags": {"type": "string", "description": "Comma-separated tags."},
                "pinned": {"type": "boolean", "default": False},
                "importance": {"type": "integer", "minimum": 0, "maximum": 5, "default": 3},
            },
            "required": ["title", "body"],
        },
    },
    {
        "name": "memory_recent",
        "description": "List the most recent memories for a project, including pinned decisions.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "project": {"type": "string", "description": "Project path; defaults to the active workspace."},
                "limit": {"type": "integer", "minimum": 1, "maximum": 50, "default": 10},
            },
        },
    },
]


def emit(message):
    sys.stdout.write(json.dumps(message, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def result(request_id, value):
    emit({"jsonrpc": "2.0", "id": request_id, "result": value})


def error(request_id, code, message):
    emit({"jsonrpc": "2.0", "id": request_id, "error": {"code": code, "message": message}})


def db():
    if not DB or not os.path.exists(DB):
        raise RuntimeError("memory database not found")
    connection = sqlite3.connect(DB, timeout=5)
    connection.row_factory = sqlite3.Row
    return connection


def text(value):
    return {"content": [{"type": "text", "text": value}], "isError": False}


def format_rows(rows):
    if not rows:
        return "Aucune mémoire correspondante."
    lines = []
    for row in rows:
        body = " ".join((row["body"] or "").split())
        if len(body) > 500:
            body = body[:500] + "…"
        marker = " [épinglée]" if row["pinned"] else ""
        lines.append(
            f"- #{row['id']} [{row['kind']}]{marker} {row['title']} : {body}"
            + (f" (tags: {row['tags']})" if row["tags"] else "")
        )
    return "\n".join(lines)


def query_tokens(query):
    words = []
    for word in query.replace('"', " ").replace("'", " ").split():
        word = "".join(ch for ch in word if ch.isalnum() or ch in "_-")
        if word:
            words.append('"' + word + '"*')
    return " AND ".join(words)


def memory_search(arguments):
    query = str(arguments.get("query", "")).strip()
    project = str(arguments.get("project", "")).strip() or DEFAULT_PROJECT
    limit = max(1, min(50, int(arguments.get("limit", 8))))
    connection = db()
    rows = []
    match = query_tokens(query)
    if match:
        try:
            rows = connection.execute(
                "SELECT m.* FROM memories_fts f JOIN memories m ON m.id=f.rowid "
                "WHERE memories_fts MATCH ? AND (? = '' OR m.project = ?) "
                "ORDER BY m.pinned DESC, bm25(memories_fts), m.updated_at DESC LIMIT ?",
                (match, project, project, limit),
            ).fetchall()
        except sqlite3.Error:
            rows = []
    if not rows:
        like = "%" + query + "%"
        rows = connection.execute(
            "SELECT * FROM memories WHERE (title LIKE ? OR body LIKE ? OR tags LIKE ?) "
            "AND (? = '' OR project = ?) ORDER BY pinned DESC, updated_at DESC LIMIT ?",
            (like, like, like, project, project, limit),
        ).fetchall()
    connection.close()
    return format_rows(rows)


def memory_remember(arguments):
    title = str(arguments.get("title", "")).strip()
    body = str(arguments.get("body", "")).strip()
    if not title:
        raise RuntimeError("title is required")
    project = str(arguments.get("project", "")).strip() or DEFAULT_PROJECT
    kind = str(arguments.get("kind", "note")).strip() or "note"
    tags = arguments.get("tags", "")
    if isinstance(tags, list):
        tags = ",".join(str(tag) for tag in tags)
    tags = str(tags).strip()
    pinned = 1 if arguments.get("pinned", False) else 0
    importance = max(0, min(5, int(arguments.get("importance", 3))))
    stamp = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="milliseconds")
    connection = db()
    cursor = connection.execute(
        "INSERT INTO memories(project,kind,title,body,tags,pinned,importance,source,created_at,updated_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?)",
        (project, kind, title, body, tags, pinned, importance, "agent", stamp, stamp),
    )
    connection.commit()
    identifier = cursor.lastrowid
    connection.close()
    return f"Mémoire #{identifier} enregistrée : {title}"


def memory_recent(arguments):
    project = str(arguments.get("project", "")).strip() or DEFAULT_PROJECT
    limit = max(1, min(50, int(arguments.get("limit", 10))))
    connection = db()
    rows = connection.execute(
        "SELECT * FROM memories WHERE (? = '' OR project = ?) "
        "ORDER BY pinned DESC, updated_at DESC LIMIT ?",
        (project, project, limit),
    ).fetchall()
    connection.close()
    return format_rows(rows)


def call_tool(name, arguments):
    if name == "memory_search":
        return memory_search(arguments)
    if name == "memory_remember":
        return memory_remember(arguments)
    if name == "memory_recent":
        return memory_recent(arguments)
    raise RuntimeError(f"unknown tool: {name}")


def handle(message):
    method = message.get("method", "")
    request_id = message.get("id")
    params = message.get("params") or {}
    if method == "initialize":
        version = params.get("protocolVersion") or "2024-11-05"
        result(request_id, {
            "protocolVersion": version,
            "capabilities": {"tools": {}},
            "serverInfo": SERVER,
        })
    elif method == "notifications/initialized":
        return
    elif method == "ping":
        result(request_id, {})
    elif method == "tools/list":
        result(request_id, {"tools": TOOLS})
    elif method == "tools/call":
        name = params.get("name", "")
        try:
            value = call_tool(name, params.get("arguments") or {})
            result(request_id, text(value))
        except Exception as exc:  # keep the server alive and explain the failure
            result(request_id, {"content": [{"type": "text", "text": str(exc)}], "isError": True})
    elif method == "resources/list":
        result(request_id, {"resources": []})
    elif method == "prompts/list":
        result(request_id, {"prompts": []})
    elif request_id is not None:
        error(request_id, -32601, f"method not found: {method}")


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            handle(json.loads(line))
        except Exception as exc:
            print(f"ultimate-hai-memory: {exc}", file=sys.stderr, flush=True)


if __name__ == "__main__":
    main()
