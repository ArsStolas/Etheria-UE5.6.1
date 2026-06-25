# Document de Conception - Quetes du Prologue

Ce document sert de fiche de remplissage pour Unreal Engine et ORION RPG. Il liste les quetes a creer, les noms d'assets conseilles, les tags a utiliser, les objectifs, les PNJ et les conditions de fin.

Les dialogues complets, choix et conditions de choix sont dans `PrologueDialogues.md`.

## 1. References Globales

| Element | Valeur |
| --- | --- |
| Village de depart | Valdorel |
| Amie kidnappee | Maelys |
| Antagonistes | Les Sans-Aube |
| Deuxieme zone | Gorges d'Ysbrume |
| Capacite debloquee | Grappin |
| Pouvoir tease | Glide / pouvoir d'Ether lie au vent |

### Tags Cles

| Usage | Tag |
| --- | --- |
| Quetes ORION | `Quest.*` |
| Nodes ORION | `Quest.<Zone>.<Quest>.Node.*` |
| Items/interactables de quete | `Quest.<Zone>.<Quest>.Item.*` |
| Participants de dialogue ORION | `Dialog.Participant.*` |
| Faction antagoniste | `Faction.SansAube` |
| Capacite grappin | `Ability.GrapplingHook` |
| Item global preuve du loup | `Item.WolfClaw` |
| Item global narratif de Maelys | `Item.MaelysScarf` |

## 2. Tableau des Assets a Creer

### Quetes

| Type | Nom fichier conseille | Tag ORION | PNJ / acteur lie | Etat initial | Notes |
| --- | --- | --- | --- | --- | --- |
| Quest Blueprint | `BPQ_PRO_01_RetrouverSesEsprits` | `Quest.Prologue.RetrouverSesEsprits` | Spawn player, foulard de Maelys | Active | Quete automatique au premier spawn. |
| Quest Blueprint | `BPQ_PRO_02_PremierSang` | `Quest.Prologue.PremierSang` | `BP_Wolf`, clairiere du vieux chene | Locked | Activee quand le joueur entre dans le trigger de la clairiere. |
| Quest Blueprint | `BPQ_PRO_03_ArriveeValdorel` | `Quest.Prologue.ArriveeValdorel` | Aveline Ronce, porte de Valdorel | Locked | Premiere interaction avec le village. |
| Quest Blueprint | `BPQ_PRO_04_LeDoyen` | `Quest.Prologue.LeDoyen` | Orvan Lume, maison du doyen | Locked | Pose le lore des Sans-Aube et dirige vers Nalia. |
| Quest Blueprint | `BPQ_VIL_01_GrappinDeNalia` | `Quest.Village.GrappinDeNalia` | Nalia Forgebrume, atelier | Locked | Repare et remet le grappin au joueur. |
| Quest Blueprint | `BPQ_VIL_02_EssayerLeGrappin` | `Quest.Village.EssayerLeGrappin` | Nalia Forgebrume, zone d'entrainement | Locked | Tutoriel separe pour tester le grappin. |
| Quest Blueprint | `BPQ_VIL_03_ChatSurLesToits` | `Quest.Village.ChatSurLesToits` | Mirette Solain, auberge, chat Pepin | Unlocked | Quete secondaire optionnelle apres obtention du grappin. |
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

1. Aller a la zone ou Maelys a ete enlevee.
2. Ramasser le foulard de Maelys.
3. Continuer vers le sentier.

Dialogues :

| Participant | Ligne |
| --- | --- |
| Player | "Maelys... non. Ils l'ont emmenee vers les hauteurs." |
| Player | "Le village est tout proche. Quelqu'un a Valdorel saura quoi faire." |

Conditions de succes :

- Le joueur atteint le trigger de la clairiere du vieux chene.

Recompenses :

- Ajout ou validation de `Quest.Prologue.RetrouverSesEsprits.Item.MaelysScarf`.
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

1. Ramasser une vieille lame.
2. Traverser le sentier sombre.
3. Survivre a l'attaque.
4. Vaincre le loup.
5. Recuperer une griffe de loup.
6. Rejoindre Valdorel.

Dialogues :

| Participant | Ligne |
| --- | --- |
| Player | "Pas maintenant... recule !" |
| Player | "Ce n'etait pas un loup ordinaire. Ses yeux brillaient." |

Conditions de succes :

- Le loup est mort.
- Le joueur recupere `Quest.Prologue.PremierSang.Item.WolfClaw`.
- Le joueur atteint l'entree de Valdorel.

Recompenses :

