# Direction visuelle — Ultimate IAH

## Note

**Ultimate IAH est un instrument de mission, pas un dashboard SaaS.** Il est
conçu pour un développeur Omarchy qui pilote des agents, des connexions et un
workspace. Le ton est technique, calme et dense. L'élément mémorable est le
rail de navigation à spine, doublé d'un en-tête de commandement qui rend
l'état du harness lisible en un regard. Mode : opérer.

## Tokens

Les tokens vivent dans `designtokens.h` et sont exposés à QML sous `design`.

- Espacement : 4, 8, 12, 16, 20, 24, 32, 40, 48.
- Rayons : 6 (contrôles), 10 (panneaux), 16 (grandes surfaces), pill.
- Typographie : `iA Writer Duospace` pour le display, Omarchy pour le corps,
  `backend.monoFont` pour les données et les labels.
- Couleurs : uniquement la palette Omarchy active. L'accent sert aux actions et
  au vivant ; cyan/vert/ambre/rouge aux états. Aucun dégradé décoratif.
- Icônes : un seul jeu SVG, viewBox 24, trait 1.5, bouts arrondis.
- Logo : le cavalier et sa baleine, détourés sur fond transparent. Variante
  blanche pour les thèmes sombres, variante encre pour les thèmes clairs. Icône
  applicative sur tuile encre avec un trait d'accent.
- Largeurs : rail 236 / 68, en-tête 60, statut 30, contenu max 1180.

## Structure

- Rail : marque, workspace, groupes Opérer / Intelligence / Runtime / Réseau,
  commandes et réglages. Le rail se replie en icônes avec un indicateur actif.
- En-tête : repli du rail, workspace, fil d'Ariane, palette de commandes,
  état du harness, aide.
- Pages : `PageHeader` commun (icône, titre display, description courte,
  actions), puis des panneaux à hairline.
- Barre de statut : chemin, branche, état des services.

## États

Chaque action a hover, focus visible, disabled et retour. Les vues ont un état
vide, une erreur et un défilement sans scroll horizontal. Les modales utilisent
le même fond en encre et une seule bordure.

## Densité

Les écrans sont compacts mais respirants : 1180 px de contenu maximal, deux
colonnes asymétriques sur le cockpit, une colonne à 760 px. Le seuil de largeur
minimale est 760 px pour cette application de bureau.
