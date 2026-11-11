# Architecture

## Le problème

Deux agents, deux façons de travailler. Le DeepSeek Harness est un serveur web
avec une interface riche (sessions, skills, fichiers, terminal). OpenCode est
un agent en terminal, rapide à lancer, avec ses propres modèles et outils.
Les faire cohabiter demandait jusqu'ici deux fenêtres, deux dossiers, deux
copier-coller.

DeepCrasy H ne fusionne pas les agents : il fusionne **la fenêtre** et
**l'espace de travail**, et laisse chaque agent rester lui-même.

## Choix d'architecture

### Qt Quick plutôt qu'Electron

Omarchy installe déjà Qt 6, Qt Quick, QtWebEngine et Qt Wayland. Un binaire
unique de 500 Ko remplace un runtime Chromium complet, s'intègre au thème et
aux arrondis Hyprland, et suit la langue du bureau. QtWebEngine fournit
exactement ce qu'il faut pour le harness : un Chromium récent, un profil
persistant, des cookies.

### Le harness comme vue web

`dsh web` est conçu pour un navigateur. Réimplémenter son interface serait
une course sans fin. DeepCrasy H lance donc :

```text
dsh [web | --profile <nom>] --no-open --port 0
```

et lit la ligne de démarrage :

```text
dsh web: http://127.0.0.1:35043/?token=…
```

L'URL complète, token compris, est chargée dans le `WebEngineView`. Le token
est à usage unique et n'est jamais persisté. Le profil `deepcrasy-h` garde les
cookies et le stockage local, donc l'état de l'interface survit aux
redémarrages.

### OpenCode en ACP, pas en terminal

Un émulateur de terminal dans QML aurait été un projet à part entière, et un
TUI ne se prête pas aux panneaux redimensionnables. OpenCode expose
`opencode acp`, un serveur **Agent Client Protocol** : JSON-RPC 2.0, une
requête par ligne, sur stdio.

Le client (`src/acpclient.*`) implémente :

| Méthode | Usage |
| --- | --- |
| `initialize` | handshake, capacités (pas de fs ni de terminal côté client) |
| `session/new` | nouvelle session dans le dossier partagé |
| `session/list` | historique du dossier |
| `session/load` | reprise d'une session |
| `session/prompt` | envoi d'un tour |
| `session/cancel` | notification d'annulation |
| `session/set_config_option` | modèle et mode |
| `session/update` | texte, réflexion, outils, plan, usage, commandes |
| `session/request_permission` | autorisation native, avec options |

Les notifications de session enfant (sous-agents) sont ignorées : mélanger
leur transcript à celui du parent serait trompeur.

### Les thèmes

Omarchy décrit chaque thème dans un `colors.toml` sémantique : `background`,
`foreground`, `accent`, `selection`, et une série de couleurs nommées
(`red`, `green`, `cyan`, `orange`, variantes `bright_*`). `src/theme.*` lit ce
fichier et en dérive tout ce dont l'interface a besoin : surfaces, bordures,
texte secondaire, halos, ombre, couleurs de statut.

Deux modes :

- **Suivre Omarchy** (défaut) : la palette est relue quand le bureau change de
  thème. Le veilleur réarme ses chemins à chaque rechargement, parce qu'un
  changement de thème remplace le dossier d'état.
- **Thème choisi** : n'importe quel thème installé peut être utilisé pour
  l'application seule. Le choix est enregistré dans `config.json` et ne touche
  jamais à la configuration du bureau.

Les surfaces sont calculées en mélangeant `background` et `foreground` plutôt
qu'en codant des gris : un thème clair obtient des cartes légèrement grises et
un texte secondaire assez sombre, un thème sombre l'inverse. `Theme::dark()`
pilote aussi `Material.theme`.

Les icônes sont des SVG blancs. `src/iconprovider.*` les rend avec
`QSvgRenderer`, puis applique la couleur avec `CompositionMode_SourceIn` ; QML
demande `image://themeicon/terminal.svg?color=%23…`. Cela évite les effets QML
qui laissaient des icônes blanches ou des boîtes noires selon la palette.

`src/ThemeCard.qml` affiche un aperçu réel de chaque palette dans Réglages :
la carte est peinte avec les couleurs du thème qu'elle représente.

### Instance unique et démarrage

`main.cpp` ouvre un `QLocalServer` par utilisateur (`deepcrasy-h-<uid>`). Une
deuxième instance envoie `raise`, attend un accusé, puis quitte ; la première
remonte sa fenêtre. Si le serveur ne répond pas (instance bloquée), le socket
est considéré comme périmé et une nouvelle instance démarre. Le catalogue
`opencode models` est chargé à l'ouverture du sélecteur, pas pendant
`engine.load`, pour que la fenêtre apparaisse immédiatement.

