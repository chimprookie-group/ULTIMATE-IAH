# Règles de travail — DeepCrasy H

Ces règles s'appliquent à toutes les sessions et aux installateurs de l'application.

## Installations

- Après chaque installation, supprimer uniquement les fichiers et dossiers temporaires créés pour cette installation et devenus inutiles. Ne jamais supprimer les données utilisateur, la configuration, les caches partagés ni les fichiers d'exécution nécessaires.
- Après chaque installation, refaire un contrôle réel : binaire présent et exécutable, version, démarrage/santé ou configuration. Rapporter explicitement ce qui a été vérifié et ce qui a été nettoyé.
- Toute action administrateur passe par `pkexec`, pour que le mot de passe soit saisi dans la fenêtre polkit. Ne jamais demander, lire, stocker ou journaliser ce mot de passe. Utiliser des arguments fixes, sans shell injectable.

## Application

- Les dépendances système passent par `pkexec pacman` et sont vérifiées après l'installation.
- Toute nouvelle action privilégiée doit réutiliser ce chemin `pkexec`, jamais `sudo` directement.

## Méthode UI — une seule correction propre

- Avant toute correction visuelle, reproduire le défaut sur une capture réelle.
- Identifier la cause structurelle commune avant de modifier plusieurs fichiers.
- Ne pas empiler les petits correctifs au hasard : une modification de composant partagé, puis une vérification réelle.
- Après correction, refaire la même capture au même endroit et au même thème, et vérifier les états survol, focus et fenêtre compacte.
- Une animation de survol ne doit jamais modifier la géométrie visible d'une icône : privilégier couleur/opacité et garder la zone strictement dans la cellule de l'icône.
