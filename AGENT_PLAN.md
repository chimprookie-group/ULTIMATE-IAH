# Ultimate Agent — plan de chantier

Objectif : un agent intégré, connaissant l'état réel d'Ultimate HAI, capable d'agir sur l'écosystème via des outils, accessible depuis la barre latérale.

## Étapes et contrôles

1. **Moteur agent** (`UltimateAgent`) : endpoint DeepSeek/9Router, boucle tool-calling, historique persistant, annulation.
   - Contrôle : build Qt, requête réelle avec `ultimate_status`, réponse finale.
2. **Outils système** : statut, diagnostic, navigation, connexions/providers, 9Router, MausBot, MCP, mémoire, skills, missions, terminal, fichiers, Git, installation, réglages.
   - Contrôle : chaque outil retourne un JSON `ok`, aucun secret dans les résultats.
3. **Connaissance live** : prompt système construit depuis l'état réel (harness, services, providers, skills, mémoire, MCP, workspace, installation).
   - Contrôle : le prompt contient les états et les chemins réels.
4. **UI** : vue `agent`, chat Markdown, appels d'outils, état vide, composeur, raccourcis, entrée sidebar `Ultimate Agent`.
   - Contrôle : smoke QML 1440/760, FR/EN, aucun warning QML.
5. **Intégration** : contexte main, routing, command palette, aide, réglages de démarrage, traductions.
   - Contrôle : navigation clavier et commande palette.
6. **Validation réelle** : lancer l'agent avec un prompt de diagnostic, vérifier tool calls et réponse.
   - Contrôle : réponse visible et état du système inchangé ou action demandée.
7. **Tests/docs/paquet/installation** : tests de schéma/prompt, doc, paquet, installation, relance.
   - Contrôle : tests passent, paquet installé, vue agent accessible.
