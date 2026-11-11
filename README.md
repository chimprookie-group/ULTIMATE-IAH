# DeepCrasy H

**DeepSeek Harness et OpenCode dans une seule fenêtre Omarchy.**

[![Licence MIT](https://img.shields.io/badge/licence-MIT-3FC9D6.svg)](LICENSE)

<img src="assets/icon-256.png" width="96" align="right" alt="Logo DeepCrasy H">

DeepCrasy H réunit les deux agents qui comptent sur le poste : le **DeepSeek
Harness** (son interface web complète, ses skills, ses sessions) et
**OpenCode** (son agent, ses modèles, ses appels d'outils), dans une
application Qt Quick native, installable comme n'importe quel paquet Omarchy.
Un seul espace de travail, deux moteurs, aucun terminal à ouvrir.

![DeepCrasy H](assets/screenshot-chat.png)

Le modèle et le mode se choisissent dans un panneau dédié : recherche, noms
complets, fournisseur et navigation clavier, au lieu d'une liste Material
tronquée qui débordait de la fenêtre.

![Sélecteur de modèle](assets/screenshot-picker.png)

## En bref

- **Un seul espace de travail.** Le dossier choisi est partagé : le harness y
  démarre son serveur, OpenCode y ouvre sa session.
- **Le harness est embarqué.** `dsh web` est lancé sur un port libre ; l'URL
  tokenisée qu'il imprime est chargée dans le moteur Chromium de Qt. Pas
  d'onglet navigateur, pas de cookie à copier.
- **OpenCode est natif.** `opencode acp` est piloté via l'**Agent Client
  Protocol** : sessions, modèles, modes, appels d'outils, permissions et
  streaming sont rendus par l'application, pas par un terminal.
- **Vue partagée.** Les deux panneaux côte à côte (à partir de 1180 px de
  large), avec bascule instantanée et transcript conservé.
- **Clavier d'abord**, thème Omarchy suivi en direct (accent, arrondis,
  langue), interface française et anglaise.
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
| `Ctrl+1` | DeepSeek Harness |
| `Ctrl+2` | Chat OpenCode |
| `Ctrl+3` | Vue partagée (les deux panneaux) |
| `Ctrl+4` | Réglages |
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

## Comment ça marche

```text
┌──────────────────────── DeepCrasy H (Qt Quick) ─────────────────────────┐
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
  `dsh web: http://…/?token=…` et la transmet à la vue. Un redémarrage tue
  l'ancien processus et en démarre un neuf.
- `src/acpclient.*` parle JSON-RPC 2.0, délimité par des retours à la ligne,
  sur l'entrée/sortie standard d'`opencode acp`. Il gère `initialize`,
  `session/new`, `session/load`, `session/list`, `session/prompt`,
  `session/cancel`, `session/set_config_option` et les notifications
  `session/update` (texte, réflexion, outils, plan, usage, commandes).
  `session/request_permission` est rendu par une boîte de dialogue native.
- `src/backend.*` est la façade QML : un espace de travail, deux moteurs, un
  seul état. `src/config.*` persiste tout dans
  `~/.config/Crasy/deepcrasy-h/config.json` (écriture atomique).
- Le cookie du harness persiste dans
  `~/.local/share/Crasy/deepcrasy-h/QtWebEngine/deepcrasy-h`, mais l'URL
  tokenisée est rechargée à chaque démarrage : aucun secret n'est écrit dans
  la configuration.
- Aucun réseau sortant n'est ajouté par DeepCrasy H. Le harness écoute sur
  l'interface locale ; OpenCode reste un processus enfant.

`docs/ARCHITECTURE.md` détaille les décisions et les limites.

## Configuration

| Chemin | Contenu |
| --- | --- |
| `~/.config/Crasy/deepcrasy-h/config.json` | espace de travail, profil harness, binaires, dernière vue |
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
validation de la configuration, le thème, les catalogues de langue, et un
**vrai** `opencode acp` : démarrage, session, prompt, réponse streamée. Pour
sauter ce dernier (pas de réseau) :

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
```

## Limites connues

- La vue partagée n'a pas de séparateur déplaçable : 57/43, choisi pour que
  les deux panneaux restent utilisables.
- Le rendu Markdown s'appuie sur `Text.MarkdownText`. Les blocs de code sont
  lisibles, mais sans coloration syntaxique.
- Le harness peut afficher sa propre fenêtre d'accueil à la première
  utilisation ; c'est son interface, pas celle de DeepCrasy H.
- ACP est jeune : les mises à jour de session inconnues sont ignorées plutôt
  que devinées.

## Licence

MIT — voir [LICENSE](LICENSE). Le logo appartient à Crasy.

Dépôt : <https://github.com/iatexplique-debug/DeepCrasy-H>
