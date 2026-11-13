---
name: ArtCraft
description: Piloter les 14 applications ArtCraft par CLI depuis le harness.
tags: [artcraft, design, cad, audio, vidéo, image]
automation: suggest
---
# ArtCraft

1. Vérifie le craft installé avec les outils `artcraft_<craft>`.
2. Commence par l'action `commands` pour découvrir le catalogue.
3. Utilise `run` avec des arguments CLI explicites.
4. Travaille dans le workspace et vérifie le fichier produit.
5. En mode read-only, seuls `commands` et `version` sont autorisés.
6. Ne lance pas une opération lourde sans limite de temps ni confirmation.

Crafts disponibles : CAD, Grid, Sound, Deck, Word, Photo, Light, Pdf, Vector,
Effect, Film, Design, Fig et Texture.
