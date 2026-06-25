# Prologue Dialogues - ORION Dialog Builder

Ce document liste les dialogues du prologue dans l'ordre de jeu. Les tags, objectifs et descriptions de quetes sont dans `PrologueQuests.md`.

## Participants ORION

| Personnage | Tag |
| --- | --- |
| Joueur | `Dialog.Participant.Player` |
| Aveline Ronce | `Dialog.Participant.Aveline` |
| Orvan Lume | `Dialog.Participant.Orvan` |
| Nalia Forgebrume | `Dialog.Participant.Nalia` |
| Mirette Solain | `Dialog.Participant.Mirette` |
| Soren Velt | `Dialog.Participant.Soren` |

## BPD_PRO_01_RetrouverSesEsprits

Quete : `Quest.Prologue.RetrouverSesEsprits`

Type : monologue joueur.

### Debut de quete

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Player | "Maelys..." |
| 2 | Player | "Ils l'ont emmenee. Je les ai vus disparaitre vers les hauteurs." |
| 3 | Player | "Je dois prevenir quelqu'un. Le village est tout proche." |

### Apres `Go To Location`

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Player | "Attends... c'est son foulard." |
| 2 | Player | "Elle l'avait encore quand ils l'ont attrapee." |

### Apres `Collect Scarf`

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Player | "Maelys, tiens bon." |
| 2 | Player | "Je vais trouver de l'aide a Valdorel. Puis je reviendrai sur leurs traces." |

Choix : aucun.

## BPD_PRO_02_LoupDuSentier

Quete : `Quest.Prologue.PremierSang`

Type : barks gameplay.

| Moment | Participant | Ligne |
| --- | --- | --- |
| Apparition du loup | Player | "Qu'est-ce que..." |
| Apparition du loup | Player | "Recule ! Je n'ai pas le temps pour ca !" |
| Premier degat recu | Player | "Il est plus rapide qu'il en a l'air..." |
| Loup vaincu | Player | "Ce n'etait pas normal." |
| Loup vaincu | Player | "Ses yeux... cette lueur. Comme une trace d'Ether." |
| Loot de griffe | Player | "Une griffe noircie." |
| Loot de griffe | Player | "Quelqu'un au village saura peut-etre ce que ca signifie." |

Choix : aucun.

## BPD_Aveline_EntreeValdorel

Quete : `Quest.Prologue.ArriveeValdorel`

Participants : Player, Aveline.

### Intro commune

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Aveline | "Halte. Tu viens du sentier nord ?" |
| 2 | Aveline | "Personne ne passe par la apres le coucher du soleil." |
| 3 | Player | "Je n'avais pas le choix. Mon amie a ete enlevee." |
| 4 | Aveline | "Enlevee ? Par qui ?" |

### Choix joueur

| Choix | Condition | Texte joueur |
| --- | --- | --- |
| A | Toujours disponible | "Des silhouettes masquees." |
| B | Toujours disponible | "Je n'ai pas bien vu." |
| C | Disponible si la griffe a ete ramassee | "J'ai trouve ca sur le chemin." |

### Branche A

| # | Participant | Ligne |
| --- | --- | --- |
| 5A | Player | "Des silhouettes masquees. Elles portaient un symbole sombre, comme un soleil brise." |
| 6A | Aveline | "Un soleil brise..." |

### Branche B

| # | Participant | Ligne |
| --- | --- | --- |
| 5B | Player | "Je n'ai pas bien vu. Tout est alle trop vite." |
| 6B | Aveline | "Calme-toi. Reprends depuis le debut. Un detail suffit parfois." |

### Branche C

| # | Participant | Ligne |
| --- | --- | --- |
| 5C | Player | "J'ai trouve ca apres l'attaque." |
| 6C | Aveline | "Une griffe... noire au bord. Ce n'est pas une blessure naturelle." |

### Retour commun

| # | Participant | Ligne |
| --- | --- | --- |
| 7 | Player | "Elle s'appelle Maelys. Ils l'ont trainee vers les hauteurs." |
| 8 | Aveline | "Alors ecoute-moi bien." |
| 9 | Aveline | "Les anciens appellent ces gens les Sans-Aube." |
| 10 | Player | "Tu les connais ?" |
| 11 | Aveline | "Pas assez. Et c'est justement ca qui m'inquiete." |
| 12 | Aveline | "Va voir Orvan, le doyen. Dis-lui que je t'envoie, et montre-lui la griffe." |
| 13 | Aveline | "S'il reste une chance de retrouver ton amie, elle commence chez lui." |

ORION : `Player Choice` -> branches -> `Reroute` commun -> fin.

## BPD_Orvan_LeDoyen

Quete : `Quest.Prologue.LeDoyen`

Participants : Player, Orvan.

