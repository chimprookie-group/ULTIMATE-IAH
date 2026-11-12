# Plan — connexions Ultimate HAI

Objectif : un seul chemin fiable pour chaque connexion, du fournisseur jusqu'au terminal et aux agents, avec diagnostic et preuve pour chaque maillon.

## Étapes

1. **Audit du chemin réel**
   - Lire HarnessServer, HarnessConnections, NineRouterManager, MausBotManager, McpManager, TerminalSession et Backend.
   - Vérifier les patches réellement passés à DSH et les variables d'environnement.
   - Check : `ps` montre les patches attendus ; un test `dsh headless` répond.

2. **Unifier les providers**
   - 9Router écrit son provider dans `connections.patch.yml` en fusionnant les providers existants, plus `agent.env` via credentials DSH.
   - Retirer `nine-router.patch.yml` du chargement HarnessServer ; migrer/supprimer l'ancien fichier.
   - Check : apply 9Router + provider natif dans le même overlay ; `dsh --dump-config` montre les deux ; suppression de 9Router ne casse pas le provider restant.

3. **Corriger les liens externes 9Router**
   - Gérer `onNewWindowRequested` et `onNavigationRequested` : loopback reste dans la WebEngine, externe s'ouvre dans le navigateur.
   - Check : une page HTML de test avec `target=_blank` ouvre le navigateur et ne quitte pas le dashboard ; le dashboard reste sur son URL.

4. **Corriger les liens et le pairing MausBot**
   - Mêmes handlers externes, sans interrompre le flux `/pair`.
   - Check : le pairing reste dans la WebEngine, un lien externe part au navigateur.

5. **Diagnostic unifié**
   - Nouveau gestionnaire `ConnectionDiagnostics` : DSH, harness, env, credentials, provider par défaut, 9Router (santé/clé/modèles), MausBot (santé/pairing/MCP), MCP mémoire/custom, conflits de patches.
   - UI dans Connexions, refresh + test harness.
   - Check : tests unitaires sur overlay/credentials ; diagnostic réel de la machine.

6. **Validation réelle**
   - Démarrer l'app, 9Router, MausBot, MCP ; cliquer/valider les liens ; tester un provider ; tester MCP.
   - Check : captures, logs, `dsh headless` OK.

7. **Tests, docs, paquet**
   - Tests, README/docs, version, paquet, installation système, nettoyage.
   - Check : `./bin/test`, `pacman -Q`, app relancée.

## Évidence

1. Audit fait : DSH patché avec `openmausbot.patch.yml`, `connections.patch.yml`, `mcp-memory.patch.yml` ; 9Router répond sur 20128 ; MausBot répond sur 8799 ; `dsh headless` répond.
2. Overlay unifié : `connections.patch.yml` contient `deploy-portal` + `nine-router`, défaut conservé ; `NINE_ROUTER_API_KEY` dans `agent.env` ; ancien `nine-router.patch.yml` absent. Test helper : fusion et suppression valides.
3. Liens 9Router : handler `onNewWindowRequested` + `onNavigationRequested` ; la source 9Router confirme `target="_blank"` pour `Get API Key`.
4. MausBot : mêmes handlers, loopback gardé dans la WebEngine, pairing `/pair` préservé.
5. Diagnostic : 17 contrôles, 17 sains après session 9Router + provider fusionné. UI Connexions.
6. Validation : session 9Router adoptée depuis le cookie WebEngine, 36 modèles chargés, DSH compose les providers. Une requête réelle sur un modèle upstream DeepSeek a renvoyé l'erreur de clé upstream 9Router, ce qui est un réglage 9Router et non une rupture de liaison.
7. Tests 29/29, paquet `ultimate-hai 1.2.0-1` installé, app relancée.
