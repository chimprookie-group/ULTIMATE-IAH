# Ultimate Agent

Ultimate Agent est l'opérateur intégré d'Ultimate IAH. Il ne répond pas seulement
sur l'application : il utilise les mêmes objets C++ que l'interface pour lire et
modifier l'écosystème réel.

## Accès

- **Ctrl+Alt+U** ouvre/ferme le dock agent à droite.
- L'entrée **Ultimate Agent** de la barre latérale ouvre la vue complète.
- L'icône étincelle de l'en-tête ouvre le dock.
- Le dock reste ouvert pendant que l'agent navigue vers une autre vue.

## Discussions et images

- Chaque discussion est indexée dans `~/.local/share/Crasy/deepcrasy-h/agent-conversations.db`.
- La vue large affiche le rail **Discussions** ; le dock et les fenêtres compactes
  utilisent le bouton historique.
- Recherche, nouvelle discussion, renommage, épinglage, suppression confirmée
  et copie du transcript sont disponibles.
- Les images sont copiées dans `agent-attachments/`, affichées dans le fil,
  agrandissables et transmises en `image_url` aux modèles compatibles vision.
- Quatre images maximum par message, 12 Mo par image.

## Modes et compétences

- **Assistant**, **Diagnostic**, **Construction** et **Revue** modifient le prompt système.
- Les skills installés peuvent être activés dans **Options → Compétences** ;
  leur contenu est injecté dans le prompt de la conversation.

## Connaissance

Le prompt système est reconstruit à chaque tour avec :

- version, workspace, Git et vue active ;
- état du harness, binaire DSH et URL ;
- providers, modèle par défaut et état de la clé DeepSeek ;
- serveurs MCP personnalisés ;
- statistiques mémoire, skills, profils d'agents, état des bots et discussion active ;
- raccourcis, chemins de données et règles de sécurité.

## Outils

| Outil | Usage |
|---|---|
| `ultimate_status` | Snapshot live complet |
| `ultimate_diagnose` | Diagnostic connexions + installation |
| `ultimate_navigate` | Ouvrir une vue |
| `ultimate_connections` | Providers, clé DeepSeek, défaut, redémarrage |
| `ultimate_services` | Cycle de vie du harness |
| `ultimate_bots` | Lister, créer, exécuter, planifier et supprimer les bots |
| `ultimate_mcp` | Serveurs MCP |
| `ultimate_memory` | Rechercher/enregistrer la mémoire |
| `ultimate_skills` | Lister/lire/créer/supprimer un skill |
| `ultimate_mission` | Missions Planner → Executor → Reviewer |
| `ultimate_terminal` | Commande shell réelle, confirmée |
| `ultimate_files` | Lister/lire/écrire dans le workspace |
| `ultimate_git` | status, diff, log |
| `ultimate_install` | Audit et installations |
| `ultimate_settings` | Thème, langue, démarrage |
| `ultimate_agents` | Profils de rôles |

## Bots

Ultimate Agent peut inspecter le service de bots, déléguer une tâche, créer un
bot, planifier une exécution ou arrêter l’API. Les actions destructrices
(suppression, arrêt du service) demandent `confirm: true`.

## Sécurité

- Les clés ne sont jamais renvoyées par un outil.
- Les opérations destructrices exigent `confirm: true`.
- Le terminal et les écritures de fichiers sont confinés au workspace actif.
- Le prompt interdit de prétendre qu'une action a réussi sans résultat d'outil.
- L'historique de conversation est stocké dans les données de configuration de
  l'application ; les secrets ne doivent pas être collés dans le chat.

## Endpoint

L'agent utilise la clé `DEEPSEEK_API_KEY` du credential store DSH avec
`deepseek-chat`. Sans clé, il affiche une erreur et demande une configuration
dans Providers.

## Persistance

L'historique est conservé dans
`~/.config/Crasy/deepcrasy-h/ultimate-agent.json`. **Nouvelle conversation**
efface l'historique et le fichier est réécrit avec des permissions privées.
