# Ultimate IAH

**Le cockpit agentique natif d’Ultimate IAH pour Omarchy.**

Ultimate IAH réunit Ultimate Agent, des discussions persistantes, un vault
Markdown relié, des agents orchestrés, une mémoire durable, un terminal réel,
des bots natifs et un hub providers/MCP — sans harness web externe.

![Cockpit Ultimate IAH](assets/screenshot-home.png)

## Identité

La baleine musclée est le nouveau logo d'Ultimate IAH. Le fond clair a été
détouré avec précision : variante blanche sur les thèmes sombres, variante encre
sur les thèmes clairs. Dans le rail, le logo est posé directement, sans tuile
autour, pour lui laisser de l'air. Il structure aussi les en-têtes de page, les
états vides d'Ultimate Agent et de la mémoire, l'écran Ultimate Agent, la palette,
l'aide et le À propos. L'icône du lanceur utilise une tuile encre arrondie avec
la baleine blanche et un discret trait d'accent.

## Agentique

![Agents et missions](assets/screenshot-agents.png)

- **Profils d'agents** : planificateur, exécutant, réviseur, chercheur, et rôles
  personnalisés.
- **Missions Plan → Exécution → Revue** : trois passes du moteur agent natif,
  avec mémoire et résultats intermédiaires transmis.
- **Skills gérés** : bibliothèque utilisateur et projet, création et suppression
  depuis l'interface, réutilisation dans une mission.
- **Permissions adaptées** : planificateur et réviseur en lecture seule ;
  exécutant en `workspace-write`, appliqué par le moteur agent.
- **Suivi persistant** : chaque mission et chaque étape est stockée dans SQLite,
  avec reprise, annulation et sorties consultables.

## Installation complète

La vue **Installation** (`Ctrl+Alt+I`) audite et installe l'écosystème complet :
moteur agent natif, Node, npm, Python, PyYAML, QtWebEngine, Git, portail
fichiers, MCP mémoire, service de bots, vault Notes et entrées de bureau.

## Ultimate Agent

La politique **one-shot** est embarquée dans le paquet et dans la mémoire
d'origine de l'agent : reproduire, comprendre la cause partagée, corriger une
fois, exécuter la vérification réelle et rapporter les preuves.

![Ultimate Agent](assets/screenshot-agent.png)

**Ctrl+Alt+U** ouvre le dock agent, et l'entrée **Ultimate Agent** de la barre
latérale ouvre la vue complète. Il connaît l'état réel d'Ultimate IAH et agit via
des outils : diagnostic, providers, MCP, mémoire, skills, notes, missions,
terminal, fichiers, Git, installation et réglages.

- **Discussions persistantes** : historique SQLite, rail de discussions,
  recherche, nouvel échange, renommage, épinglage, suppression confirmée et
  export du transcript.
- **Images dans le chat** : pièce jointe via le trombone ou glisser-déposer,
  aperçu intégré, agrandissement et transmission au modèle compatible vision.
- **Modes de travail** : Assistant, Plan, Diagnostic, Construction et Revue.
- **Contexte** : jauge de contexte et compaction native.
- **Compétences** : activation des skills installés, injectés dans le prompt.
- **Options de conversation** : copie, vidage et suppression depuis le panneau
  Options.
- Les discussions et pièces jointes restent locales :
  `agent-conversations.db` et `agent-attachments/`.

Voir [docs/AGENT.md](docs/AGENT.md) pour les outils et les règles de sécurité.

![Ultimate Agent en dock](assets/screenshot-agent-dock.png)

## Bots

![Bots Ultimate IAH](assets/screenshot-bots.png)

Ultimate IAH inclut un service de bots natif, persistant et sécurisé :

- trois bots fournis : **Atlas** (coordination), **Forge** (construction) et **Sentry** (audit) ;
- création, édition, activation et suppression ;
- permissions **lecture seule** ou **écriture workspace** ;
- exécution par le moteur agent natif ;
- file d’exécution, annulation et timeout ;
- planifications récurrentes ;
- API HTTP locale sur `127.0.0.1:8790`, protégée par un jeton privé ;
- dashboard web local ;
- pont MCP `ultimate-iah-bots` pour Ultimate Agent ;
- mémoire, runs et conversations dans SQLite.

Le bouton **Dashboard** ouvre l’interface web. Le jeton se copie depuis la vue Bots. Les bots sont accessibles par **Ctrl+Alt+B**.

## Recherche web native

Ultimate Agent expose deux outils web réels :

- **web_search** : DeepSeek Search quand la clé est disponible, sinon secours
  DuckDuckGo ; une à quatre requêtes, huit sources maximum, snippets et URLs.
- **web_fetch** : récupération HTTP(S), redirections limitées à la même origine,
  texte extrait, taille plafonnée à 200 000 caractères, cache de dix minutes.
- Les résultats sont marqués comme contenu externe non fiable.
- Outil de diagnostic `ultimate_web` et cache visible dans Plugins.

## Notes

![Notes Ultimate IAH](assets/screenshot-notes.png)