### Intro commune

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Orvan | "Tu n'es pas d'ici." |
| 2 | Orvan | "Et pourtant Aveline t'a laisse entrer. C'est rarement bon signe." |
| 3 | Player | "Elle m'a dit de venir vous voir." |
| 4 | Orvan | "Alors parle. Lentement." |
| 5 | Player | "J'etais avec Maelys pres du sentier nord." |
| 6 | Player | "Des gens nous ont surpris. Ils ne cherchaient pas a voler. Ils voulaient elle." |
| 7 | Player | "J'ai essaye de la suivre, mais j'ai ete attaque avant d'atteindre le village." |
| 8 | Player | "Aveline a reconnu le symbole d'un soleil brise." |
| 9 | Player | "Et j'ai trouve ca apres le combat." |
| 10 | Orvan | "Donne." |
| 11 | Orvan | "Noircie par l'Ether... et fraiche." |
| 12 | Orvan | "Ce loup n'etait pas malade. Il a ete marque." |

### Choix joueur

| Choix | Condition | Texte joueur |
| --- | --- | --- |
| A | Toujours disponible | "Qui sont les Sans-Aube ?" |
| B | Toujours disponible | "Pourquoi Maelys ?" |
| C | Toujours disponible | "Comment je la retrouve ?" |

### Branche A

| # | Participant | Ligne |
| --- | --- | --- |
| 13A | Player | "Qui sont les Sans-Aube ?" |
| 14A | Orvan | "Un ordre qui prefere les ruines aux vivants." |
| 15A | Orvan | "Ils cherchent les porteurs d'Ether, ceux dont le sang reagit aux anciennes pierres." |

### Branche B

| # | Participant | Ligne |
| --- | --- | --- |
| 13B | Player | "Pourquoi Maelys ?" |
| 14B | Orvan | "Parce qu'elle a quelque chose qu'ils veulent." |
| 15B | Orvan | "Une trace d'Ether. Peut-etre faible. Peut-etre dormante. Mais assez pour les attirer." |

### Branche C

| # | Participant | Ligne |
| --- | --- | --- |
| 13C | Player | "Comment je la retrouve ?" |
| 14C | Orvan | "En cessant de courir droit vers la mort." |
| 15C | Orvan | "Ils ont pris les Gorges d'Ysbrume. Le chemin est brise depuis des annees." |

### Retour commun

| # | Participant | Ligne |
| --- | --- | --- |
| 16 | Player | "S'ils ont pris les Gorges d'Ysbrume, je peux encore les rattraper." |
| 17 | Orvan | "Tu peux essayer. Mais pas comme ca." |
| 18 | Player | "Qu'est-ce que ca veut dire ?" |
| 19 | Orvan | "Les gorges ne sont pas une route. Ce sont des falaises, des ponts brises et des courants que meme les oiseaux evitent." |
| 20 | Orvan | "Les Sans-Aube ont du passer par la, oui. Mais ils connaissent des chemins que Valdorel a oublies." |
| 21 | Player | "Alors donnez-moi un autre passage." |
| 22 | Orvan | "Il n'y en a pas. Pas pour quelqu'un sans equipement." |
| 23 | Orvan | "Nalia Forgebrume travaille sur un grappin. Un outil instable, bruyant, dangereux... donc probablement parfait pour toi." |
| 24 | Player | "Un grappin suffira ?" |
| 25 | Orvan | "Pour atteindre les premieres corniches, oui." |
| 26 | Orvan | "Pour traverser les hauteurs d'Ysbrume... non." |
| 27 | Player | "Alors quoi ?" |
| 28 | Orvan | "Il existe d'anciens recits. Des voyageurs capables de se laisser porter par l'Ether, comme si le vent reconnaissait leur nom." |
| 29 | Player | "Des recits ?" |
| 30 | Orvan | "Des avertissements, plutot." |
| 31 | Orvan | "Si quelque chose en toi repond a ces gorges, ne le force pas. Ce genre de pouvoir ne s'arrache pas. Il s'eveille." |
| 32 | Player | "Je n'ai pas le temps d'attendre qu'un pouvoir decide de m'aider." |
| 33 | Orvan | "Alors commence par ce que tu peux tenir dans tes mains." |
| 34 | Orvan | "Va voir Nalia. Dis-lui que les Sans-Aube ont repris la route d'Ysbrume." |
| 35 | Orvan | "Et dis-lui que si Valdorel detourne encore les yeux, les ruines nous prendront quelqu'un d'autre." |

ORION : les 3 choix peuvent etre des choix de lore repetables avant de rejoindre le retour commun, ou un choix unique si tu veux garder le rythme rapide.

## BPD_Nalia_Grappin

Quete : `Quest.Village.GrappinDeNalia`

Participants : Player, Nalia.

