# Règles de travail — DeepCrasy H

Ces règles s'appliquent à toutes les sessions et aux installateurs de l'application.

## Installations

- Après chaque installation, supprimer uniquement les fichiers et dossiers temporaires créés pour cette installation et devenus inutiles. Ne jamais supprimer les données utilisateur, la configuration, les caches partagés ni les fichiers d'exécution nécessaires.
- Après chaque installation, refaire un contrôle réel : binaire présent et exécutable, version, démarrage/santé ou configuration. Rapporter explicitement ce qui a été vérifié et ce qui a été nettoyé.
- Toute action administrateur passe par `pkexec`, pour que le mot de passe soit saisi dans la fenêtre polkit. Ne jamais demander, lire, stocker ou journaliser ce mot de passe. Utiliser des arguments fixes, sans shell injectable.

## Application

- `NineRouterManager` vérifie 9Router après installation, nettoie ses caches temporaires, puis le démarre.
- Les dépendances système passent par `pkexec pacman` et sont vérifiées après l'installation.
- Toute nouvelle action privilégiée doit réutiliser ce chemin `pkexec`, jamais `sudo` directement.