- Ajout de `Quest.Prologue.PremierSang.Item.WolfClaw`.
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
| Orvan | "Les Sans-Aube ont du prendre les Gorges d'Ysbrume. Sans equipement, tu n'atteindras meme pas les premieres corniches. Va voir Nalia pour son grappin. Pour les hauteurs... l'Ether devra peut-etre repondre autrement." |

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
| But de gameplay | Reparer le grappin via une mini-quete de recolte et d'exploration, puis lancer une quete separee de test. |

Objectifs dans l'ordre :

1. Parler a Nalia dans l'atelier.
2. Recuperer 3 fibres de ronce solide.
3. Recuperer 1 ressort ancien dans la remise.
4. Revenir voir Nalia.

Dialogues `BPD_Nalia_Grappin` :

| Participant | Ligne |
| --- | --- |
| Nalia | "Un passage par les gorges ? Tu veux mourir vite ou juste avec panache ?" |
| Player | "Je dois retrouver Maelys." |
| Nalia | "Alors il te faut mon grappin. Mais il manque de quoi refaire la ligne." |
| Nalia | "Bien. Ne vise pas les nuages, vise les anneaux marques. Le reste viendra avec les bleus." |

Conditions de succes :

- Les composants sont recuperes.
- Dialogue de remise du grappin avec Nalia termine.

Recompenses :

- Debloque `Ability.GrapplingHook`.
- Activation de `Quest.Village.EssayerLeGrappin`.
- Unlock de `Quest.Village.ChatSurLesToits`.

Quete suivante :

- Activer `Quest.Village.EssayerLeGrappin`.
- Unlock `Quest.Village.ChatSurLesToits` comme contenu optionnel.

### Q_VIL_02 - Essayer le grappin

| Champ | Valeur |
| --- | --- |
| Nom joueur | Essayer le grappin |
| Nom asset conseille | `BPQ_VIL_02_EssayerLeGrappin` |
| Quest Tag | `Quest.Village.EssayerLeGrappin` |
| Etat ORION initial | Locked |
| Declencheur | Completion de `Quest.Village.GrappinDeNalia`. |
| PNJ principal | Nalia Forgebrume |
| But de gameplay | Isoler le tutoriel du grappin dans une quete claire, sans melanger recolte/reparation et apprentissage. |

Objectifs dans l'ordre :

1. Parler a Nalia dans la zone d'entrainement.
2. Tester le grappin sur l'ancre d'entrainement.
3. Retourner voir Nalia.

Conditions de succes :

- Le joueur utilise correctement le grappin sur l'ancre d'entrainement.
- Le dialogue final avec Nalia est termine.

Recompenses :

- Confirmation du tutoriel grappin.
- Activation de `Quest.Prologue.RouteDesGorges`.

Quete suivante :

- Activer `Quest.Prologue.RouteDesGorges`.

### Q_VIL_03 - Le chat de Mirette

| Champ | Valeur |
| --- | --- |
| Nom joueur | Le chat de Mirette |
| Nom asset conseille | `BPQ_VIL_03_ChatSurLesToits` |
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
| Declencheur | Completion de `Quest.Village.EssayerLeGrappin`. |
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

## Descriptions ORION a Remplir

Cette section est la reference canonique pour les champs visibles par le joueur. Utilise `Quest Name` et `Quest Description` dans la quete, puis `NodeName` et `Description` dans chaque node.

### `BPQ_PRO_01_RetrouverSesEsprits`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Le foulard de Maelys` |
| Quest Description | `Maelys vient d'etre enlevee. Reprenez vos esprits, rejoignez la zone ou elle a disparu et cherchez un indice avant de prevenir Valdorel.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Go To Location | `Quest.Prologue.RetrouverSesEsprits.Node.GoToLocation` | `Retourner au village` | `Rejoignez le point ou Maelys a ete emportee et cherchez une trace de son passage.` |
| Collect Scarf | `Quest.Prologue.RetrouverSesEsprits.Node.CollectScarf` | `Ramasser le foulard de Maelys` | `Ramassez le foulard de Maelys. Il prouve qu'elle est passee par ici et donne une piste vers Valdorel.` |
| Complete Quest | `Quest.Prologue.RetrouverSesEsprits.Node.CompleteQuest` | `Continuer vers le sentier` | `Le foulard en main, poursuivez vers le sentier menant a Valdorel.` |

Items :

| Element | Tag |
| --- | --- |
| Foulard de Maelys | `Quest.Prologue.RetrouverSesEsprits.Item.MaelysScarf` |

Implementation :

- `Collect Scarf` doit etre bloque tant que `Go To Location` n'est pas complete.
- Dans le BP du foulard, verifier `CanProgressQuestObjective` avec `Quest.Prologue.RetrouverSesEsprits` et `Quest.Prologue.RetrouverSesEsprits.Node.CollectScarf`.
- `Collect Scarf` est un objectif unique : `bUseAmount = false`.

