# DeepCrasy H

**Le cockpit Omarchy de DeepSeek Harness et OpenCode.**

[![Licence MIT](https://img.shields.io/badge/licence-MIT-3FC9D6.svg)](LICENSE)

<img src="assets/icon-256.png" width="96" align="right" alt="Logo DeepCrasy H">

DeepCrasy H réunit les deux agents qui comptent sur le poste : le **DeepSeek
Harness** (son interface web complète, ses skills, ses sessions) et
**OpenCode** (son agent, ses modèles, ses appels d'outils), dans une
application Qt Quick native, installable comme n'importe quel paquet Omarchy.
Un seul espace de travail, deux moteurs, un cockpit, aucun terminal à ouvrir.

![Crasy Harness](assets/screenshot-home.png)

## Le cockpit

La vue **Accueil** (`Ctrl+0`) rassemble l'état réel du poste :

- le **dossier de travail**, sa branche Git et le nombre de modifications ;
- l'état du **harness** (profil, adresse, santé, test de connexion) ;
- l'état d'**OpenCode** (session, modèle, mode, jetons consommés) ;
- un **diagnostic** des binaires, du portail, de QtWebEngine, de Git et des
  notifications ;
- les **sessions récentes** pour reprendre un travail en un clic ;
- des actions directes : nouvelle session, ouvrir le harness, vue partagée,
  changer de dossier, lancer le diagnostic.

Les cartes se replient en une colonne dans une fenêtre étroite et suivent le
thème actif.

## Palette de commandes

`Ctrl+K` (ou la loupe de l'en-tête) ouvre une palette qui interroge tout :
vues, actions, thèmes installés, langues, sessions récentes. La recherche
ignore les accents — « reglages » trouve « Réglages » — et tout se pilote au
clavier (`↑↓`, `Entrée`, `Échap`).

![Palette de commandes](assets/screenshot-palette.png)

## Thèmes

DeepCrasy H lit les `colors.toml` d'Omarchy et suit le thème du bureau par
défaut. **Réglages → Apparence** liste tous les thèmes installés avec un aperçu
réel de leur palette ; le choix s'applique instantanément, reste propre à
l'application et ne touche pas à la configuration du bureau.

Tout est dérivé de la palette : fonds, bordures, texte secondaire, icônes
teintées, accents, couleurs de statut. Les thèmes clairs restent lisibles, les
sombres gardent leur caractère.

![Thèmes installés](assets/screenshot-themes.png)

![Thème clair](assets/screenshot-light.png)

## Langues

L'interface existe en **français** et en **anglais**. En mode automatique, elle
suit la langue d'Omarchy, puis celle du navigateur embarqué (celle que le
harness utilise déjà), puis la locale du système. **Réglages → Langue** permet
de forcer Français, English ou Auto — pour DeepCrasy H uniquement.

Le chat, les appels d'outils, les permissions, les erreurs, le cockpit et la
palette passent tous par le même catalogue.

## Options et connexions

![Options et connexions](assets/screenshot-options.png)

- **Vue au démarrage** : dernière vue, accueil, chat, harness ou vue partagée.
- **Lancement automatique** du harness et d'OpenCode, indépendamment.
- **Part de la vue partagée**, réglable au pourcentage.
- **Notification de bureau** à la fin d'un tour quand la fenêtre est inactive.
- **Diagnostic complet** : `dsh`, `opencode`, serveur harness, session
  OpenCode, portail XDG, QtWebEngine, espace de travail, Git, notifications.
- **Test de connexion** au harness (requête HTTP réelle, sans afficher le
  token).
- **Fournisseurs OpenCode** détectés depuis les modèles disponibles, et
  redémarrage des deux moteurs.

## Terminal

`Ctrl+5` ouvre un vrai terminal dans le dossier de travail. C'est un **PTY**
(`forkpty`), pas un faux champ de commande : le shell de l'utilisateur, sa
complétion, ses couleurs et ses programmes plein écran. Il reste vivant quand
on change de vue, suit le thème actif, se redimensionne avec la fenêtre, et
`Ctrl+Shift+C` / `Ctrl+Shift+V` copient et collent. Le rendu est xterm.js (MIT).

![Terminal](assets/screenshot-terminal.png)

## Serveurs MCP

**Réglages → Serveurs MCP** gère les serveurs d'outils exposés à OpenCode :
liste fusionnée avec type, cible et portée, ajout d'un serveur local (`npx …`)
ou distant (`https://…`), retrait, et statut demandé à `opencode mcp list`.
Deux portées sont disponibles : **Global** dans
`~/.config/opencode/opencode.json`, et **Projet** dans
`<espace de travail>/opencode.json`. Le fichier choisi est modifié de façon
atomique, sans toucher au thème, à l'autoupdate ni au reste de la
configuration. Un bouton relance OpenCode pour charger un serveur qui vient
d'être ajouté ; le statut reste best-effort et peut afficher « Non testé » tant
que le service n'a pas rechargé.

Les serveurs **distants** ont un bouton clé : DeepCrasy H lance
`opencode mcp auth <nom> --standalone`, reconnaît l'URL d'autorisation,
l'ouvre dans le navigateur et attend la fin du flux. L'URL reste cliquable
dans le bandeau, l'authentification peut être annulée, et le statut est
rafraîchi au retour. Un second bouton lance
`opencode mcp logout <nom>` et efface les identifiants OAuth stockés ; le
résultat s'affiche dans le même bandeau. Le CLI sortant 0 même en cas
d'échec, le résultat est aussi lu dans sa sortie.

![Serveurs MCP](assets/screenshot-mcp.png)

## En bref

- **Un seul espace de travail.** Le dossier choisi est partagé : le harness y
  démarre son serveur, OpenCode y ouvre sa session.
- **Le harness est embarqué.** `dsh web` est lancé sur un port libre ; l'URL
  tokenisée qu'il imprime est chargée dans le moteur Chromium de Qt.
- **OpenCode est natif.** `opencode acp` est piloté via l'**Agent Client
  Protocol** : sessions, modèles, modes, appels d'outils, permissions et
  streaming sont rendus par l'application.
- **Vue partagée** harness + chat (à partir de 1180 px de large), avec bascule
  instantanée et transcript conservé.
- **Terminal intégré** (`Ctrl+5`) : PTY réel, shell de l'utilisateur, thème,
  copier/coller, redémarrage.
- **Serveurs MCP** : liste fusionnée global/projet, ajout local ou distant,
  retrait, authentification et déconnexion OAuth des serveurs distants, statut
  et redémarrage d'OpenCode depuis Réglages.
- **Thèmes complets, français et anglais, palette de commandes, diagnostic,
  Git, notifications.**
- **Une seule fenêtre Wayland**, un binaire, une entrée de lanceur, une icône.

## Installer

### Paquet Arch / Omarchy

```sh
cd ~/Work/deepcrasy-h
./bin/install          # construit puis makepkg -fsi (demande sudo)
```

Le paquet installe `/usr/bin/deepcrasy-h`, l'entrée `.desktop`, l'icône
scalable et les PNG 256/512. L'application apparaît ensuite dans le lanceur
Omarchy (`Super + Espace`).

### Sans root

```sh
./bin/install-user     # ~/.local/bin + ~/.local/share/{applications,icons}
```

### Depuis les sources

```sh
./bin/build            # qmake6 + make → build/deepcrasy-h
./build/deepcrasy-h    # lance l'application
```

### Prérequis

| Outil | Rôle | Vérification |
| --- | --- | --- |
| `dsh` | DeepSeek Harness, lancé en `dsh web` | `dsh --version` |
| `opencode` | OpenCode, lancé en `opencode acp` | `opencode --version` |
| `git` | branche et modifications du cockpit, optionnel | `git --version` |
| Qt 6 | `qt6-base`, `qt6-declarative`, `qt6-webengine`, `qt6-svg`, `qt6-wayland` | fourni par Omarchy |

```sh
deepcrasy-h doctor          # tableau lisible
deepcrasy-h doctor --json   # pour un agent ou un script, code 0 si tout est prêt
```

Les chemins vers `dsh` et `opencode`, ainsi que le profil du harness, se
règlent dans **Réglages** (`Ctrl+4`).

## Utilisation

| Raccourci | Action |
| --- | --- |
| `Ctrl+0` | Cockpit Crasy Harness |
| `Ctrl+1` | DeepSeek Harness |
| `Ctrl+2` | Chat OpenCode |
| `Ctrl+3` | Vue partagée (les deux panneaux) |
| `Ctrl+4` | Réglages |
| `Ctrl+5` | Terminal intégré |
| `Ctrl+K` | Palette de commandes |
| `Ctrl+L` | Aller à la saisie |
| `Entrée` | Envoyer le message |
| `Maj+Entrée` | Nouvelle ligne |
| `Ctrl+N` | Nouvelle session OpenCode |
| `Ctrl+M` | Choisir le modèle |
| `Ctrl+R` | Relancer le harness |
| `?` / `F1` | Toutes les touches |
| `Échap` | Fermer la fenêtre active |
| `Q` | Quitter (confirmation si un tour est en cours) |

Le bouton de dossier dans l'en-tête change d'espace de travail via le portail
XDG. Le harness redémarre dans le nouveau dossier et OpenCode y ouvre une
session ; le transcript en cours est abandonné, l'ancienne session reste
consultable dans l'historique.

La vue partagée demande au moins 1180 px de large. En dessous, l'application
revient au chat et affiche un message : une fenêtre carrelée sur une moitié
d'écran reste ainsi lisible.

Le chat et le sélecteur de modèle :

![Chat](assets/screenshot-chat.png)

![Sélecteur de modèle](assets/screenshot-picker.png)

## Comment ça marche

```text
┌──────────────────────── DeepCrasy H (Qt Quick) ─────────────────────────┐
│  Cockpit · Palette · Thèmes · Réglages                                  │
│  DeepSeek Harness                    │  OpenCode                         │
│  QtWebEngine ← http://127.0.0.1:PORT │  chat natif ← JSON-RPC/stdio       │
│   (?token=…)                         │                                   │
└──────────────┬───────────────────────┴──────────────┬────────────────────┘
               │ QProcess                             │ QProcess
        ┌──────▼──────┐                        ┌──────▼──────┐
        │  dsh web    │                        │ opencode acp│
        └─────────────┘                        └─────────────┘
                     même dossier de travail
```

- `src/harnessserver.*` lance `dsh web --no-open --port 0`, lit la ligne
  `dsh web: http://…/?token=…` et la transmet à la vue.
- `src/acpclient.*` parle JSON-RPC 2.0 sur stdio : `initialize`,
  `session/new`, `session/load`, `session/list`, `session/prompt`,
  `session/cancel`, `session/set_config_option`, `session/update` et
  `session/request_permission`.
- `src/backend.*` est la façade QML : un espace de travail, deux moteurs, un
  seul état. Il porte aussi le diagnostic, Git, le test de connexion et les
  notifications.
- `src/terminalsession.*` ouvre un PTY avec `forkpty`, décode l'UTF-8 au fil de
  l'eau et expose l'entrée/sortie à xterm.js via QWebChannel. `src/terminal/`
  contient xterm.js et son addon de redimensionnement.
- `src/mcpmanager.*` lit et écrit la section `mcp.servers` des configurations
  OpenCode globale et de projet, et interroge `opencode mcp list` en
  arrière-plan pour les statuts.
- `src/messagemodel.*` expose le transcript comme un vrai modèle de liste :
  un token streame ne réécrit qu'une ligne, la ListView garde ses délégués et
  sa position. L'auto-scroll est coalescé à 40 ms et le bas du transcript garde
  une marge au-dessus du composer.
- `src/gitstatus.*` lit `git status --porcelain=v1 -b` sans toucher au dépôt ;
  `src/theme.*` lit les palettes Omarchy et en dérive toute l'interface ;
  `src/iconprovider.*` teinte les SVG avec la bonne couleur.
- `src/config.*` persiste tout dans
  `~/.config/Crasy/deepcrasy-h/config.json` (écriture atomique).
- Le cookie du harness persiste dans
  `~/.local/share/Crasy/deepcrasy-h/QtWebEngine/deepcrasy-h`, mais l'URL
  tokenisée est rechargée à chaque démarrage.
- Aucun réseau sortant n'est ajouté par DeepCrasy H. Le harness écoute sur
  l'interface locale ; OpenCode reste un processus enfant.

`docs/ARCHITECTURE.md` détaille les décisions et les limites.

## Configuration

| Chemin | Contenu |
| --- | --- |
| `~/.config/Crasy/deepcrasy-h/config.json` | espace de travail, profil, binaires, vue, thème, langue, options |
| `~/.local/share/Crasy/deepcrasy-h/QtWebEngine/` | cookies et stockage local du harness |
| `~/.cache/Crasy/deepcrasy-h/` | cache WebEngine |

Le profil `web` est utilisé par défaut : un profil personnel peut contenir des
plugins à instance unique (l'agent du bureau, par exemple) qui refusent un
second lancement. Indiquez votre profil dans **Réglages** si vous savez qu'il
peut cohabiter.

## Développement

```sh
./bin/build     # compile
./bin/test      # tests Qt, dont un vrai aller-retour ACP avec OpenCode
./bin/install   # paquet système
```

Les tests couvrent l'extraction de l'URL tokenisée, la persistance et la
validation de la configuration (options comprises), l'édition sûre de la
configuration MCP, la lecture Git, le thème, les catalogues de langue, un
**vrai** PTY (commande envoyée au shell et sortie relue), et un **vrai**
`opencode acp` : démarrage, session, prompt, réponse streamée. Pour sauter ce dernier (pas de réseau) :

```sh
DEEPCRASY_SKIP_INTEGRATION=1 ./bin/test
```

Structure :

```text
deepcrasy-h.pro         projet qmake
bin/build               qmake6 + make
bin/test                Qt Test hors écran
bin/install             makepkg -fsi
bin/install-user        installation dans ~/.local
src/                    C++ et QML, à plat
tests/                  Qt Test
pkgbuild/               PKGBUILD, .desktop, icône
assets/                 logo et captures
```

Pour une capture reproductible sans données personnelles :

```sh
QT_QPA_PLATFORM=offscreen ./build/deepcrasy-h \
  --demo --screenshot /tmp/deepcrasy.png

# Une autre taille, un thème précis, la palette ouverte
QT_QPA_PLATFORM=offscreen ./build/deepcrasy-h \
  --theme catppuccin-latte --view home --height 1000 --palette \
  --screenshot /tmp/cockpit.png
```

## Stabilité

Qt 6.12 peut crasher dans `QSGBatchRenderer::prepareAlphaBatches()` sur un
glyphe de texte en distance field dont la matrice est nulle. DeepCrasy H force
donc le **rendu natif des glyphes** (`Text.NativeRendering`) et la boucle Quick
`basic` (même thread que l'interface), sauf si `QSG_RENDER_LOOP` est déjà défini.
C'est un contournement de deux bugs Qt ; il pourra être retiré quand Qt Quick
sera corrigé.

## Limites connues

- La vue partagée n'a pas de séparateur déplaçable à la souris ; la part est
  réglée dans **Options** (57 % par défaut).
- Le rendu Markdown s'appuie sur `Text.MarkdownText`. Les blocs de code sont
  lisibles, mais sans coloration syntaxique.
- Le harness peut afficher sa propre fenêtre d'accueil à la première
  utilisation ; c'est son interface, pas celle de DeepCrasy H.
- ACP est jeune : les mises à jour de session inconnues sont ignorées plutôt
  que devinées.
- Le thème choisi ne s'applique qu'à DeepCrasy H. L'interface web du harness
  garde son propre thème, et `omarchy theme set` continue de piloter le bureau.

## Licence

MIT — voir [LICENSE](LICENSE). Le logo appartient à Crasy. Le terminal embarque
xterm.js (MIT) — voir [THIRD_PARTY.md](THIRD_PARTY.md).

Dépôt : <https://github.com/iatexplique-debug/DeepCrasy-H>
