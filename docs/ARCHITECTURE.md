# Architecture — Ultimate HAI

Application Qt 6 / QML Harness-only pour Omarchy.

## Processus

- **Backend** : configuration, thème, Git, diagnostics, workspace, cycle de vie
  du harness.
- **HarnessServer** : `dsh web --no-open --port 0`, URL tokenisée dans WebEngine.
  Overlays chargés : `connections.patch.yml`, `mcp-memory.patch.yml`,
  `mcp-servers.patch.yml`.
- **TerminalSession** : PTY forkpty + xterm.js, chargé à la première ouverture.
- **HarnessConnections** : providers `llm-pi-ai`, credentials DSH,
  `agent.env` mode 600 injecté dans `dsh web` et le shell.
- **McpManager** : serveurs MCP stdio/HTTP, config JSON privée compilée en
  `mcp-servers.patch.yml`.
- **MemoryStore** : SQLite WAL + FTS5, sauvegarde et intégrité.
- **AgentManager** : rôles intégrés et profils personnalisés.
- **MissionRunner** : Plan → Exécution → Revue via `dsh headless`, permissions
  read-only/workspace-write, sorties persistées.
- **Autopilot** : file de tâches, reprise, mémoire, timeout.

## Vues

Cockpit, Écosystème, Fichiers, Mémoire, Autopilote, Harness, Agents, Terminal
agent, Connexions, MCP, Installation, Réglages.

## Données

- Config : `~/.config/Crasy/deepcrasy-h/config.json` (chemin historique stable).
- Overlays : même dossier de configuration.
- Données : `~/.local/share/Crasy/deepcrasy-h/` (memory.db, autopilot.db,
  missions.db, integrations).
- Credentials : `~/.dsh/.credentials.yaml` (mode 600).
- `agent.env` : miroir privé, mode 600.

Le nom visible, le binaire, le paquet et le `.desktop` sont **Ultimate HAI**.
Les chemins historiques sont conservés volontairement pour la migration des
données. Les anciens fichiers d'intégration retirés sont supprimés.

## Règles

- Une instance par utilisateur (`ultimate-hai-uid`).
- WebEngine des vues masquées déchargé.
- Aucune exécution arbitraire par shell : `dsh headless` et serveurs MCP
  explicitement configurés.
- Planificateur/réviseur read-only ; exécutant workspace-write.
- `pkexec` pour toute action administrateur ; nettoyage et vérification après
  installation.
