# Document de Conception - Quetes du Prologue

Ce document sert de fiche de remplissage pour Unreal Engine et ORION RPG. Il liste les quetes a creer, les noms d'assets conseilles, les tags a utiliser, les objectifs, les PNJ, les dialogues et les conditions de fin.

## 1. References Globales

| Element | Valeur |
| --- | --- |
| Village de depart | Valdorel |
| Amie kidnappee | Maelys |
| Antagonistes | Les Sans-Aube |
| Deuxieme zone | Gorges d'Ysbrume |
| Capacite debloquee | Grappin |

### Tags Cles

| Usage | Tag |
| --- | --- |
| Quetes ORION | `Quest.*` |
| Participants de dialogue ORION | `Dialog.Participant.*` |
| Faction antagoniste | `Faction.SansAube` |
| Capacite grappin | `Ability.GrapplingHook` |
| Item preuve du loup | `Item.WolfClaw` |
| Item narratif de Maelys | `Item.MaelysScarf` |

## 2. Tableau des Assets a Creer

### Quetes

| Type | Nom fichier conseille | Tag ORION | PNJ / acteur lie | Etat initial | Notes |
| --- | --- | --- | --- | --- | --- |
| Quest Blueprint | `BPQ_PRO_01_RetrouverSesEsprits` | `Quest.Prologue.RetrouverSesEsprits` | Spawn player, foulard de Maelys | Active | Quete automatique au premier spawn. |
| Quest Blueprint | `BPQ_PRO_02_PremierSang` | `Quest.Prologue.PremierSang` | `BP_Wolf`, clairiere du vieux chene | Locked | Activee quand le joueur entre dans le trigger de la clairiere. |
| Quest Blueprint | `BPQ_PRO_03_ArriveeValdorel` | `Quest.Prologue.ArriveeValdorel` | Aveline Ronce, porte de Valdorel | Locked | Premiere interaction avec le village. |
| Quest Blueprint | `BPQ_PRO_04_LeDoyen` | `Quest.Prologue.LeDoyen` | Orvan Lume, maison du doyen | Locked | Pose le lore des Sans-Aube et dirige vers Nalia. |
| Quest Blueprint | `BPQ_VIL_01_GrappinDeNalia` | `Quest.Village.GrappinDeNalia` | Nalia Forgebrume, atelier | Locked | Debloque le grappin de facon permanente. |
| Quest Blueprint | `BPQ_VIL_02_ChatSurLesToits` | `Quest.Village.ChatSurLesToits` | Mirette Solain, auberge, chat Pepin | Unlocked | Quete secondaire optionnelle apres obtention du grappin. |
| Quest Blueprint | `BPQ_PRO_05_RouteDesGorges` | `Quest.Prologue.RouteDesGorges` | Pont effondre, camp abandonne | Locked | Premiere vraie utilisation du grappin hors village. |
| Quest Blueprint | `BPQ_GOR_01_NuisiblesDEther` | `Quest.Gorges.NuisiblesDEther` | Soren Velt, wasps, nids | Locked | Introduit les monstres de la deuxieme zone. |

### Dialogues

| Type | Nom fichier conseille | Participants | Declencheur | Notes |
| --- | --- | --- | --- | --- |
| Dialog Blueprint | `BPD_Aveline_EntreeValdorel` | Player, Aveline | Objectif "Parler a Aveline" | Confirme que les Sans-Aube sont connus. |
| Dialog Blueprint | `BPD_Orvan_LeDoyen` | Player, Orvan | Objectif "Interroger Orvan" | Explique pourquoi Maelys a ete enlevee. |
| Dialog Blueprint | `BPD_Nalia_Grappin` | Player, Nalia | Objectif "Parler a Nalia" | Introduit le grappin et ses composants. |
| Dialog Blueprint | `BPD_Mirette_Chat` | Player, Mirette | Quete secondaire du chat | Ton plus leger, tutoriel grappin optionnel. |
| Dialog Blueprint | `BPD_Soren_Gorges` | Player, Soren | Rencontre dans les gorges | Donne la piste vers les ruines hautes. |

## 3. Fiches de Quetes

### Q_PRO_01 - Le foulard de Maelys

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le foulard de Maelys |
| Nom asset conseille | `BPQ_PRO_01_RetrouverSesEsprits` |
| Quest Tag | `Quest.Prologue.RetrouverSesEsprits` |
| Etat ORION initial | Active |
| Declencheur | Premier spawn du joueur apres la cinematique du kidnapping. |
| But de gameplay | Donner une direction claire, poser l'urgence narrative et guider vers la premiere menace. |

