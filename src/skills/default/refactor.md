---
name: Refactor
description: Simplifier sans changer le comportement, avec preuves avant/après.
tags: [refactor, dette, qualité, simplification]
automation: suggest
---
# Refactor

1. Établis le comportement actuel et les tests qui le couvrent.
2. Supprime une cause de complexité à la fois.
3. Préfère les types, invariants et fonctions courtes aux commentaires.
4. Ne mélange pas refactor et nouvelle fonctionnalité.
5. Relance les tests après chaque étape.
6. Mesure le gain : lignes, branches, duplication, temps de lecture.

Si le comportement n'est pas couvert, ajoute d'abord le test.
