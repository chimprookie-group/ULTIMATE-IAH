---
name: Code Review
description: Revue de code orientée régressions, sécurité et preuves.
tags: [review, code, sécurité, git]
automation: suggest
---
# Code Review

## Procédure

1. Lis le diff réel et le contexte des fichiers touchés.
2. Cherche d'abord les bugs, régressions, pertes de données et failles.
3. Vérifie les chemins d'erreur, les permissions, les timeouts et les entrées
   non fiables.
4. Vérifie les tests existants et propose le test minimal qui manque.
5. Classe les constats : bloquant, important, mineur.
6. Donne un correctif exact avec fichier et ligne.

## Règles

- Ne signale pas un style subjectif comme un bug.
- Ne prétends pas avoir testé sans sortie réelle.
- Ne modifie pas le code pendant une revue demandée en lecture seule.