### Le rendu et le crash du render thread

Sous Qt 6.12, un nœud de glyphe (`QSGDistanceFieldGlyphNode` ou
`QSGDefaultGlyphNode` selon le mode de rendu) peut arriver dans le batch
renderer avec une matrice nulle ; `Element::computeBounds()` déréférence alors
`node->matrix()` et segfaulte dans `prepareAlphaBatches()`. Le core identifie
précisément ces nœuds, et la pile ne contient que Qt Quick : ce n'est ni l'ACP
ni un OOM.

`main.cpp` installe un garde-fou sur `QQuickWindow::beforeRendering` : il
parcourt le scene graph depuis le `contentItem` et appelle
`setRendererMatrix(&identity)` sur tout nœud de géométrie dont la matrice est
nulle. Une matrice nulle signifie identité, donc le rendu ne change pas. La
boucle `QSG_RENDER_LOOP=basic` est forcée pour que ce parcours reste sur le
thread de l'interface. Un stream de stress de 200 secondes et les vues
terminal/harness passent avec ce réglage. Le garde-fou utilise l'API privée
`QQuickItemPrivate` ; il est à retirer quand Qt Quick sera corrigé.

### Le transcript

`AcpClient` possède un `MessageListModel` (QAbstractListModel) plutôt qu'une
`QVariantList` : les rôles `role`, `text`, `streaming`, `title`, `status`,
`output`, `entries`, etc. sont exposés directement au délégué. Un chunk de
streaming appelle `setAt()`, qui émet `dataChanged` sur une seule ligne ; les
autres lignes et les délégués restent en place. Les ajouts passent par
`beginInsertRows`/`endInsertRows`.

Côté QML, l'auto-scroll ne s'exécute plus à chaque chunk : un timer de 40 ms
rappelle `positionViewAtEnd()` au plus toutes les 40 ms, seulement si le bas
est suivi. Un footer de 12 px et une marge basse empêchent la dernière ligne
de disparaître sous le composer. Un glissement utilisateur désactive le suivi,
et le prochain retour en bas le réactive.

### Le cockpit et la palette

La vue **Accueil** n'est pas un tableau de bord décoratif : `Backend` expose
l'état réel des deux moteurs, le dossier, Git, le diagnostic et les sessions.
Les cartes se contentent de le lire. `HomePane` rafraîchit Git et le diagnostic
quand elle devient visible, puis à chaque changement d'état des moteurs.

La **palette de commandes** est une simple liste d'objets construits dans
`Main.qml` : vues, actions, thèmes, langues, sessions récentes. Elle ne connaît
que des identifiants (`view:home`, `theme:nord`, `session:…`) qu'un unique
`runCommand()` traduit en appels à `Backend`. La recherche replie les accents
côté QML, sans dépendance.

### L'écosystème

`Ecosystem` est un scanner local, sans réseau : il détecte les binaires connus
dans le `PATH`, lit le fichier projet (package.json, Cargo.toml, go.mod,
pyproject.toml, .pro, CMakeLists.txt, Makefile), en déduit des commandes sûres,
et lit les `SKILL.md` de `~/.agents/skills`, `~/.config/opencode/skills`,
`~/.claude/skills` et du projet. Les versions d'outils sont sondées une fois et
mises en cache ; **Rafraîchir** force un nouveau scan.

Le panneau `EcosystemPane` rend ces données et propose les recettes et le brief
de contexte. Les commandes projet passent par `TerminalSession::run()` : si le
shell n'est pas encore lancé, la commande est mémorisée et écrite dès que le
PTY démarre ; sinon elle part immédiatement. Le brief est prérempli dans le
composer, jamais envoyé sans validation.

### 9Router

