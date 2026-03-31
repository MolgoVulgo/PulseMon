# Navigation gestuelle — JC3248W535EN (Guide standard projet)

## Objectif

Ce document définit la méthode standard pour implémenter la navigation par gestes tactiles sur les projets utilisant l'écran **JC3248W535EN (ESP32‑S3 + écran 3.5" + tactile capacitif)** avec **LVGL**.

Le document est volontairement **générique** :

- indépendant du projet (Weather, monitoring, etc.)
- indépendant de la structure UI spécifique
- applicable à toute UI LVGL basée sur écrans root

Il définit :

- la chaîne tactile
- le routage des gestes
- l'architecture de navigation
- le rattachement aux écrans UI

---

# 1. Architecture générale

La navigation gestuelle repose sur quatre couches :

```text
Touch controller
   ↓
ESP BSP / driver tactile
   ↓
LVGL input device
   ↓
LV_EVENT_GESTURE
   ↓
Gesture router
   ↓
Navigation screen
   ↓
UI refresh
```

Responsabilités :

| Couche | Rôle |
|------|------|
| BSP tactile | lire le contrôleur tactile |
| LVGL indev | convertir le contact en geste |
| Gesture router | décider de la navigation |
| UI | charger l'écran et mettre à jour les widgets |

---

# 2. Initialisation tactile (JC3248W535EN)

Sur les projets basés JC3248W535EN, le tactile est généralement initialisé via le BSP.

Chaîne typique :

```text
bsp_display_start_with_config()
   ↓
bsp_display_indev_init()
   ↓
bsp_touch_new()
   ↓
lvgl_port_add_touch()
```

Résultat :

- LVGL possède un **indev tactile actif**
- LVGL peut générer des **gestures events**

Aucun code supplémentaire n'est nécessaire pour détecter les gestes.

---

# 3. Détection du geste

LVGL fournit directement la direction du geste.

Fonction utilisée :

```c
lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
```

Valeurs possibles :

| Direction | Signification |
|-----------|--------------|
| LV_DIR_LEFT | swipe gauche |
| LV_DIR_RIGHT | swipe droite |
| LV_DIR_TOP | swipe haut |
| LV_DIR_BOTTOM | swipe bas |

La plupart des interfaces utilisent uniquement **LEFT / RIGHT**.

---

# 4. Routeur de geste

Un **gesture router** centralise la navigation.

Principe :

```text
screen source
   +
 direction gesture
   ↓
 screen cible
```

Exemple logique :

| écran courant | geste | écran cible |
|---------------|------|------------|
| Home | swipe left | Details |
| Details | swipe right | Home |
| Home | swipe right | Monitoring |
| Monitoring | swipe left | Home |

Le routeur ne doit pas contenir de logique UI complexe.

Il doit uniquement :

- identifier l'écran source
- lire la direction
- déclencher le changement d'écran

---

# 5. Chargement de l'écran

Le changement d'écran utilise LVGL :

```c
lv_scr_load_anim(
    target_screen,
    animation_type,
    duration,
    delay,
    auto_delete
);
```

Animations recommandées :

| geste | animation |
|------|-----------|
| swipe gauche | MOVE_LEFT |
| swipe droite | MOVE_RIGHT |

Cela permet de conserver une cohérence visuelle.

---

# 6. Rattachement aux écrans UI

La détection de geste doit être attachée **au root screen**.

Principe :

```c
lv_obj_add_event_cb(
    screen_root,
    gesture_handler,
    LV_EVENT_GESTURE,
    NULL
);
```

Important :

- attacher le callback au **screen root**
- ne pas l'attacher à des widgets internes

Cela garantit :

- geste global
- comportement cohérent

---

# 7. Organisation recommandée

Structure recommandée :

```text
ui/

screens.c
screens.h

navigation.c
navigation.h

ui_refresh.c
```

Responsabilités :

| fichier | rôle |
|-------|------|
| navigation.c | router gestuel |
| screens.c | création UI |
| ui_refresh.c | mise à jour widgets |

---

# 8. Refresh UI

Après un changement d'écran, il est recommandé de rafraîchir les widgets.

Exemple logique :

```text
load screen
   ↓
update widgets
```

Exemple typique :

```text
update météo
update capteurs
update graph
```

Cela évite :

- écrans affichant des valeurs anciennes

---

# 9. Écrans recommandés pour navigation gestuelle

Tous les écrans ne doivent pas supporter les gestes.

### Écrans adaptés

- dashboards
- monitoring
- météo
- graphiques

### Écrans à éviter

- paramètres
- formulaires
- configuration wifi

Ces écrans utilisent plutôt des boutons.

---

# 10. Bonnes pratiques

### garder un router central

Éviter d'écrire la navigation dans plusieurs fichiers.

### éviter la logique dans les callbacks UI

Les callbacks UI doivent uniquement appeler le router.

### garder une matrice claire

La navigation doit être représentée sous forme de matrice.

---

# 11. Exemple de flux complet

```text
finger swipe

↓

LVGL gesture event

↓

gesture router

↓

screen change

↓

UI refresh
```

---

# 12. Résumé

Sur les projets JC3248W535EN :

- le tactile est géré par le BSP
- LVGL détecte les gestes
- un routeur central gère la navigation
- les gestes sont attachés aux écrans root
- les écrans sont rechargés avec animation
- l'UI est rafraîchie après navigation

