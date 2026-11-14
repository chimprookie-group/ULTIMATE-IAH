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


## Plugins de notes

Deux familles, configurables dans
`~/.config/Crasy/deepcrasy-h/note-plugins/` :

### Intégrés au harness (actifs par défaut)

- `task-extract` : transforme les cases `- [ ]` en notes-tâches `todo`.
- `task-sync` : passe en `done` les notes-tâches dont la case est cochée.
- `taskify` : crée une note-tâche à partir de la note courante.
- `section-tasks` : crée une note-tâche par section de niveau 2.
- `harness-brief` : ajoute workspace, modèle et tâches ouvertes.
- `frontmatter` : déduit `todo`/`done` depuis les cases à cocher.
- `toc` : insère ou met à jour un sommaire des titres.
- `link-audit` : liste les wikilinks non résolus.
- `skill-suggest` : propose les skills du harness pertinents.
- `memory-capture` : enregistre l'essentiel dans la mémoire du projet.

### Commandes externes

Déposez un manifeste JSON :

```json
{
  "id": "upper",
  "name": "Majuscules",
  "description": "Met le contenu en majuscules.",
  "command": "/home/moi/bin/upper.sh",
  "mode": "replace",
  "readOnly": false
}
```

Le script reçoit le chemin absolu de la note en `argv[1]`, le titre en
`argv[2]` et le Markdown sur `stdin`. Sa sortie standard est appliquée selon
`mode` : `append`, `replace` ou `create`. Aucun shell n'est utilisé, délai
max 8 s. La liste et l'exécution sont aussi exposées à l'agent via
`ultimate_note_plugins`.

Dans Notes, chaque plugin peut s'exécuter sur la note courante ou sur
**toutes les notes filtrées** (`Appliquer à N`). Les écritures sont
annulables et rétablissables (40 niveaux), et un plugin peut être planifié
(15 min, 1 h, 6 h, 24 h) sur toutes les notes.

Les plugins externes sont exécutés par défaut dans **bubblewrap** :
système de fichiers en lecture seule, vault monté en écriture, réseau coupé,
HOME temporaire. Un manifeste peut demander `"sandbox": false`, et les
tests le font pour rester portables.
