# Direction visuelle — DeepCrasy H

**But** : un poste de pilotage pour développeur qui fait tourner plusieurs agents locaux.
**Public** : utilisateur Omarchy qui veut voir, lancer et connecter ses agents sans perdre le fil.
**Ton** : instrument de précision, calme, dense, éditorial — pas un dashboard SaaS.
**Mémorable** : la colonne « Services » vivante à droite du cockpit, chaque agent avec son état,
son modèle et une action immédiate.
**Mode** : opérer. La densité est volontaire ; les espaces respirent par groupes, pas par cartes isolées.

## Tokens

- **Affichage** : `iA Writer Duospace` pour les titres et chiffres clés ; corps : police Omarchy
  (`backend.uiFont`) ; données : `backend.monoFont`.
- **Couleurs** : toutes dérivées du thème Omarchy actif (`theme.background`, `surface`,
  `raised`, `border`, `text`, `mutedText`, `accent`). Les couleurs sémantiques
  `green`, `yellow`, `red` servent uniquement aux états.
- **Espacement** : base 4 px, groupes 8/12/16/24.
- **Rayons** : 8 px pour les contrôles, 12 px pour les panneaux ; pas de rayon sur les séparateurs.
- **Ombre** : aucune sur les panneaux ; une seule ombre réservée aux overlays (palette, dialogues).
- **Mouvement** : une révélation orchestrée de 180 ms à l'ouverture du cockpit ; feedback de survol 100 ms.
- **Interdits** : gradients décoratifs, glassmorphism, emoji-icônes, cartes identiques en grille,
  texte gris sur gris, majuscules en eyebrow partout.

## Structure

1. **Barre latérale groupée** : Atelier / Agents / Intégrations + réglages. Se replie sous 1080 px.
2. **En-tête** : titre de vue, espace de travail, recherche-commande, split, aide.
3. **Cockpit** : un bloc de commande large, puis projet et sessions à gauche, services à droite.
4. **Barre d'état** : branche, quatre services, modèle courant.

Les vues spécialisées (Écosystème, Fichiers, Providers, 9Router, MausBot, Réglages) gardent leur contenu
et sont atteignables depuis la barre latérale et la palette.
