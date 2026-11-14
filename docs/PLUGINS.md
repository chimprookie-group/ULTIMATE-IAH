# Plugins et ArtCraft

Ultimate Agent reçoit ses outils via le registre natif `PluginManager`.

## Socle DSH

Le registre expose 29 capacités du noyau actives par défaut et 12 extensions
expérimentales désactivées, avec leur outil natif ou leur vue. Cette carte est
la projection native du profil DSH `web` et du bundle `base`.

## ArtCraft

- Binaires détectés dans `~/.local/share/Crasy/deepcrasy-h/artcraft/bin`,
  `~/.dsh/crafts/bin`, `~/.local/bin` ou le `PATH`.
- 14 craft exposés comme `artcraft_<craft>`, plus tout binaire `*-cli`
  inconnu détecté dans le dossier ArtCraft (adaptation automatique aux
  nouveaux crafts de la suite).
- La suite est gérée dans
  `~/.local/share/Crasy/deepcrasy-h/artcraft/suite` : « Compléter » installe
  les crafts manquants, « Mettre à jour » fait un `git pull` puis réinstalle
  avec `--force`.
- Les actions agent `ultimate_plugins action=artcraft_install|artcraft_update`
  déclenchent la même gestion.
- `commands`, `version` et `run` passent par `QProcess`, sans shell.
- Le mode `read-only` n’autorise que `commands` et `version`.

## Plugins externes

Déposez un manifeste dans
`~/.config/Crasy/deepcrasy-h/plugins/manifests/*.json` :

```json
{
  "id": "my-tool",
  "name": "My Tool",
  "description": "A command exposed to the agent",
  "command": "/usr/bin/my-tool",
  "args": ["--help"]
}
```

L’outil devient `plugin_my-tool`. L’activation est persistée dans
`plugins/settings.json`.

## Harness

Ultimate Agent construit `tools()` à partir de ses outils intégrés et de ceux du
registre. Les outils de plugins sont donc réellement appelables dans la boucle
agent, pas seulement listés dans l’interface.
