# Base neuronale native

`KnowledgeBase` est la carte persistante des capacités du harness.

## Contenu enregistré

- outils natifs Ultimate Agent ;
- outils de plugins, dont ArtCraft ;
- skills installés et packs Leanrea / DeepSeek Stack ;
- composants de l’écosystème (mémoire, missions, bots, GitHub, MCP, terminal,
  apprentissage, 4Crasy…).

## Fonctionnement

- SQLite `knowledge.db`, permissions `600` ;
- réenregistrement automatique au démarrage ;
- contexte compact injecté dans le prompt de l’agent ;
- recherche complète via `ultimate_knowledge action=search query=…` ;
- consultation depuis Apprentissage → Base neuronale native.

La base ne contient pas de secrets. Elle décrit uniquement les capacités
installées et leur fonctionnement.