### `BPQ_PRO_02_PremierSang`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Le loup du sentier` |
| Quest Description | `Le chemin vers Valdorel est dangereux. Trouvez de quoi vous defendre, traversez le repaire et survivez a l'attaque du loup.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Recover Old Blade | `Quest.Prologue.PremierSang.Node.RecoverOldBlade` | `Ramasser la vieille lame` | `Ramassez une vieille lame abandonnee pres du sentier. Elle devrait suffire a vous defendre.` |
| Reach Wolf Den | `Quest.Prologue.PremierSang.Node.ReachWolfDen` | `Traverser le sentier sombre` | `Avancez vers Valdorel en suivant le sentier malgre les bruits dans les bois.` |
| Survive Attack | `Quest.Prologue.PremierSang.Node.SurviveAttack` | `Survivre a l'attaque` | `Un loup vous attaque. Gardez vos distances, esquivez et restez en vie.` |
| Defeat Wolf | `Quest.Prologue.PremierSang.Node.DefeatWolf` | `Vaincre le loup` | `Abattez le loup qui bloque le passage vers Valdorel.` |
| Collect Wolf Claw | `Quest.Prologue.PremierSang.Node.CollectWolfClaw` | `Ramasser la griffe noircie` | `Recuperez une griffe du loup. Sa couleur anormale pourrait interesser les gardes du village.` |
| Reach Valdorel | `Quest.Prologue.PremierSang.Node.ReachValdorel` | `Rejoindre Valdorel` | `Rejoignez les portes de Valdorel pour demander de l'aide.` |
| Complete Quest | `Quest.Prologue.PremierSang.Node.CompleteQuest` | `Entrer a Valdorel` | `Vous avez survecu au sentier. Trouvez quelqu'un capable de vous aider.` |

Items :

| Element | Tag |
| --- | --- |
| Vieille lame | `Quest.Prologue.PremierSang.Item.OldBlade` |
| Griffe noircie | `Quest.Prologue.PremierSang.Item.WolfClaw` |

### `BPQ_PRO_03_ArriveeValdorel`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Les portes de Valdorel` |
| Quest Description | `Vous avez atteint Valdorel avec le foulard de Maelys et une griffe noircie. Convainquez la garde Aveline de vous laisser entrer et trouvez le doyen.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Talk To Aveline | `Quest.Prologue.ArriveeValdorel.Node.TalkToAveline` | `Parler a Aveline` | `Expliquez a la garde de Valdorel ce qui est arrive a Maelys.` |
| Show Wolf Claw | `Quest.Prologue.ArriveeValdorel.Node.ShowWolfClaw` | `Montrer la griffe noircie` | `Montrez la griffe trouvee sur le loup pour prouver que le sentier est corrompu.` |
| Talk To Orvan | `Quest.Prologue.ArriveeValdorel.Node.TalkToOrvan` | `Trouver le doyen Orvan` | `Aveline vous envoie voir Orvan. Rejoignez sa maison pour comprendre qui a enleve Maelys.` |
| Complete Quest | `Quest.Prologue.ArriveeValdorel.Node.CompleteQuest` | `Entrer chez Orvan` | `Le doyen doit maintenant entendre votre histoire.` |

Items :

| Element | Tag |
| --- | --- |
| Preuve de la griffe | `Quest.Prologue.ArriveeValdorel.Item.WolfClawEvidence` |

### `BPQ_PRO_04_LeDoyen`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Le nom des ravisseurs` |
| Quest Description | `Racontez l'enlevement a Orvan. Le doyen connait peut-etre le symbole des Sans-Aube et le chemin qu'ils ont emprunte.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Question Orvan | `Quest.Prologue.LeDoyen.Node.QuestionOrvan` | `Interroger Orvan` | `Racontez l'enlevement de Maelys au doyen et montrez-lui les indices recuperes.` |
| Examine Map | `Quest.Prologue.LeDoyen.Node.ExamineMap` | `Examiner la carte d'Ysbrume` | `Examinez la carte d'Orvan pour comprendre pourquoi les Gorges d'Ysbrume sont difficiles d'acces.` |
| Talk To Nalia | `Quest.Prologue.LeDoyen.Node.TalkToNalia` | `Aller voir Nalia` | `Orvan pense que le grappin de Nalia vous permettra d'atteindre les premieres corniches des gorges.` |
| Complete Quest | `Quest.Prologue.LeDoyen.Node.CompleteQuest` | `Demander le grappin` | `Les Sans-Aube ont probablement pris Ysbrume. Il vous faut de l'equipement avant de les suivre.` |

