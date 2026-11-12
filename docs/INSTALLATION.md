# Installation complète

## Paquet système

```bash
sudo pacman -U ultimate-hai-1.4.10-1-x86_64.pkg.tar.zst
```

Le paquet dépend de Qt 6, QtWebEngine, Node.js, npm, Python, PyYAML, Git et
xdg-desktop-portal.

## Runtime DSH

Ultimate HAI cherche DSH dans cet ordre :

1. `/usr/lib/crasy/dsh/node_modules/.bin/dsh` (runtime système du paquet crasy) ;
2. `dsh` dans le `PATH` ;
3. une installation mise de `@deepseek-ai/dsh` ;
4. `~/.local/bin/dsh`.

Si aucun runtime n'est présent, ouvrez **Installation** et utilisez
**Installer DSH** : le paquet npm est installé dans les données de
l'application, sans droit administrateur.

## Services locaux

- **9Router** : installation npm locale depuis **Installation → Installer
  9Router**, puis démarrage. Santé attendue sur `127.0.0.1:20128/api/health`.
- **MausBot** : installation npm locale depuis **Installation → Installer
  MausBot**, puis démarrage. Santé attendue sur `127.0.0.1:8799/api/health`.
- **MCP mémoire** : généré automatiquement dans les données de l'application.

Les dépendances système utilisent exclusivement `pkexec pacman`; le mot de
passe polkit n'est jamais lu ni stocké.

## Vérification

**Installation** vérifie 18 composants : DSH, Node, npm, Python, PyYAML,
QtWebEngine, Git, portail fichiers, installations et services 9Router/MausBot,
dossiers de configuration/données, MCP mémoire et entrée de bureau.

## Poste vierge

1. Installer le paquet Ultimate HAI.
2. Ouvrir **Installation**.
3. Lancer **Dépendances système** si nécessaire.
4. Installer DSH, 9Router et MausBot si absents.
5. Démarrer les services.
6. Ouvrir **Connexions**, saisir la clé DeepSeek une seule fois.
7. Le diagnostic doit afficher 0 élément manquant.

Les données utilisateur restent dans `~/.config/Crasy/deepcrasy-h` et
`~/.local/share/Crasy/deepcrasy-h`. Aucune migration destructive n'est faite.

Au premier lancement, l'application retire l'ancienne entrée utilisateur
`deepcrasy-h.desktop`, ses anciennes icônes et masque l'ancien lanceur
`crasy.desktop` (le paquet `crasy` reste installé car il fournit le runtime DSH
système). Seul le lanceur **Ultimate HAI** avec le nouveau logo reste visible.
