---
name: GitHub Project
description: Connecter, créer et gérer dépôts, issues et pull requests.
tags: [github, repo, issue, pr, projet]
automation: suggest
---
# GitHub Project

## Outils

`github_status`, `github_repos`, `github_issues`, `github_pulls`,
`github_clone`, `github_create_repo`, `github_create_issue`.

## Procédure

1. Vérifie la connexion avec `github_status`.
2. Pour créer un projet, valide le nom et la visibilité avant l'action.
3. Clone dans le workspace uniquement si demandé.
4. Pour une issue, fournis un titre court et un corps reproductible.
5. Ne lis jamais un token et ne le colle jamais dans une commande.
6. Les écritures GitHub nécessitent le mode workspace-write.

## Sortie

Donne l'URL du dépôt ou de l'issue et l'état réel de l'opération.
