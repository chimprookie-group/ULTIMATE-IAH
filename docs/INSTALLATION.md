# Installation

## En une commande

```sh
curl -fsSL https://github.com/chimprookie-group/ULTIMATE-IAH/releases/latest/download/install.sh | bash
```

Le script vérifie les dépendances, télécharge le paquet avec un checksum
SHA256 obligatoire, installe via `pkexec`, initialise les skills/4Crasy et
exécute `doctor`.

## Paquet système

```bash
pkexec pacman -U ultimate-iah-1.25.1-1-x86_64.pkg.tar.zst
```

Le paquet dépend de Qt 6, QtWebEngine, Node.js, npm, Python, PyYAML, Git,
GitHub CLI et xdg-desktop-portal.

## Runtime Ultimate Agent

Ultimate Agent est embarqué dans le paquet. Aucun runtime DSH externe, aucun
serveur web séparé. Les jobs agent utilisent le binaire installé avec
`--agent-job`.

## Composants locaux

- **MCP mémoire** : généré automatiquement dans les données de l'application.
- **MCP personnalisés** : gérés depuis la vue MCP.

Les dépendances système utilisent exclusivement `pkexec pacman`; le mot de
passe polkit n'est jamais lu ni stocké.

## Vérification

**Installation** vérifie les composants actifs : moteur agent, Node, npm, Python,
PyYAML, QtWebEngine, Git, portail fichiers, dossiers de configuration/données,
MCP mémoire, registre des bots, pont MCP bots, vault Notes et entrées de bureau.

## Poste vierge

1. Installer le paquet Ultimate IAH.
2. Ouvrir **Installation**.
3. Lancer **Dépendances système** si nécessaire.
4. Installer Ultimate Agent s'il est absent.
5. Ouvrir **Providers**, saisir la clé DeepSeek une seule fois.
6. Le diagnostic doit afficher 0 élément manquant.

Les données utilisateur restent dans `~/.config/Crasy/deepcrasy-h` et
`~/.local/share/Crasy/deepcrasy-h`. Aucune migration destructive n'est faite.

Au premier lancement, l'application retire les anciennes entrées utilisateur
`deepcrasy-h.desktop` et `ultimate-hai.desktop`, leurs icônes, puis masque
l'ancien lanceur système `crasy.desktop` (le paquet historique reste installé
car il fournit le moteur agent système). Seul le lanceur **Ultimate IAH** avec
le nouveau logo reste visible.