Objectifs dans l'ordre :

1. Ramasser le foulard de Maelys.
2. Suivre les traces vers le sentier.
3. Atteindre la clairiere du vieux chene.

Dialogues :

| Participant | Ligne |
| --- | --- |
| Player | "Maelys... non. Ils l'ont emmenee vers les hauteurs." |
| Player | "Le village est tout proche. Quelqu'un a Valdorel saura quoi faire." |

Conditions de succes :

- Le joueur atteint le trigger de la clairiere du vieux chene.

Recompenses :

- Ajout ou validation de `Item.MaelysScarf`.
- Activation de `Quest.Prologue.PremierSang`.

Quete suivante :

- Activer `Quest.Prologue.PremierSang`.

### Q_PRO_02 - Le loup du sentier

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le loup du sentier |
| Nom asset conseille | `BPQ_PRO_02_PremierSang` |
| Quest Tag | `Quest.Prologue.PremierSang` |
| Etat ORION initial | Locked |
| Declencheur | Entree dans le trigger de la clairiere et spawn du loup. |
| But de gameplay | Tutoriel combat, esquive, ciblage et recuperation d'un objet de preuve. |

Objectifs dans l'ordre :

1. Survivre a l'attaque.
2. Vaincre le loup.
3. Recuperer une griffe de loup.
4. Rejoindre Valdorel.

Dialogues :

| Participant | Ligne |
| --- | --- |
| Player | "Pas maintenant... recule !" |
| Player | "Ce n'etait pas un loup ordinaire. Ses yeux brillaient." |

Conditions de succes :

- Le loup est mort.
- Le joueur recupere `Item.WolfClaw`.
- Le joueur atteint l'entree de Valdorel.

Recompenses :

- Ajout de `Item.WolfClaw`.
- Activation de `Quest.Prologue.ArriveeValdorel`.

Quete suivante :

- Activer `Quest.Prologue.ArriveeValdorel`.

### Q_PRO_03 - Les portes de Valdorel

| Champ | Valeur |
| --- | --- |
| Nom joueur | Les portes de Valdorel |
| Nom asset conseille | `BPQ_PRO_03_ArriveeValdorel` |
| Quest Tag | `Quest.Prologue.ArriveeValdorel` |
| Etat ORION initial | Locked |
| Declencheur | Le joueur arrive a la porte du village apres l'attaque du loup. |
| PNJ principal | Aveline Ronce |
| But de gameplay | Introduire le village, le premier PNJ et l'existence des Sans-Aube. |

Objectifs dans l'ordre :

1. Parler a Aveline a l'entree.
2. Montrer la griffe de loup.
3. Parler au doyen Orvan.

Dialogues `BPD_Aveline_EntreeValdorel` :

| Participant | Ligne |
| --- | --- |
| Aveline | "Halte. Tu viens du sentier nord ? Personne ne passe par la apres le coucher du soleil." |
| Player | "Ils ont enleve Maelys. Des silhouettes marquees d'un demi-soleil brise." |
| Aveline | "Les Sans-Aube... Alors les anciens avaient raison." |
| Aveline | "Va voir le doyen Orvan. Et garde cette griffe, elle l'interessera." |

Conditions de succes :

- Dialogue avec Aveline termine.
- Objectif de destination vers Orvan active.

Recompenses :

- Acces narratif au village.
- Activation de `Quest.Prologue.LeDoyen`.

Quete suivante :

- Activer `Quest.Prologue.LeDoyen`.

### Q_PRO_04 - Le nom des ravisseurs

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le nom des ravisseurs |
| Nom asset conseille | `BPQ_PRO_04_LeDoyen` |
| Quest Tag | `Quest.Prologue.LeDoyen` |
| Etat ORION initial | Locked |
| Declencheur | Le joueur entre dans la maison du doyen ou interagit avec Orvan. |
| PNJ principal | Orvan Lume |
| But de gameplay | Poser le lore, expliquer l'enlevement et orienter vers le grappin. |

Objectifs dans l'ordre :

1. Interroger Orvan.
2. Examiner la carte du village.
3. Parler a Nalia, l'artisane.

Dialogues `BPD_Orvan_LeDoyen` :

| Participant | Ligne |
| --- | --- |
| Orvan | "Les Sans-Aube ne prennent jamais quelqu'un au hasard." |
| Player | "Pourquoi Maelys ?" |
| Orvan | "Parce qu'elle porte peut-etre une trace d'Ether. Comme toi." |
| Orvan | "Ils sont passes par les Gorges d'Ysbrume. A pied, tu n'y survivras pas. Nalia peut t'aider." |

