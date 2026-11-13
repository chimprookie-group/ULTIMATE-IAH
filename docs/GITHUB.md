# Écosystème GitHub

`GitHubService` s’appuie sur le CLI `gh` authentifié. Le token reste dans le
keyring gh et n’est jamais lu par l’application.

## Opérations

- `github_status`
- `github_repos`
- `github_issues`
- `github_pulls`
- `github_clone` (workspace-write)
- `github_create_repo` (workspace-write)
- `github_create_issue` (workspace-write)
- `github_create_pull` (workspace-write)

## Vue GitHub

- Connexion, compte et scopes.
- Dépôts, filtre, langage, visibilité et étoiles.
- Détail du dépôt, issues et pull requests.
- Création d’un projet privé/public avec clonage.
- Clonage d’un dépôt existant.
- Création d’issue et de pull request.

## Sécurité

- Aucun token affiché ou journalisé.
- Noms de dépôts validés.
- Clonage limité au workspace.
- Les opérations d’écriture sont refusées en lecture seule.
