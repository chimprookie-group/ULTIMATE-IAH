# Bibliothèque de skills

Ultimate IAH installe 12 skills dans `~/.agents/skills` au premier démarrage.
Le marqueur `.defaults-v1` évite de réécrire la bibliothèque à chaque lancement.
Le bouton **Bibliothèque par défaut** force la restauration des skills manquants.

## Skills fournis

- `research` — recherche web, sources et citations ;
- `code-review` — revue de code et régressions ;
- `test-first` — boucle TDD ;
- `debug` — cause racine ;
- `git-workflow` — commits et branches ;
- `github-project` — dépôts, issues et PR ;
- `notes-vault` — notes et graphe ;
- `security` — audit permissions et secrets ;
- `release` — build, paquet, installation ;
- `automation` — bots et planifications ;
- `refactor` — simplification sûre ;
- `artcraft` — suite créative.

## Sélection automatique

Quand aucun skill n'est activé manuellement, Ultimate Agent compare le prompt
aux noms, descriptions et tags des skills, puis injecte les deux meilleurs dans
le prompt système. L'agent peut aussi lire un skill avec
`ultimate_skills` action `read`.

## Format

Chaque skill est un `SKILL.md` avec frontmatter `name`, `description`, `tags`.
Les skills projet sont cherchés dans `<workspace>/.agents/skills`.
