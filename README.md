# Crasy Harness

**Le cockpit DeepSeek Harness pour Omarchy.** Un seul agent, son terminal, ses
connexions et votre projet — sans OpenCode.

![Cockpit Crasy Harness](assets/screenshot-home.png)

![Mémoire persistante](assets/screenshot-memory.png)

![Autopilote](assets/screenshot-autopilot.png)

## Connexions

![Connexions du harness](assets/screenshot-connections.png)

Le centre **Connexions** gère les providers du harness directement : clé
DeepSeek, endpoints OpenAI-compatible, modèles et modèle par défaut. Les clés
vont dans le credential store DSH, les routes dans un overlay `llm-pi-ai`
consommé par le harness. 9Router et MausBot restent accessibles comme sources
et ponts MCP.

## Mémoire et autonomie

- **Mémoire persistante** : SQLite + FTS5, par projet, avec faits, décisions,
  préférences et résultats. Recherche plein texte, épinglage, intégrité et
  outils MCP (`memory_search`, `memory_remember`, `memory_recent`) pour le
  harness interactif.
- **Autopilote** : file de tâches bornée, exécutée une à une par `dsh headless`
  dans le workspace, avec mémoire injectée, timeout, reprise et journal. Le
  mode autonome est désactivé par défaut et s'active explicitement.
- **Tâches manuelles** : le terminal agent injecte aussi la mémoire pertinente.

## Le socle

- **Harness** : le web UI DeepSeek Harness est embarqué, avec son logo, son
  profil, son adresse locale et son cycle de vie.
- **Terminal agent** : un vrai PTY (bash, zsh, fish) avec xterm.js. Une barre
  au-dessus envoie une tâche au harness via `dsh headless "<tâche>"` dans le
  workspace courant.
- **Cockpit** : bloc de tâche harness, projet Git, services et raccourcis.
- **Projet / Fichiers** : lecture des fichiers, diff Git, commandes projet,
  contexte prêt à confier au harness.
- **Connexions** : 9Router, MausBot et le harness. 9Router peut exposer ses
  modèles au harness ; MausBot expose son équipe via MCP.

## Raccourcis

| Touche | Action |
|---|---|
| `Ctrl+0` | Cockpit |
| `Ctrl+1` | Harness |
| `Ctrl+2` | Terminal agent |
| `Ctrl+4` | Réglages |
| `Ctrl+6` | Projet |
| `Ctrl+7` | Fichiers |
| `Ctrl+9` | 9Router |
| `Ctrl+Shift+M` | MausBot |
| `Ctrl+K` | Palette de commandes |
| `Ctrl+R` | Redémarrer le harness |
| `?` | Aide |

## Installer

```sh
./bin/build
./bin/test
./bin/install-user
cd pkgbuild && makepkg -f
```

Le paquet Arch/Omarchy installe le binaire, le `.desktop`, l'icône et les
licences. Le harness (`dsh`) et le shell restent des dépendances système.

![Mémoire persistante](assets/screenshot-memory.png)

![Autopilote](assets/screenshot-autopilot.png)

## Connexions

- **Harness** : configurez le provider et le modèle dans le web UI embarqué
  (Réglages → Modèles) ou via 9Router → Appliquer au harness.
- **9Router** : dashboard local complet, 40+ providers, modèles exposés au
  harness.
- **MausBot** : dashboard local, pont MCP `dsh-mcp-client` vers le harness.
- **MCP** : les serveurs MCP du harness sont configurés via les overlays DSH.

## Règles d'installation

Voir [AGENTS.md](AGENTS.md) et [docs/DESIGN.md](docs/DESIGN.md) : nettoyage
après installation, vérification réelle, `pkexec` pour toute action
administrative, aucun mot de passe stocké.

## Licence

MIT.