Items :

| Element | Tag |
| --- | --- |
| Carte d'Ysbrume | `Quest.Prologue.LeDoyen.Item.YsbrumeMap` |

Note lore :

- Le grappin est l'equipement physique necessaire pour commencer Ysbrume.
- Le glide est seulement tease ici : c'est un pouvoir d'Ether lie au vent, a eveiller plus tard dans une quete ou un donjon.

### `BPQ_VIL_01_GrappinDeNalia`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Le grappin de Nalia` |
| Quest Description | `Nalia peut vous aider a atteindre les premieres corniches d'Ysbrume, mais son grappin doit etre repare avant de pouvoir l'utiliser.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Talk To Nalia | `Quest.Village.GrappinDeNalia.Node.TalkToNalia` | `Parler a Nalia` | `Expliquez a Nalia que les Sans-Aube ont pris la route d'Ysbrume et que vous devez les suivre.` |
| Collect Bramble Fibers | `Quest.Village.GrappinDeNalia.Node.CollectBrambleFibers` | `Recuperer 3 fibres de ronce` | `Recuperez trois fibres de ronce solide derriere l'atelier pour reparer la ligne du grappin.` |
| Collect Ancient Spring | `Quest.Village.GrappinDeNalia.Node.CollectAncientSpring` | `Trouver le ressort ancien` | `Cherchez un ressort ancien dans la remise de Nalia.` |
| Return To Nalia | `Quest.Village.GrappinDeNalia.Node.ReturnToNalia` | `Retourner voir Nalia` | `Retournez voir Nalia avec les pieces necessaires pour qu'elle termine le grappin.` |
| Complete Quest | `Quest.Village.GrappinDeNalia.Node.CompleteQuest` | `Recevoir le grappin` | `Le grappin est repare. Nalia veut maintenant vous le faire essayer dans une zone sure.` |

Items :

| Element | Tag |
| --- | --- |
| Fibre de ronce | `Quest.Village.GrappinDeNalia.Item.BrambleFiber` |
| Ressort ancien | `Quest.Village.GrappinDeNalia.Item.AncientSpring` |
| Grappin | `Quest.Village.GrappinDeNalia.Item.GrapplingHook` |

### `BPQ_VIL_02_EssayerLeGrappin`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Essayer le grappin` |
| Quest Description | `Le grappin est repare, mais Nalia refuse de vous laisser partir sans un essai. Testez-le sur l'ancre d'entrainement avant de quitter Valdorel.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Talk To Nalia | `Quest.Village.EssayerLeGrappin.Node.TalkToNalia` | `Ecouter Nalia` | `Nalia vous explique comment viser les ancres marquees avant le premier essai.` |
| Test Grappling Hook | `Quest.Village.EssayerLeGrappin.Node.TestGrapplingHook` | `Tester le grappin` | `Utilisez le grappin sur l'ancre d'entrainement pour verifier que vous pouvez vous accrocher et vous deplacer.` |
| Return To Nalia | `Quest.Village.EssayerLeGrappin.Node.ReturnToNalia` | `Retourner voir Nalia` | `Retournez voir Nalia apres l'essai pour valider que vous maitrisez les bases.` |
| Complete Quest | `Quest.Village.EssayerLeGrappin.Node.CompleteQuest` | `Partir vers Ysbrume` | `Vous savez utiliser le grappin. La route des Gorges d'Ysbrume est maintenant accessible.` |

Items :

| Element | Tag |
| --- | --- |
| Ancre d'entrainement | `Quest.Village.EssayerLeGrappin.Item.TrainingAnchor` |

### `BPQ_VIL_03_ChatSurLesToits`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Le chat de Mirette` |
| Quest Description | `Mirette a perdu son chat Pepin sur les toits de Valdorel. Utilisez le grappin pour le recuperer sans casser l'auberge.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Talk To Mirette | `Quest.Village.ChatSurLesToits.Node.TalkToMirette` | `Parler a Mirette` | `Mirette cherche quelqu'un pour recuperer son chat coince sur les toits.` |
| Find Pepin | `Quest.Village.ChatSurLesToits.Node.FindPepin` | `Reperer Pepin` | `Trouvez Pepin sur les toits de l'auberge.` |
| Reach Roof | `Quest.Village.ChatSurLesToits.Node.ReachRoof` | `Monter sur le toit` | `Utilisez le grappin pour atteindre le toit ou Pepin s'est refugie.` |
| Return Pepin | `Quest.Village.ChatSurLesToits.Node.ReturnPepin` | `Ramener Pepin` | `Ramenez Pepin a Mirette pour terminer cette faveur.` |
| Complete Quest | `Quest.Village.ChatSurLesToits.Node.CompleteQuest` | `Recevoir la recompense` | `Mirette vous remercie pour avoir sauve son chat et presque ses tuiles.` |

