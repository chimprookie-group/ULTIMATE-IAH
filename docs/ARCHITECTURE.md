# Architecture — Ultimate IAH

Application Qt 6 / QML native agentique pour Omarchy.

## Processus

- **Backend** : configuration, thème, Git, diagnostics et workspace.
- **Agent runtime** : Ultimate Agent est le moteur natif ; aucun serveur web DSH
  n'est lancé. Les jobs utilisent le même binaire en mode `--agent-job`.
- **TerminalSession** : PTY forkpty + xterm.js, chargé à la première ouverture.
- **HarnessConnections** : providers, credentials locaux et `agent.env` mode
  600 injecté dans Ultimate Agent et le shell. Le chemin historique du store
  de credentials est conservé pour la migration.
- **McpManager** : serveurs MCP stdio/HTTP, config JSON privée compilée en
  `mcp-servers.patch.yml`.
- **MemoryStore** : SQLite WAL + FTS5, sauvegarde et intégrité.
- **BotStore / BotRunner / BotManager / BotServer** : registre SQLite des bots,
  exécution par job agent natif, file d’attente, planificateur, API loopback à jeton
  et pont MCP `ultimate-iah-bots`.
- **AgentManager** : rôles intégrés et profils personnalisés.
- **MissionRunner** : Plan → Exécution → Revue via le job agent natif, permissions
  read-only/workspace-write, sorties persistées.
- **Autopilot** : file de tâches, reprise, mémoire, timeout.
- **PluginManager** : registre natif des outils, détection des 14 CLI ArtCraft,
  activation persistante, manifestes externes et outils dynamiques du harness.
- **NoteStore / NoteGraph** : vault Markdown, index SQLite, wikilinks,
  backlinks et graphe force-directed.
- **GitHubService** : connexion gh, dépôts, création/clonage, issues et PR,
  outils agent et vue GitHub.
- **WebService** : web_search native (DeepSeek Search, secours DuckDuckGo),
  web_fetch borné, cache et protection des redirections cross-origin.

## Vues

Cockpit, Ultimate Agent, Notes, Écosystème, Fichiers, Mémoire, Autopilote,
Agents, Terminal agent, Bots, Providers, Plugins, MCP, Installation, Réglages.

## Données

- Config : `~/.config/Crasy/deepcrasy-h/config.json` (chemin historique stable).
- Overlays : même dossier de configuration.
- Données : `~/.local/share/Crasy/deepcrasy-h/` (memory.db, autopilot.db,
  missions.db, bots.db, bots_mcp.py, agent-conversations.db,
  agent-attachments/, vault/, integrations).
- Aucun serveur web DSH n'est lancé. Les jobs agent utilisent le même binaire
  en mode `--agent-job`.
- API bots : `127.0.0.1:8790`, jeton `bot.token` mode 600, dashboard local.
- Credentials : store local historique (mode 600).
- `agent.env` : miroir privé, mode 600.

Le nom visible, le binaire, le paquet et le `.desktop` sont **Ultimate IAH**.
Les chemins historiques sont conservés volontairement pour la migration des
données. Les anciens fichiers d'intégration retirés sont supprimés.

## Règles

- Une instance par utilisateur (`ultimate-iah-uid`).
- Le terminal WebEngine n'est chargé qu'à l'ouverture.
- Aucune exécution arbitraire par shell : le job agent natif et les serveurs MCP
  explicitement configurés.
- Planificateur/réviseur read-only ; exécutant workspace-write.
- `pkexec` pour toute action administrateur ; nettoyage et vérification après
  installation.