### Intro commune

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Nalia | "Si c'est Orvan qui t'envoie, j'ai deja mal a la tete." |
| 2 | Player | "Il dit que vous pouvez m'aider a traverser les Gorges d'Ysbrume." |
| 3 | Nalia | "Il a dit ca comme si c'etait raisonnable ?" |
| 4 | Player | "Maelys a ete enlevee par les Sans-Aube." |
| 5 | Nalia | "..." |
| 6 | Nalia | "Bon. La, ca devient raisonnable." |

### Choix joueur

| Choix | Condition | Texte joueur |
| --- | --- | --- |
| A | Toujours disponible | "J'ai besoin de votre grappin." |
| B | Toujours disponible | "Je peux payer." |
| C | Toujours disponible | "Je n'ai pas de temps a perdre." |

### Branches

| Choix | Participant | Ligne |
| --- | --- | --- |
| A | Player | "J'ai besoin de votre grappin." |
| A | Nalia | "Tout le monde a besoin de mon grappin jusqu'au moment ou il faut l'utiliser au-dessus du vide." |
| B | Player | "Je peux payer." |
| B | Nalia | "Avec quoi ? De la panique et une griffe de loup ? Garde ton argent." |
| C | Player | "Je n'ai pas de temps a perdre." |
| C | Nalia | "Alors evite d'en perdre en tombant dans une gorge." |

### Retour commun

| # | Participant | Ligne |
| --- | --- | --- |
| 9 | Nalia | "Le mecanisme fonctionne, mais la ligne est fichue." |
| 10 | Nalia | "Il me faut trois fibres de ronce solide et un ressort ancien." |
| 11 | Player | "Ou je trouve ca ?" |
| 12 | Nalia | "Les ronces poussent derriere l'atelier. Le ressort est dans ma remise." |
| 13 | Nalia | "Et si quelque chose bouge dans la remise, ne frappe pas. C'est probablement une etagere. Probablement." |

### Apres collecte des composants

| # | Participant | Ligne |
| --- | --- | --- |
| 14 | Player | "J'ai ce qu'il faut." |
| 15 | Nalia | "Pose ca la. Et garde tes doigts loin du crochet." |
| 16 | Nalia | "Voila. Le grappin est repare." |
| 17 | Player | "Je peux partir ?" |
| 18 | Nalia | "Non. Maintenant tu vas apprendre a ne pas te briser contre le premier mur venu." |

Fin de cette quete : activer `Quest.Village.EssayerLeGrappin`.

## BPD_Nalia_EssayerLeGrappin

Quete : `Quest.Village.EssayerLeGrappin`

Participants : Player, Nalia.

### Avant le test

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Nalia | "Regarde l'ancre au-dessus de la poutre." |
| 2 | Nalia | "Ne vise pas les nuages. Vise les anneaux marques." |
| 3 | Player | "Et si je rate ?" |
| 4 | Nalia | "Alors tu recommences. Ici, le sol est assez proche pour pardonner." |

### Apres test reussi

| # | Participant | Ligne |
| --- | --- | --- |
| 5 | Nalia | "Pas mal." |
| 6 | Nalia | "Le grappin t'emmenera la ou ton bras ne peut pas. Pas la ou ton cerveau refuse d'aller." |
| 7 | Player | "Merci, Nalia." |
| 8 | Nalia | "Ramene Maelys. Et si tu croises les Sans-Aube... regarde bien leur symbole." |

## BPD_Mirette_Chat

Quete : `Quest.Village.ChatSurLesToits`

Participants : Player, Mirette.

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Mirette | "Toi. Tu as l'air assez desespere pour aider une aubergiste." |
| 2 | Player | "Ca depend du probleme." |
| 3 | Mirette | "Il s'appelle Pepin." |
| 4 | Player | "Pepin ?" |
| 5 | Mirette | "Mon chat. Il est sur le toit, et il me juge depuis vingt minutes." |

Choix :

| Choix | Condition | Texte joueur | Reponse Mirette |
| --- | --- | --- | --- |
| A | Toujours disponible | "Je vais le recuperer." | "Enfin quelqu'un d'utile." |
| B | Toujours disponible | "Je suis presse." | "Tout le monde est presse jusqu'a ce qu'un chat bloque l'acces au garde-manger." |
| C | Toujours disponible | "Il mord ?" | "Seulement les gens qu'il estime. Donc probablement." |

Retour commun :

| # | Participant | Ligne |
| --- | --- | --- |
| 8 | Mirette | "Utilise ton grappin, mais evite mes tuiles." |
| 9 | Mirette | "Elles coutent plus cher que ce chat. Et le chat le sait." |

Recuperation de Pepin :

| # | Participant | Ligne |
| --- | --- | --- |
| 10 | Player | "Viens la, Pepin." |
| 11 | Player | "Je comprends pourquoi elle t'a laisse la-haut si longtemps." |

Retour a Mirette :