Items :

| Element | Tag |
| --- | --- |
| Pepin | `Quest.Village.ChatSurLesToits.Item.Pepin` |

### `BPQ_PRO_05_RouteDesGorges`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Vers les Gorges d'Ysbrume` |
| Quest Description | `Le grappin vous permet d'atteindre les premieres falaises d'Ysbrume. Suivez les traces des Sans-Aube et cherchez un passage vers les hauteurs.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Reach Broken Bridge | `Quest.Prologue.RouteDesGorges.Node.ReachBrokenBridge` | `Atteindre le pont effondre` | `Suivez la route d'Ysbrume jusqu'au pont effondre.` |
| Cross With Grappling Hook | `Quest.Prologue.RouteDesGorges.Node.CrossWithGrapplingHook` | `Traverser avec le grappin` | `Utilisez le grappin pour franchir le pont brise et atteindre l'autre rive.` |
| Find Abandoned Camp | `Quest.Prologue.RouteDesGorges.Node.FindAbandonedCamp` | `Trouver le camp abandonne` | `Cherchez un camp recent. Les Sans-Aube ont peut-etre fait halte dans les gorges.` |
| Examine Sans-Aube Tracks | `Quest.Prologue.RouteDesGorges.Node.ExamineSansAubeTracks` | `Examiner les traces` | `Examinez les traces laissees par les Sans-Aube pour confirmer que Maelys est passee ici.` |
| Complete Quest | `Quest.Prologue.RouteDesGorges.Node.CompleteQuest` | `Suivre la piste` | `La piste continue vers les hauteurs, mais la brume d'Ysbrume cache d'autres dangers.` |

Items :

| Element | Tag |
| --- | --- |
| Traces des Sans-Aube | `Quest.Prologue.RouteDesGorges.Item.SansAubeTracks` |

### `BPQ_GOR_01_NuisiblesDEther`

| Champ | Valeur |
| --- | --- |
| Quest Name | `Nids dans la brume` |
| Quest Description | `Un eclaireur blesse a vu les Sans-Aube passer avec Maelys. Nettoyez les nids de guepes d'Ether pour ouvrir la route.` |

| Node | Node Tag | NodeName | Description |
| --- | --- | --- | --- |
| Talk To Soren | `Quest.Gorges.NuisiblesDEther.Node.TalkToSoren` | `Parler a Soren` | `Interrogez l'eclaireur blesse pres du camp abandonne.` |
| Kill Ether Wasps | `Quest.Gorges.NuisiblesDEther.Node.KillEtherWasps` | `Eliminer 4 guepes d'Ether` | `Tuez quatre guepes d'Ether pour securiser le passage.` |
| Destroy Nests | `Quest.Gorges.NuisiblesDEther.Node.DestroyNests` | `Detruire 2 nids` | `Detruisez deux nids pour empecher les guepes de revenir.` |
| Return To Soren | `Quest.Gorges.NuisiblesDEther.Node.ReturnToSoren` | `Retourner voir Soren` | `Retournez voir Soren pour obtenir la direction prise par les Sans-Aube.` |
| Complete Quest | `Quest.Gorges.NuisiblesDEther.Node.CompleteQuest` | `Ouvrir le raccourci` | `La route vers les ruines hautes est degagee. Continuez la poursuite de Maelys.` |

Items :

| Element | Tag |
| --- | --- |
| Guepe d'Ether | `Quest.Gorges.NuisiblesDEther.Item.EtherWasp` |
| Nid de guepes | `Quest.Gorges.NuisiblesDEther.Item.EtherNest` |

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
- Le tutoriel d'utilisation du grappin est separe dans `Quest.Village.EssayerLeGrappin`.
- La quete du chat est optionnelle et ne doit pas bloquer la progression principale.
- La deuxieme zone doit etre bloquee tant que `Quest.Village.EssayerLeGrappin` n'est pas completee.
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
- Verifier que `Quest.Prologue.RouteDesGorges` ne s'active qu'apres `Quest.Village.EssayerLeGrappin`.

## Assumptions

- Le document est en Markdown dans le repo.
- Les noms de fichiers utilisent les prefixes `BPQ_` pour les quetes et `BPD_` pour les dialogues.
- Les tags Unreal restent sans accents et sans espaces.
- Les Sans-Aube remplacent definitivement l'ancien nom des antagonistes.
