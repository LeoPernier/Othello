# Solveur Othello : étude de cas de programmation parallèle

## Introduction

Avant tout, je vais présenter le problème. En effet, l'objectif est de déterminer, pour un solveur d’Othello, si et quand le parallélisme (OpenMP) vaut réellement le coup face au séquentiel. Pour en dire davantage, à partir de quelle profondeur de recherche le gain de temps dépasse les surcoûts tel que la création et synchronisation de threads, le déséquilibre de charge et la perte de localité. Bref, jusqu’où l’accélération reste-t-elle soutenable et quelles limites empêchent l'accélération de prendre place.

Ce rapport a donc pour objectif d'évaluer l’apport du parallélisme sur mon solveur Othello. Bref, pour ce travail j'ai utilisé un algorithme **alpha-bêta** de base et fait varier la profondeur de recherche de 1 à 8 (car plus ferait exploser le temps d’exécution) afin de collecter des statistiques comparables. On observera donc des résultats sur 40 parties (5 par profondeur) ainsi qu'une analyse de ces résultats.

---

## Résultats et analyse

> Note: Pour référence, les statistiques sont sous le répertoire **test_results** (J'ai conscience que ce n'est pas le nom le plus charmeur haha). Un total de 40 fichiers **.csv** y seront accessible et parviennent de l'outil **AutoCompare**. À note que le nom de chaque fichier a une signification, ils sont sous format **N.M** ou **N** représente la profondeur et **M** représente le numéro de la partie, il y aura donc 5 fichiers par profondeur pour un total de 40 fichiers.

### Grilles par profondeur (5 parties x 8 profondeurs)

![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N1.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N2.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N3.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N4.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N5.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N6.jpg)

> Note: Il est en effet normal que ce dernier graphique soit aussi petit. Pour tout dire, le solveur a battu le joueur très vite, les données ont tout de meme été utilisees dans ce rapport.

![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N7.jpg)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/grille_row_N8.jpg)

> Note: Ces 40 graphiques sont générés par ChatGPT a l'aide de données récoltées par mes tests et outils (Accessible dans ce travail si vous désirez generer vos propres stats).

En bref, sur nos 40 parties, l’accélération progresse nettement avec la profondeur jusqu’à un palier autour de **N = 3 a 7**. À **N = 1**, plus de **60%** des coups sont ralentis (< 1) et la moyenne (tous les coups) reste < 1. À **N = 2**, on obtient enfin un gain net (approx. **x1.98**) mais approx. **36%** des coups restent défavorables. À partir de **N = 3**, le tout se stabilise: les accélérations moyennes se situent entre **x2.4** et **x2.77** et la proportion de coups < 1 tombe vers approx. **8 a 11%**. On observe ensuite une légère régression à **N = 8** (moyenne approx. **x2.43**; < 1 approx. **10.56%**), montrant que le rendement décroissant.

Les profondeurs 1 et 2 ne valent donc pas la peine: la création/synchronisation de threads, le déséquilibre de charge et la perte de localité coûtent plus que ce qu’ils rapportent, d’où de nombreux coups < 1 et un bénéfice global nul (**N = 1**) ou marginal (**N = 2**). Au contraire, a partir de **N = 3**, le parallélisme commence vraiment à payer car le travail par coup est suffisant afin d'amortir l’overhead. Pourtant, plus la profondeur augmente, plus le coût d’exploration explose (tel un arbre exponentiel): au-delà d'approx. **N = 6 ou 7**, le gain stagne alors que le temps d’exécution se décuple, ce qui n’est pas rentable pour notre alpha-bêta de base.

---

### Accélération par profondeur

| Profondeur | Moyenne (tous coups) | Moyenne (coups > 1) | Coups < 1 | Coups > 1 | Coups totaux | % < 1  |
|:----------:|:--------------------:|:-------------------:|:---------:|:---------:|:------------:|:------:|
| 1          | 0.84                 | 1.716               | 94        | 56        | 150          | 62.667 |
| 2          | 1.98                 | 2.914               | 54        | 94        | 148          | 36.486 |
| 3          | 2.774                | 3.006               | 14        | 139       | 153          | 9.15   |
| 4          | 2.458                | 2.661               | 14        | 139       | 153          | 9.15   |
| 5          | 2.405                | 2.618               | 15        | 146       | 161          | 9.317  |
| 6          | 2.601                | 2.825               | 12        | 122       | 134          | 8.955  |
| 7          | 2.651                | 2.852               | 13        | 147       | 160          | 8.125  |
| 8          | 2.426                | 2.677               | 17        | 144       | 161          | 10.559 |

![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/pourcentage_speedup_inferieur_1.png)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/acceleration_moyennes_par_profondeur.png)
![Si vous ne pouvez pas voir les images, elles sont toutes accessibles dans le répertoire "graphiques"!](/graphiques/repartition_coups_par_profondeur.png)

> Note: Ces graphiques sont générés par chatGPT.

Globalement, **N = 3** offre la meilleure moyenne "tous les coups" (x2.774) et une fraction < 1 d’environ **9.15%**. La moyenne "coups > 1" culmine en plus à **N = 3** (x3.006). Les profondeurs 6 et 7 minimisent la proportion de coups défavorables (approx. 8 à 9 %). À **N = 8**, on observe un légepetitr recul: la moyenne retombe à **x2.426** et la part < 1 remonte à **10.559%**. Les profondeurs 3 à 7 constituent donc le meilleur compromis entre gain et régularité.

---

Le visuel que nous avons par coup met en évidence l’impact de l’overhead lorsque la profondeur est basse (peu de travail utile par thread et synchronisations fréquentes) et l’importance de l’équilibrage de charge. À mesure que la profondeur augmente, le volume de travail par coup grandit et amortit mieux les surcoûts, d’où la forte baisse des coups < 1. Le plateau au-delà de **N approx. 3 à 5** s’explique par des limites de localité mémoire, la contention des ressources (caches) et des sections non parallélisables (Amdahl). Enfin, la légère régression à **N = 8** illustre des rendements décroissants typiques: donc plus de travail total, mais pas plus d’accélération utile.

---

## Conclusion

Pour conclure, le seuil utile est atteint dès **N = 3**. En effet, vers **N = 6 et 7**, l’accélération reste correcte mais la facture en temps d’exécution grimpe beaucoup. En effet, **N = 8** nous montre des signes de saturation. Pour pousser la barre un petit peu, il vaux mieux répartir la charge et réduire les synchronisations ainsi que de soigner la localité (ordonnancement des tâches et tailles de blocs).