- Vault Markdown local, fichiers plats et index SQLite.
- Arborescence, recherche, tags, épinglage, renommage et corbeille.
- **Wikilinks `[[Titre]]`**, backlinks et graphe interactif.
- Note quotidienne, autosauvegarde et aperçu Markdown.
- Pièces jointes image copiées dans le vault.
- Outils agent `ultimate_notes` pour lire, chercher, créer et mettre à jour.
- Raccourci **Ctrl+Alt+N**.

## Socle DSH natif

Le panneau Plugins expose la carte complète du harness :

- **29 capacités du noyau** actives par défaut : agent loop, sessions, outils,
  fichiers, shell, web search/fetch, MCP, skills, plan, objectifs, jobs,
  sous-agents, planification, token meter, compaction, persona, notes, mémoire,
  providers, ArtCraft, etc.
- **12 extensions expérimentales** visibles mais désactivées par défaut.
- Chaque capacité indique son outil natif ou sa vue de contrôle.
- Les outils réellement installés restent détectés dynamiquement.

## Bibliothèque de skills

Ultimate IAH embarque **12 skills par défaut** : recherche web, revue de code,
TDD, debug, Git, GitHub, notes, sécurité, release, automatisation, refactor et
ArtCraft.

- seeding idempotent dans `~/.agents/skills` ;
- aucun fichier utilisateur écrasé ;
- sélection automatique des skills pertinents avant chaque requête ;
- lecture/écriture via `ultimate_skills` ;
- bouton **Bibliothèque par défaut** pour restaurer les skills manquants ;
- les skills projet restent prioritaires dans le workspace.

## Écosystème GitHub

- Connexion via le CLI `gh` et son keyring, sans lire ni stocker le token.
- Liste, recherche et sélection des dépôts.
- Création de projet public/privé, avec clonage optionnel dans le workspace.
- Clonage d’un dépôt existant.
- Issues et pull requests, création d’issue.
- Outils agent `github_status`, `github_repos`, `github_issues`, `github_pulls`,
  `github_clone`, `github_create_repo`, `github_create_issue`.
- Vue **GitHub** (Ctrl+Alt+H), intégrée au Cockpit et à l’Écosystème.

## Plugins et ArtCraft

![Plugins et ArtCraft](assets/screenshot-plugins.png)

Le registre natif expose les outils au harness Ultimate Agent :

- **14 craft** : CADCraft, GridCraft, SoundCraft, DeckCraft, WordCraft,
  PhotoCraft, LightCraft, PdfCraft, VectorCraft, EffectCraft, FilmCraft,
  DesignCraft, FigCraft et TextureCraft.
- Détection réelle des binaires, versions, activation/désactivation et
  persistance de l’état.
- Outils agent dynamiques `artcraft_<craft>` : catalogue, version et exécution
  CLI contrôlée par permission.
- Manifestes de plugins externes dans
  `~/.config/Crasy/deepcrasy-h/plugins/manifests`.
- Aucun outil n’est déclaré s’il n’est pas réellement installé.

## Providers et MCP

![Providers](assets/screenshot-connections.png)

![Serveurs MCP](assets/screenshot-mcp.png)

- **Hub providers centralisé** : catalogue de 46 presets classés en Directs,
  Agrégateurs, Asie, Entreprise et Locaux.
- **Application fiable** : chaque preset ouvre un formulaire pré-rempli ;
  URL, modèles, protocole et clé sont vérifiés avant l'enregistrement.
- **Protocoles** : OpenAI-compatible et Anthropic Messages.
- **Provider et modèle par défaut** : sélection en un clic, utilisée
  immédiatement par Ultimate Agent.
- **Clé DeepSeek unique** : une saisie dans Providers crée le provider
  `deepseek-official`, alimente Ultimate Agent, `agent.env` et le terminal,
  puis définit `deepseek-chat` par défaut si aucun modèle ne l'est déjà.
- **Édition et suppression** des providers et diagnostic des liaisons.
- **MCP stdio et HTTP** : ajout, activation, suppression, presets Filesystem /
  Fetch / Git, utilisés directement par Ultimate Agent.
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

- File de tâches, exécution une à une par le moteur agent natif, intervalle ou manuel.
- Timeout, reprise, écriture des résultats en mémoire.
- Autonomie opt-in : elle tourne tant que l'application est ouverte.

## Terminal

- Terminal PTY réel (bash, zsh, fish) dans le workspace partagé.
- Projet, Git, fichiers, diff, outils et skills.
- Plus aucun harness web DSH embarqué.

## Raccourcis

| Touche | Action |
|---|---|
| `Ctrl+0` | Cockpit |
| `Ctrl+1` | Ultimate Agent |
| `Ctrl+2` | Terminal agent |
| `Ctrl+4` | Réglages |
| `Ctrl+6` | Écosystème |
| `Ctrl+7` | Fichiers |
| `Ctrl+8` | Providers |
| `Ctrl+Alt+U` | Ultimate Agent |
| `Ctrl+Alt+B` | Bots |
| `Ctrl+Alt+N` | Notes |
| `Ctrl+Alt+P` | Plugins et ArtCraft |
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
sudo pacman -U ultimate-iah-1.15.0-1-x86_64.pkg.tar.zst
```

Le paquet remplace `ultimate-hai` et `deepcrasy-h`. Les données existantes sont conservées dans
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