`NineRouterManager` traite 9Router comme un service local : détection de
`9router` (global ou installé dans les données de l'app), installation npm
locale, démarrage sur `127.0.0.1:20128`, login dashboard (mot de passe initial
`123456`), clé API locale, catalogue `/v1/models`, logs et arrêt propre.

Le dashboard officiel est embarqué dans un `WebEngineView` avec un profil
persistant. Le panneau d'intégration choisit les modèles à exposer :

- OpenCode : `POST /api/cli-tools/opencode-settings`, qui écrit le provider
  OpenAI-compatible et le modèle actif, puis redémarrage ACP.
- Harness : génération d'un overlay `llm-pi-ai` avec les providers existants
  du profil fusionnés, d'un provider `nine-router`, et d'un fichier
  `nine-router.env` (mode 600). `HarnessServer` ajoute
  `--profile <profil> --patch <overlay>` et injecte `NINE_ROUTER_API_KEY`
  dans l'environnement de `dsh`. Le patch peut aussi définir
  `agent-default-model`.

Un marqueur `nine-router.enabled` et la présence du patch permettent de
redémarrer 9Router avec l'application. Le port reste local et le dashboard
9Router n'est jamais exposé au réseau.

### Gestionnaire de providers

`ProviderManager` lit et écrit `~/.config/opencode/opencode.json`. Chaque
provider personnalisé est une entrée de `providers` avec le package
`@opencode/ai/providers/openai-compatible`, `settings.baseURL`,
`settings.apiKey` et une carte de modèles. Le format a été validé contre
OpenCode avec un faux serveur OpenAI : `opencode models --standalone` liste le
modèle et `opencode run --model …` répond.

L'écriture est atomique (`QSaveFile`) et le fichier passe en mode 600. QML ne
reçoit que l'ID, l'URL, les modèles et un suffixe masqué. Le bouton Tester
interroge `baseURL/models` avec la clé, sans l'exposer. Après sauvegarde,
`opencode service restart` puis le redémarrage ACP rendent les nouveaux
modèles disponibles. Les comptes OAuth/integrés restent gérés par
`opencode auth list --standalone` dans la carte **Comptes OpenCode**.

### Modèles, providers et pièces jointes

`Ecosystem::refreshModels()` interroge `opencode models` et
`opencode auth list`, met les résultats en cache et les expose au sélecteur de
modèle du composer ainsi qu'à la carte Écosystème. Le modèle courant est
toujours conservé dans la liste, même s'il ne vient pas du catalogue.

Le composer garde jusqu'à cinq fichiers joints ; `WorkspacePane` envoie le
chemin et `Backend`/QML récupèrent l'aperçu. À l'envoi, chaque fichier devient
un bloc `<file path="…">` limité à 14 000 caractères, suivi du texte saisi.
L'historique local garde 50 prompts et se parcourt avec `↑`/`↓`.

### Les erreurs OpenCode

Un échec de prompt peut arriver en réponse JSON-RPC, mais aussi comme texte
d'assistant. `AcpClient::classifyFailure()` reconnaît les limites d'usage et les
providers manquants ; `detectFailuresInTranscript()` convertit le message en
rôle `error` à la fin du prompt, et les erreurs JSON-RPC créent directement ce
rôle. Le dernier prompt est gardé pour `retryLastPrompt()`. QML rend une carte
avec trois actions : ouvrir le sélecteur de modèle, remplacer la clé du
provider courant (`opencode auth logout`, `opencode auth login`, puis
`opencode service restart`) dans le terminal, ou réessayer.

### Le workspace et les profils

`WorkspaceModel` liste les fichiers avec `git ls-files` quand le dossier est un
dépôt, sinon avec un parcours borné qui ignore `build`, `node_modules`,
`.git`, etc. La lecture et le diff passent par un chemin résolu et vérifié
(`canonicalFilePath` doit rester sous le workspace). L'aperçu est limité à
256 Kio et détecte les binaires ; le diff à 320 Kio. Le QML ne fait que
sélectionner, filtrer et afficher.

Les profils sont une liste persistée dans `Config` (`id`, `name`, `model`,
`mode`) avec un `activeProfile`. `Backend::applyProfile()` applique le modèle et
le mode, et `configOptionsChanged` réapplique le profil actif une fois quand
OpenCode est prêt. Le choix manuel du modèle ou du mode ne détruit pas le
profil ; il reste le point de départ.

### Le terminal

`TerminalSession` ouvre un PTY avec `forkpty`, prépare l'environnement avant le
fork (le fils n'appelle que `chdir` et `execle`), puis lit le descripteur
maître via `QSocketNotifier`. Un `QStringDecoder` garde les caractères UTF-8
coupés entre deux lectures.

Le pont vers la page est QWebChannel : le C++ émet `output`, la page xterm.js
l'écrit ; l'entrée clavier et les redimensionnements remontent par `input` et
`resize`. Le thème est poussé par `runJavaScript` à chaque changement de
palette. Copier/coller passe par le presse-papiers Qt plutôt que par l'API
navigateur, qui n'est pas fiable dans QtWebEngine.

Le shell reste vivant quand la vue est masquée ; il est arrêté avec
l'application. Quand le terminal a le focus, les raccourcis globaux qui
entreraient en conflit avec le shell (Ctrl+K, Ctrl+R, Ctrl+L, Ctrl+N, ?, Q,
Échap) lui sont rendus.

### Les serveurs MCP

`McpManager` possède la section `mcp.servers` de deux configurations OpenCode :
la globale (`~/.config/opencode/opencode.json`, ou `OPENCODE_CONFIG` si défini)
et celle du projet (`<espace de travail>/opencode.json`). `setWorkspace()`
suit l'espace de travail et recalcule le chemin projet ; la liste affichée
fusionne les deux portées, chaque entrée portant son `scope`. Ajouts et
retraits réécrivent le document JSON choisi avec `QSaveFile` en préservant
toutes les autres clés ; une configuration illisible est signalée et jamais
écrasée. Les commandes locales sont découpées avec `QProcess::splitCommand`,
les URL distantes doivent être http(s).

L'authentification d'un serveur distant lance
`opencode mcp auth <nom> --standalone` (répertoire temporaire pour le global,
espace de travail pour le projet). La sortie fusionnée est lue au fil de
l'eau ; la première URL d'autorisation externe est proposée et ouverte via
`QDesktopServices`, tout en restant cliquable dans l'interface. Le processus
peut être annulé et expire au bout de trois minutes. Comme le CLI sort avec le
code 0 même quand l'authentification échoue, les marqueurs de sa sortie sont
aussi examinés. Un succès relance `mcp list` pour rafraîchir le statut.

Le statut vient de `opencode mcp list`, lancé en arrière-plan dans l'espace de
travail avec un délai de 20 s. Le format texte (glyphes `✓`, `✗`, `○`, `-`) est parsé et fusionné avec
la liste issue du fichier ; un serveur non reconnu reste « non testé ». Comme
OpenCode garde sa configuration en mémoire, l'ajout ne prend effet qu'après un
redémarrage — le bouton correspondant relance le client ACP.

### Connexions, diagnostic et notifications

- `runDiagnostics()` vérifie les binaires résolus (y compris un chemin absolu
  choisi dans Réglages), les états du harness et d'OpenCode, le portail XDG,
  QtWebEngine, l'espace de travail, Git et le service de notifications. Les
  versions sont mises en cache et invalidées quand le binaire change.
- `testHarness()` fait une vraie requête HTTP avec `QNetworkAccessManager` sur
  l'URL tokenisée, sans jamais afficher le token ; seul le statut est exposé.
- `GitStatus::parse()` lit `git status --porcelain=v1 -b`, sans écriture ni
  dépendance à libgit2. Le processus tourne en asynchrone.
- Les notifications passent par `org.freedesktop.Notifications` en D-Bus, et
  seulement si la fenêtre n'a pas le focus.
- Les options (vue au démarrage, démarrages automatiques, part de la vue
  partagée, notifications) vivent dans `Config` et sont relues au lancement.
  `Backend::applyStartupView()` applique la vue choisie avant le chargement de
  l'interface.

### Un seul propriétaire d'état

`Backend` est la seule façade exposée à QML. Il possède `Config`,
`HarnessServer`, `AcpClient` et `PortalFolderPicker`. QML ne parle jamais
directement à un processus.

La configuration est un unique objet JSON, réécrit via `QSaveFile` (fichier
temporaire puis renommage). Un plantage ne laisse pas de fichier à moitié
écrit.

## Cycle de vie

1. `main` initialise QtWebEngine, le traducteur et le thème.
2. `Backend::boot()` démarre le harness puis OpenCode.
3. Quand l'ACP est prêt, `initialize` a répondu ; une session est créée dans
   l'espace de travail.
4. Changer d'espace de travail : `dsh` redémarre dans le nouveau dossier,
   OpenCode ouvre une nouvelle session. L'ancienne reste dans `session/list`.
5. Quitter : les destructeurs terminent les deux processus enfants
   (`SIGTERM`, puis `SIGKILL` après un délai).

## Défaillances

- `dsh` absent ou en échec : la vue harness affiche le détail, les dernières
  lignes du journal et un bouton Recommencer.
- `opencode` absent ou en échec : le chat affiche la même chose, avec
  Redémarrer.
- Un tour interrompu par la mort du processus : le client remet `busy` à faux
  et ajoute une note dans le transcript.
- L'URL du harness n'arrive pas en 60 s : échec explicite, pas d'attente
  infinie.

## Sécurité

- Les deux processus tournent avec les droits de l'utilisateur, sans privilège
  supplémentaire.
- Le harness écoute sur l'interface locale ; le token dans l'URL est la seule
  autorisation.
- Aucune télémétrie, aucun service réseau ajouté.
- Les permissions OpenCode ne sont jamais accordées automatiquement : chaque
  `session/request_permission` ouvre la boîte de dialogue, et Échap refuse.

## Tests

`tests/deepcrasy-h_tests.cpp` couvre la logique pure (URL, configuration et
options, lecture Git, thème, langues) et un véritable aller-retour ACP :
`opencode acp` est lancé, une session créée, un prompt envoyé, la réponse
streamée vérifiée. Ce test peut être sauté avec
`DEEPCRASY_SKIP_INTEGRATION=1`.

## Pistes

- Séparateur déplaçable en vue partagée.
- Coloration syntaxique des blocs de code.
- Onglets de sessions OpenCode dans le panneau.
- Notification système quand un long tour se termine.
