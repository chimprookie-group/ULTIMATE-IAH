---
name: Release
description: Construire, tester, empaqueter, installer et vérifier une release.
tags: [release, package, build, arch]
automation: suggest
---
# Release

1. Vérifie l'état Git et les versions.
2. Build propre et suite de tests complète.
3. Mets à jour la version, le paquet et la documentation.
4. Construis le paquet avec `makepkg`.
5. Installe uniquement via `pkexec pacman`.
6. Vérifie le binaire, la version, le lanceur, les icônes et les données.
7. Lance l'application et contrôle les erreurs QML.
8. Commit et push avec un message factuel.

Ne supprime jamais les données utilisateur pendant une release.
