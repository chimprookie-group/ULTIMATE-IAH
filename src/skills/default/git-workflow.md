---
name: Git Workflow
description: Branches, diffs, commits atomiques et pull requests propres.
tags: [git, commit, branche, pr]
automation: suggest
---
# Git Workflow

1. Inspecte `git status`, la branche et le diff avant toute action.
2. Ne mélange pas plusieurs chantiers dans un commit.
3. Commit atomique avec un message qui explique le pourquoi.
4. Vérifie `git diff --check` avant de committer.
5. Ne force-push jamais une branche partagée sans demande explicite.
6. Pour une PR, résume le changement, les tests exécutés et les risques.
7. Ne commit jamais de secret, de token ou de fichier de configuration privé.
