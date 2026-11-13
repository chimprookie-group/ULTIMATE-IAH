# Auto-apprentissage

`LearningEngine` transforme les trajectoires d’Ultimate Agent en connaissances
locales.

## Boucle

1. Chaque tour enregistre le prompt, le résultat, le modèle et les outils.
2. Un échec ou une séquence de deux outils réussie crée une leçon.
3. Une leçon répétée augmente sa confiance et son nombre d’utilisations.
4. Les leçons pertinentes sont injectées dans le contexte du tour suivant.
5. Une leçon devenue fiable crée une proposition de skill.
6. Le skill n’est écrit dans `~/.agents/skills` qu’après approbation.

## Persistance et sécurité

- SQLite `learning.db` dans le dossier de données de l’application ;
- secrets et tokens expurgés avant stockage ;
- plafond de 1 000 événements et 200 leçons ;
- aucun code, provider ou réglage système modifié automatiquement ;
- activation et suppression disponibles dans le panneau Apprentissage.

## Portée

La boucle est native et ne fait aucun appel externe pour apprendre. Elle complète
les skills, la mémoire FTS5, les missions, les bots et l’autopilote.