Conditions de succes :

- Dialogue avec Orvan termine.
- Carte examinee.
- Objectif vers Nalia active.

Recompenses :

- Debloque la piste des Gorges d'Ysbrume.
- Unlock de `Quest.Village.GrappinDeNalia`.

Quete suivante :

- Unlock ou activer `Quest.Village.GrappinDeNalia`.

### Q_VIL_01 - Le grappin de Nalia

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le grappin de Nalia |
| Nom asset conseille | `BPQ_VIL_01_GrappinDeNalia` |
| Quest Tag | `Quest.Village.GrappinDeNalia` |
| Etat ORION initial | Locked |
| Declencheur | Le joueur parle a Nalia apres la quete du doyen. |
| PNJ principal | Nalia Forgebrume |
| But de gameplay | Debloquer le grappin via une mini-quete de recolte, exploration et test. |

Objectifs dans l'ordre :

1. Parler a Nalia dans l'atelier.
2. Recuperer 3 fibres de ronce solide.
3. Recuperer 1 ressort ancien dans la remise.
4. Tester le grappin sur la poutre d'entrainement.
5. Revenir voir Nalia.

Dialogues `BPD_Nalia_Grappin` :

| Participant | Ligne |
| --- | --- |
| Nalia | "Un passage par les gorges ? Tu veux mourir vite ou juste avec panache ?" |
| Player | "Je dois retrouver Maelys." |
| Nalia | "Alors il te faut mon grappin. Mais il manque de quoi refaire la ligne." |
| Nalia | "Bien. Ne vise pas les nuages, vise les anneaux marques. Le reste viendra avec les bleus." |

Conditions de succes :

- Les composants sont recuperes.
- Le test de grappin sur la poutre est valide.
- Dialogue final avec Nalia termine.

Recompenses :

- Debloque `Ability.GrapplingHook`.
- Unlock de `Quest.Village.ChatSurLesToits`.
- Activation de `Quest.Prologue.RouteDesGorges`.

Quete suivante :

- Activer `Quest.Prologue.RouteDesGorges`.
- Unlock `Quest.Village.ChatSurLesToits` comme contenu optionnel.

### Q_VIL_02 - Le chat de Mirette

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le chat de Mirette |
| Nom asset conseille | `BPQ_VIL_02_ChatSurLesToits` |
| Quest Tag | `Quest.Village.ChatSurLesToits` |
| Etat ORION initial | Unlocked |
| Declencheur | Disponible apres obtention du grappin. |
| PNJ principal | Mirette Solain |
| Acteur lie | Chat Pepin |
| But de gameplay | Quete secondaire legere pour pratiquer le grappin en securite dans le village. |

Objectifs dans l'ordre :

1. Parler a Mirette.
2. Reperer Pepin sur les toits.
3. Utiliser le grappin pour atteindre le toit.
4. Ramener Pepin a Mirette.

Dialogues `BPD_Mirette_Chat` :

| Participant | Ligne |
| --- | --- |
| Mirette | "Pepin a encore grimpe la-haut. Il descend seulement quand il veut humilier quelqu'un." |
| Player | "Je vais essayer de le recuperer." |
| Mirette | "Essaie de ne pas casser mes tuiles. Elles coutent plus cher que ce chat." |

Conditions de succes :

- Le joueur interagit avec Pepin.
- Le joueur retourne parler a Mirette.

Recompenses :

- Petite monnaie, consommable ou ration.
- Aucun blocage de progression principale.

Quete suivante :

- Aucune quete obligatoire. Cette quete reste optionnelle.

### Q_PRO_05 - Vers les Gorges d'Ysbrume

| Champ | Valeur |
| --- | --- |
| Nom joueur | Vers les Gorges d'Ysbrume |
| Nom asset conseille | `BPQ_PRO_05_RouteDesGorges` |
| Quest Tag | `Quest.Prologue.RouteDesGorges` |
| Etat ORION initial | Locked |
| Declencheur | Completion de `Quest.Village.GrappinDeNalia`. |
| But de gameplay | Faire sortir le joueur du village et valider l'usage du grappin en condition reelle. |

Objectifs dans l'ordre :

1. Atteindre le pont effondre.
2. Traverser avec le grappin.
3. Trouver le camp abandonne.
4. Examiner les traces des Sans-Aube.

Dialogues :

