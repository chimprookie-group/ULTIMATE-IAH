---
name: Debug
description: Reproduire, isoler et corriger une cause racine, pas un symptôme.
tags: [debug, bug, erreur, diagnostic]
automation: suggest
---
# Debug

1. Reproduis le problème avec la commande ou l'action la plus petite possible.
2. Lis le message d'erreur complet et le code réel.
3. Formule une hypothèse causale, puis une expérience qui la réfute.
4. Isole la variable : entrée, état, concurrence, configuration, version.
5. Corrige la cause partagée une seule fois.
6. Vérifie le cas initial, un cas voisin et l'absence de régression.
7. Si deux correctifs échouent, arrête les variantes et change d'hypothèse.

Ne masque pas une erreur avec un `try/catch` silencieux.
