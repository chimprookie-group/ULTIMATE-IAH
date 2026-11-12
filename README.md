# Ultimate IAH

**Le cockpit agentique du DeepSeek Harness pour Omarchy.**

Ultimate IAH réunit le harness, des agents orchestrés, une mémoire persistante,
un terminal réel, un autopilote borné et un centre de connexions providers/MCP
— sans OpenCode.

![Cockpit Ultimate IAH](assets/screenshot-home.png)

## Identité

La baleine musclée est le nouveau logo d'Ultimate IAH. Le fond clair a été
détouré avec précision : variante blanche sur les thèmes sombres, variante encre
sur les thèmes clairs. Dans le rail, le logo est posé directement, sans tuile
autour, pour lui laisser de l'air. Il structure aussi les en-têtes de page, les
états vides d'Ultimate Agent et de la mémoire, l'écran Harness, la palette,
l'aide et le À propos. L'icône du lanceur utilise une tuile encre arrondie avec
la baleine blanche et un discret trait d'accent.

## Agentique

![Agents et missions](assets/screenshot-agents.png)

- **Profils d'agents** : planificateur, exécutant, réviseur, chercheur, et rôles
  personnalisés.
- **Missions Plan → Exécution → Revue** : trois appels `dsh headless` successifs,
  avec mémoire et résultats intermédiaires transmis.
- **Skills gérés** : bibliothèque utilisateur et projet, création et suppression
  depuis l'interface, réutilisation dans une mission.
- **Permissions adaptées** : planificateur et réviseur en lecture seule ;
  exécutant en `workspace-write` ; approbation `never` pour le mode headless.
- **Suivi persistant** : chaque mission et chaque étape est stockée dans SQLite,
  avec reprise, annulation et sorties consultables.

## Installation complète

La vue **Installation** (`Ctrl+Alt+I`) audite et installe l'écosystème complet :
runtime DSH, Node, npm, Python, PyYAML, QtWebEngine, Git, portail fichiers,
MCP mémoire, service de bots et entrées de bureau. Le runtime DSH système
(`/usr/lib/crasy/dsh`) est utilisé en priorité ; sinon DSH peut être installé
localement sans droits administrateur.

## Ultimate Agent

La politique **one-shot** est embarquée dans le paquet et dans la mémoire
d'origine de l'agent : reproduire, comprendre la cause partagée, corriger une
fois, exécuter la vérification réelle et rapporter les preuves.

![Ultimate Agent](assets/screenshot-agent.png)

**Ctrl+Alt+U** ouvre le dock agent, et l'entrée **Ultimate Agent** de la barre
latérale ouvre la vue complète. Il connaît l'état réel d'Ultimate IAH et agit via
des outils : diagnostic, providers, DSH, MCP, mémoire, skills, missions,
terminal, fichiers, Git, installation et réglages.

Voir [docs/AGENT.md](docs/AGENT.md) pour les outils et les règles de sécurité.

![Ultimate Agent en dock](assets/screenshot-agent-dock.png)

## Raccourci DeepSeek Harness

Le paquet installe aussi un lanceur **DeepSeek Harness** qui ouvre directement la
vue harness d'Ultimate IAH, avec le nouveau logo. L'ancien lanceur Crasy reste
masqué pour éviter l'ancienne icône.

## Bots

![Bots Ultimate IAH](assets/screenshot-bots.png)

Ultimate IAH inclut un service de bots natif, persistant et sécurisé :

- trois bots fournis : **Atlas** (coordination), **Forge** (construction) et **Sentry** (audit) ;
- création, édition, activation et suppression ;
- permissions **lecture seule** ou **écriture workspace** ;
- chat et délégation via `dsh headless` ;
- file d’exécution, annulation et timeout ;
- planifications récurrentes ;
- API HTTP locale sur `127.0.0.1:8790`, protégée par un jeton privé ;
- dashboard web local ;
- pont MCP `ultimate-iah-bots` pour les agents DSH ;
- mémoire, runs et conversations dans SQLite.