| # | Participant | Ligne |
| --- | --- | --- |
| 12 | Mirette | "Pepin ! Mon petit tyran." |
| 13 | Player | "Il n'a pas rendu ca facile." |
| 14 | Mirette | "S'il l'avait fait, ce ne serait pas Pepin." |
| 15 | Mirette | "Tiens. Pour toi. Et pour les tuiles que tu n'as presque pas cassees." |

## BPD_PRO_05_RouteDesGorges

Quete : `Quest.Prologue.RouteDesGorges`

Type : monologue joueur.

| Moment | Participant | Ligne |
| --- | --- | --- |
| Depart du village | Player | "Les Gorges d'Ysbrume..." |
| Depart du village | Player | "Si Orvan a raison, les Sans-Aube sont deja loin." |
| Depart du village | Player | "Mais ils ont laisse des traces." |
| Pont effondre | Player | "Voila pourquoi Nalia insistait sur le grappin." |
| Pont effondre | Player | "Impossible de traverser autrement." |
| Apres traversee | Player | "Ca marche." |
| Apres traversee | Player | "Brutal, mais ca marche." |
| Camp abandonne | Player | "Un camp." |
| Camp abandonne | Player | "Le feu est froid, mais les traces sont recentes." |
| Traces | Player | "Des marques de bottes. Plusieurs personnes." |
| Traces | Player | "Et ici... quelque chose a ete traine." |
| Traces | Player | "Maelys est passee par la." |
| Traces | Player | "Je te retrouve." |

## BPD_Soren_Gorges

Quete : `Quest.Gorges.NuisiblesDEther`

Participants : Player, Soren.

### Intro commune

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Soren | "N'approche pas trop vite." |
| 2 | Player | "Tu es blesse." |
| 3 | Soren | "Bonne observation. Mauvais timing." |
| 4 | Player | "Je cherche une fille enlevee par les Sans-Aube." |
| 5 | Soren | "Alors tu cours dans la meme mauvaise direction que moi." |

Choix :

| Choix | Condition | Texte joueur | Reponse Soren |
| --- | --- | --- | --- |
| A | Toujours disponible | "Tu les as vus ?" | "Oui. Trois silhouettes. Une captive. Vivante." |
| B | Toujours disponible | "Qui es-tu ?" | "Soren Velt. Eclaireur de Valdorel quand Valdorel se souvient qu'elle a besoin d'eclaireurs." |
| C | Toujours disponible | "Qu'est-ce qui t'a attaque ?" | "La brume. Ou plutot ce qui vit dedans. Des guepes d'Ether." |

Retour commun :

| # | Participant | Ligne |
| --- | --- | --- |
| 8 | Soren | "Elles gardent les nids autour du passage." |
| 9 | Soren | "Si tu veux suivre les Sans-Aube, il faut nettoyer la zone." |
| 10 | Player | "Combien ?" |
| 11 | Soren | "Quatre guepes devraient suffire a ouvrir la route." |
| 12 | Soren | "Et detruis deux nids. Sinon elles reviendront avant toi." |

Apres objectifs termines :

| # | Participant | Ligne |
| --- | --- | --- |
| 13 | Player | "Les nids sont detruits." |
| 14 | Soren | "Alors ecoute bien." |
| 15 | Soren | "Les Sans-Aube n'ont pas pris la route basse." |
| 16 | Soren | "Ils montent vers les ruines hautes." |
| 17 | Player | "Avec Maelys ?" |
| 18 | Soren | "Oui." |
| 19 | Soren | "Elle marchait encore quand je les ai vus. Faible, mais debout." |
| 20 | Player | "Alors je continue." |
| 21 | Soren | "Prends le raccourci derriere les roches. Et ne suis pas les chants dans la brume." |
| 22 | Player | "Les chants ?" |
| 23 | Soren | "Si tu les entends, c'est deja qu'ils t'ont entendu." |

## Future - Eveil du glide

Quete future conseillee : `Quest.Ruins.EveilDuVent`

Le grappin permet d'atteindre les premieres corniches. Le glide doit arriver plus tard comme un pouvoir d'Ether qui s'eveille dans une chute, une chambre ancienne ou une epreuve de ruines.

| # | Participant | Ligne |
| --- | --- | --- |
| 1 | Player | "Non... pas maintenant." |
| 2 | Player | "Le vent... il ralentit ma chute ?" |
| 3 | Player | "Qu'est-ce qui m'arrive ?" |

## Notes Dialog Builder

- Monologue : `Root -> DialogLine -> DialogLine -> End`.
- Dialogue a choix : `Root -> Intro -> PlayerChoice -> Branches -> Reroute commun -> Fin`.
- Utiliser des decorators pour cacher les choix lies a un item ou un etat de quete.
- Les dialogues qui terminent un objectif doivent appeler la progression apres la derniere ligne, pas au debut.
