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

`tests/deepcrasy-h_tests.cpp` couvre la logique pure (URL, configuration,
thème, langues) et un véritable aller-retour ACP : `opencode acp` est lancé,
une session créée, un prompt envoyé, la réponse streamée vérifiée. Ce test
peut être sauté avec `DEEPCRASY_SKIP_INTEGRATION=1`.

## Pistes

- Séparateur déplaçable en vue partagée.
- Coloration syntaxique des blocs de code.
- Onglets de sessions OpenCode dans le panneau.
- Notification système quand un long tour se termine.
