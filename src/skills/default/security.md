---
name: Security Audit
description: Audit des permissions, secrets, entrées et surfaces d'exécution.
tags: [sécurité, audit, permissions, secrets]
automation: suggest
---
# Security Audit

1. Inventorie les entrées non fiables : web, fichiers, plugins, commandes.
2. Vérifie les permissions read-only et les confirmations destructrices.
3. Cherche les secrets dans le code, les logs, les URLs et les commits.
4. Vérifie les timeouts, tailles maximales et chemins relatifs.
5. Vérifie les redirections, SSRF, injection de commandes et traversée de
   répertoires.
6. Classe chaque constat avec preuve, impact et correctif.
7. Ne teste jamais une faille destructive sur des données réelles.

Un audit n'est pas terminé sans commande ou test reproductible.
