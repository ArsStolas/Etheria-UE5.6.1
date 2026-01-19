# ModularSurfaceAudio (UE5.6) — Plugin C++ (Blueprint Friendly)

Objectif : gérer **Footsteps** (marche/course/sprint) + **Landing** (volume/pitch en fonction de |VelocityZ|) + **Wind Loop** (directionnel selon vitesse) :
- Basé sur **Surface Types** (Project Settings → Physics)
- **Random** dans des listes (anti-répétition)
- **Spatialisation** : chaque pas est joué **à la position du pied** (socket ou IK)
- **Perf first** : **pas de Tick** → uniquement **timers**, et **1 trace uniquement au moment des notifies**
- **Solo** : aucune réplication

---

## 1) Installation

1. Copie le dossier `Plugins/ModularSurfaceAudio` dans ton projet UE :
   - `YourProject/Plugins/ModularSurfaceAudio`
2. Ouvre le projet, accepte la recompilation si demandée.
3. Vérifie : `Edit → Plugins` et cherche **Modular Surface Audio**.

---

## 2) Configurer les surfaces (Surface Types)

1. `Project Settings → Physics → Surface Types`
2. Ajoute tes surfaces : `Grass`, `Sand`, `Mud`, `Wood`, etc.
3. Pour chaque **Physical Material** (PhysMat_Grass…), définis le `Surface Type` correspondant.
4. Landscape : assure-toi que chaque layer (LayerInfo ou setup material) utilise le bon PhysMat.

---

## 3) Créer la librairie audio (DataAsset)

### Surface Audio Library

1. Dans le Content Browser : `Add → Miscellaneous → Data Asset`
2. Choisis `SurfaceAudioLibrary`
3. Remplis :
   - `DefaultEntry` : sons fallback si surface inconnue
   - `Entries` : une entrée par `SurfaceType`

Chaque entrée contient :
- `Footsteps` : Walk / Run / Sprint
  - `Dry[]` : liste de sons secs
  - `Wet[]` : liste de sons “pluie”
- `Landing` :
  - `Sounds (Dry/Wet)`
  - `VolumeCurve` (X = |VelocityZ|, Y = volume multiplier)
  - `PitchCurve` (X = |VelocityZ|, Y = pitch multiplier)
  - `MinImpactSpeed` (évite sons sur micro marches)

---

## 4) Ajouter les components sur ton Player (Blueprint)

Dans ton `BP_PlayerCharacter` :

### A) Footsteps + Landing
- Ajoute un component : `SurfaceAudioComponent`
- Assigne `Library` = ton DataAsset `DA_SurfaceAudioLibrary`
- Option perf : laisse `WetPlaybackMode = Switch` (1 seul son par event)

### B) Wind
- Ajoute un component : `WindAudioComponent`
- Crée un DataAsset : `WindAudioProfile`
  - `WindLoop` : idéalement un MetaSound en loop
  - `VolumeCurve` / `PitchCurve` (X = Speed)
  - `MinSpeed` et `UpdateInterval` (timer)
- Assigne `Profile`.

**Important** : `WindAudioComponent` ne tourne pas en Tick, il met à jour le loop via un timer.

---

## 5) Mettre les AnimNotifies de footsteps

Dans tes animations walk/run/sprint :
1. Ajoute le notify : `Surface Footstep`
2. Paramètres du notify :
   - `Foot` : Left / Right
   - `Gait` : Walk / Run / Sprint
   - `FootSocketName` : par défaut `foot_l` / `foot_r` (adapte selon ton skeleton)
   - `bTryIKProvider` : coche si tu veux utiliser l’IK (voir section suivante)

Le notify appelle automatiquement `SurfaceAudioComponent.PlayFootstepFromNotify(...)`.

---

## 6) Option IK (position exacte du pied)

Si tu veux que la position du son soit celle du pied “IK” (plus précis sur pentes/escaliers) :

1. Dans ton Character BP, implémente l’interface : `FootPlacementProviderInterface`
2. Implémente `GetFootWorldLocation(Foot, OutWorldLocation)`
   - Retourne la position world de ton IK foot (Left/Right)
3. Dans le notify : coche `bTryIKProvider`

Perf : aucune trace supplémentaire, on ne fait que changer le point de départ.

---

## 7) Landing (volume/pitch scalés) + cache VelocityZ (sans Tick)

### Setup recommandé (perf + fiable)

Dans ton Character BP :
- `Event OnMovementModeChanged`
  - Si `NewMode == Falling` : `SurfaceAudioComponent.StartLandingVelocityCache()`
  - Sinon : `SurfaceAudioComponent.StopLandingVelocityCache()`

- `Event OnLanded (Hit)` :
  - `SurfaceAudioComponent.PlayLandingFromCache(Hit)`
  - `SurfaceAudioComponent.StopLandingVelocityCache()`

Ainsi :
- pendant la chute, un timer échantillonne `Abs(VelocityZ)`
- à l’atterrissage, on joue le son avec les curves.

---

## 8) Wetness (météo/pluie)

Depuis ton système météo (Blueprint) :
- Quand il pleut : `SurfaceAudioComponent.SetWetness(1.0)`
- Quand il fait sec : `SetWetness(0.0)`

Mode :
- `WetPlaybackMode = Switch` (perf) : joue Dry OU Wet
- `WetPlaybackMode = Crossfade` (qualité) : joue Dry + Wet pondérés (peut doubler le nombre de one-shots)

---

## 9) Test checklist

1. Place ton perso sur 2 surfaces (ex: Grass puis Sand)
2. Marche : sons différents
3. Cours : sons différents
4. Saute et atterris : landing avec scaling volume/pitch
5. Active pluie : sons Wet
6. En air à grande vitesse : vent qui monte

---

## Notes perf
- Footsteps : **1 trace UNIQUEMENT au moment du notify**
- Landing : pas de trace si `OnLanded` fournit PhysMat, sinon fallback 1 trace (via Hit)
- Wind : timer (UpdateInterval), **pas de Tick**

---

## Support
Tu peux étendre le plugin en ajoutant :
- Concurrency / Attenuation par surface
- Events BP (OnFootstepPlayed) pour VFX

