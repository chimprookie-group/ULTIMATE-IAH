# Architecture — Crasy Harness

Crasy Harness est une application Qt 6 / QML, Harness-only, pour Omarchy.

## Processus

- **Backend** (`src/backend.*`) possède la configuration, le thème, Git, les
  diagnostics, le workspace et le cycle de vie du harness.
- **HarnessServer** lance `dsh web --no-open --port 0`, lit l'URL tokenisée et
  l'expose au WebEngine. Il peut recevoir plusieurs overlays `--patch` :
  `nine-router.patch.yml`, `openmausbot.patch.yml`, `connections.patch.yml`.
- **TerminalSession** est le PTY (forkpty + xterm.js). Le terminal est chargé à
  la première ouverture seulement.
- **NineRouterManager** et **MausBotManager** gèrent leurs services locaux et
  leurs ponts harness. Aucun moteur OpenCode n'est construit ni lancé.
- **HarnessConnections** écrit les providers `llm-pi-ai` du harness, les clés
  dans le credential store DSH et le modèle par défaut, via un helper Python
  atomique. Il maintient aussi `agent.env` (mode 600), miroir des credentials :
  `HarnessServer` l'injecte dans `dsh web`, et le terminal agent le source avant
  `dsh headless`, car le profil headless ne résout pas les `refs` DSH.

## Vues

- Cockpit : tâche harness, projet, services, terminal agent.
- Harness : WebEngine `dsh web`, chargé à la première ouverture et déchargé quand la vue est masquée.
- Terminal agent : PTY + barre `dsh headless "<tâche>"`.
- Projet / Fichiers : outils, skills, Git, diff, contexte.
- Connexions : DeepSeek, providers OpenAI-compatible, modèle par défaut,
  9Router, MausBot et MCP.
- Réglages : thème, langue, démarrage, workspace, binaire/profil harness.

## Données

- Config : `~/.config/Crasy/deepcrasy-h/config.json`.
- Overlay providers : `~/.config/Crasy/deepcrasy-h/connections.patch.yml`.
- Credentials : `~/.dsh/.credentials.yaml` (mode 600, géré par DSH).
- Profil harness : `~/.dsh/profiles/<profile>/cordis.patch.yml`.
- 9Router : `~/.local/share/Crasy/deepcrasy-h/9router` + `~/.9router`.
- MausBot : `~/.local/share/Crasy/deepcrasy-h/openmausbot-*`.

## Règles

- Une seule instance par utilisateur (`QLocalServer`).
- Vues et profils WebEngine créés à la première ouverture, jamais au démarrage.
- WebEngine des vues masquées : URL vidée.
- Cockpit : uniquement Git et sessions locales, pas de diagnostics/MCP.
- Aucun OpenCode, ACP, chat, session ou permission OpenCode.
- Installation vérifiée et nettoyée ; `pkexec` pour toute action privilégiée.
