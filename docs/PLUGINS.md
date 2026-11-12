# Plugins et ArtCraft

Ultimate Agent reçoit ses outils via le registre natif `PluginManager`.

## ArtCraft

- Binaires détectés dans `~/.local/share/Crasy/deepcrasy-h/artcraft/bin`,
  `~/.dsh/crafts/bin`, `~/.local/bin` ou le `PATH`.
- 14 craft exposés comme `artcraft_<craft>`.
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