| Participant | Ligne |
| --- | --- |
| Player | "Ces marques... ils ont traine quelque chose." |
| Player | "Maelys est vivante. Il faut continuer." |

Conditions de succes :

- Le joueur traverse le pont effondre.
- Le camp abandonne est decouvert.
- Les traces des Sans-Aube sont examinees.

Recompenses :

- Activation de la premiere quete des gorges.

Quete suivante :

- Activer `Quest.Gorges.NuisiblesDEther`.

### Q_GOR_01 - Nids dans la brume

| Champ | Valeur |
| --- | --- |
| Nom joueur | Nids dans la brume |
| Nom asset conseille | `BPQ_GOR_01_NuisiblesDEther` |
| Quest Tag | `Quest.Gorges.NuisiblesDEther` |
| Etat ORION initial | Locked |
| Declencheur | Rencontre avec Soren pres du camp abandonne. |
| PNJ principal | Soren Velt |
| But de gameplay | Introduire les monstres de la deuxieme zone et varier les objectifs avec combat plus destruction d'objets. |

Objectifs dans l'ordre :

1. Parler a Soren.
2. Eliminer 4 guepes d'Ether.
3. Detruire 2 nids.
4. Retourner voir Soren.

Dialogues `BPD_Soren_Gorges` :

| Participant | Ligne |
| --- | --- |
| Soren | "Je les ai suivis jusqu'aux gorges. Puis la brume s'est mise a bourdonner." |
| Player | "Tu as vu une fille avec eux ?" |
| Soren | "Oui. Vivante. Ils l'emmenaient vers les ruines hautes." |
| Soren | "Nettoie les nids, et je te montrerai le raccourci." |

Conditions de succes :

- 4 ennemis de type guepe sont vaincus.
- 2 nids sont detruits.
- Dialogue de retour avec Soren termine.

Recompenses :

- Debloque un raccourci vers la suite.
- Donne l'indice narratif : les Sans-Aube vont vers les ruines hautes.

Quete suivante :

- Suite a definir : premiere quete des ruines hautes.

## 4. Participants de Dialogue

| Personnage | Tag ORION | Role | Placement conseille |
| --- | --- | --- | --- |
| Joueur | `Dialog.Participant.Player` | Protagoniste | Participant special joueur. |
| Aveline Ronce | `Dialog.Participant.Aveline` | Garde de Valdorel | Porte principale du village. |
| Orvan Lume | `Dialog.Participant.Orvan` | Doyen | Maison du doyen ou place centrale. |
| Nalia Forgebrume | `Dialog.Participant.Nalia` | Artisane du grappin | Atelier de forge/bricolage. |
| Mirette Solain | `Dialog.Participant.Mirette` | Aubergiste | Auberge de Valdorel. |
| Soren Velt | `Dialog.Participant.Soren` | Eclaireur blesse | Camp abandonne des Gorges d'Ysbrume. |

## 5. Notes d'Implementation ORION

- Les quetes principales du prologue doivent s'enchainer avec des events `ActivateQuest` ou `UnlockQuest`.
- Les PNJ doivent avoir un tag participant identique a celui configure dans le Dialog Builder.
- Le grappin reste verrouille jusqu'a la completion de `Quest.Village.GrappinDeNalia`.
- La quete du chat est optionnelle et ne doit pas bloquer la progression principale.
- La deuxieme zone doit etre bloquee tant que le grappin n'est pas debloque.
- Les noms techniques doivent rester sans accents pour eviter les soucis Unreal/Git.
- Les noms affiches au joueur peuvent utiliser les accents dans les champs `QuestName`, `Description` et les lignes de dialogue.

## Test Plan

- Verifier que tous les tags apparaissent dans les dropdowns Unreal.
- Creer chaque BP Quest avec le bon `QuestTag`.
- Tester l'enchainement spawn -> loup -> village -> doyen -> Nalia -> grappin -> gorges.
- Verifier que les dialogues n'apparaissent qu'au bon etat de quete.
- Verifier que les objectifs ORION s'affichent dans l'ordre prevu.
- Verifier que la quete `Quest.Village.ChatSurLesToits` reste optionnelle.
- Verifier que `Ability.GrapplingHook` est indisponible avant la fin de `Quest.Village.GrappinDeNalia`.

## Assumptions

- Le document est en Markdown dans le repo.
- Les noms de fichiers utilisent les prefixes `BPQ_` pour les quetes et `BPD_` pour les dialogues.
- Les tags Unreal restent sans accents et sans espaces.
- Les Sans-Aube remplacent definitivement l'ancien nom des antagonistes.