Le bouton **Dashboard** ouvre l’interface web. Le jeton se copie depuis la vue Bots. Les bots sont accessibles par **Ctrl+Alt+B**.

## Providers et MCP

![Providers](assets/screenshot-connections.png)

![Serveurs MCP](assets/screenshot-mcp.png)

- **Hub providers centralisé** : catalogue de 46 presets classés en Directs,
  Agrégateurs, Asie, Entreprise et Locaux.
- **Application fiable** : chaque preset ouvre un formulaire pré-rempli ;
  URL, modèles, protocole et clé sont vérifiés avant l'enregistrement.
- **Protocoles** : OpenAI-compatible et Anthropic Messages.
- **Provider et modèle par défaut** : sélection en un clic, synchronisée avec
  le harness DSH.
- **Clé DeepSeek unique** : une saisie dans Providers crée le provider
  `deepseek-official`, alimente DSH, `agent.env` et le terminal, puis définit
  `deepseek-chat` par défaut si aucun modèle ne l'est déjà.
- **Édition et suppression** des providers, diagnostic et redémarrage DSH.
- **MCP stdio et HTTP** : ajout, activation, suppression, presets Filesystem /
  Fetch / Git, patch automatique pour le harness.
- **MCP intégré** : mémoire Ultimate IAH.
- **Liens directs** vers MCP, mémoire, bots, Ultimate Agent et installation.

## Mémoire

![Mémoire persistante](assets/screenshot-memory.png)

- SQLite WAL + FTS5, par projet : faits, décisions, préférences, résultats.
- Recherche plein texte, épinglage, intégrité, sauvegarde.
- Outils MCP : `memory_search`, `memory_remember`, `memory_recent`.
- Mémoire injectée dans les missions, l'autopilote et les tâches du terminal.

## Autopilote

![Autopilote](assets/screenshot-autopilot.png)

- File de tâches, exécution une à une via `dsh headless`, intervalle ou manuel.
- Timeout, reprise, écriture des résultats en mémoire.
- Autonomie opt-in : elle tourne tant que l'application est ouverte.

## Terminal et harness

- Web UI DeepSeek Harness embarqué, profil et workspace partagés.
- Terminal PTY réel (bash, zsh, fish) avec barre de tâche harness.
- Projet, Git, fichiers, diff, outils et skills.

## Raccourcis

| Touche | Action |
|---|---|
| `Ctrl+0` | Cockpit |
| `Ctrl+1` | Harness |
| `Ctrl+2` | Terminal agent |
| `Ctrl+4` | Réglages |
| `Ctrl+6` | Écosystème |
| `Ctrl+7` | Fichiers |
| `Ctrl+8` | Providers |
| `Ctrl+Alt+U` | Ultimate Agent |
| `Ctrl+Alt+B` | Bots |
| `Ctrl+Alt+A` | Autopilote |
| `Ctrl+Alt+C` | MCP |
| `Ctrl+Alt+G` | Agents et missions |
| `Ctrl+Alt+M` | Mémoire |
| `Ctrl+K` | Palette de commandes |
| `?` | Aide |

## Installer

```sh
./bin/build
./bin/test
cd pkgbuild && makepkg -f
sudo pacman -U ultimate-hai-1.7.2-1-x86_64.pkg.tar.zst
```

Le paquet remplace `deepcrasy-h`. Les données existantes sont conservées dans
les chemins historiques (`~/.config/Crasy/deepcrasy-h` et
`~/.local/share/Crasy/deepcrasy-h`) : mémoire, providers, MCP, missions et
autopilote restent en place. Les anciens composants retirés sont nettoyés.

## Limites

- L'autopilote est **borné** : tâches créées par l'utilisateur, une à la fois,
  opt-in, et l'application doit rester ouverte.
- La mémoire est **lexicale FTS5**, sans embeddings ni synchronisation cloud.
- MCP exécute des serveurs avec les permissions de votre session : n'activez
  que des commandes contrôlées.

## Licence

MIT.
