# Installation complète

## Paquet système

```bash
sudo pacman -U ultimate-hai-1.5.13-1-x86_64.pkg.tar.zst
```

Le paquet dépend de Qt 6, QtWebEngine, Node.js, npm, Python, PyYAML, Git et
xdg-desktop-portal.

## Runtime DSH

Ultimate IAH cherche DSH dans cet ordre :

1. `/usr/lib/crasy/dsh/node_modules/.bin/dsh` (runtime système du paquet crasy) ;
2. `dsh` dans le `PATH` ;
3. une installation mise de `@deepseek-ai/dsh` ;
4. `~/.local/bin/dsh`.

Si aucun runtime n'est présent, ouvrez **Installation** et utilisez
**Installer DSH** : le paquet npm est installé dans les données de
l'application, sans droit administrateur.

## Composants locaux

- **MCP mémoire** : généré automatiquement dans les données de l'application.
- **MCP personnalisés** : gérés depuis la vue MCP.

Les dépendances système utilisent exclusivement `pkexec pacman`; le mot de
passe polkit n'est jamais lu ni stocké.

## Vérification

**Installation** vérifie les composants actifs : DSH, Node, npm, Python, PyYAML,
QtWebEngine, Git, portail fichiers, dossiers de configuration/données, MCP
mémoire et entrées de bureau.

## Poste vierge

1. Installer le paquet Ultimate IAH.
2. Ouvrir **Installation**.
3. Lancer **Dépendances système** si nécessaire.
4. Installer DSH s'il est absent.
5. Ouvrir **Connexions**, saisir la clé DeepSeek une seule fois.
6. Le diagnostic doit afficher 0 élément manquant.

Les données utilisateur restent dans `~/.config/Crasy/deepcrasy-h` et
`~/.local/share/Crasy/deepcrasy-h`. Aucune migration destructive n'est faite.

Au premier lancement, l'application retire l'ancienne entrée utilisateur
`deepcrasy-h.desktop`, ses anciennes icônes et masque l'ancien lanceur
`crasy.desktop` (le paquet `crasy` reste installé car il fournit le runtime DSH
système). Seul le lanceur **Ultimate IAH** avec le nouveau logo reste visible.
