# Prompt Codex — correction des graphes pour retrouver le rendu visuel de la v1

Objectif : modifier uniquement la **géométrie** et les **labels d’axe Y** des graphes LVGL de la nouvelle version afin de retrouver le rendu visuel de la v1, **sans toucher à l’alimentation actuelle des données**, **sans modifier la logique de séries**, **sans changer le mode d’update**, et **sans réécrire le pipeline de rendu**.

## Contexte

Références de travail :
- ancienne version : `monitoring_ui.cpp`, `monitoring_ui.h`
- nouvelle version : `ui_graphs.c`, `ui_graphs.h`

Constat :
- la v1 est visuellement meilleure sur les graphes,
- la v2 alimente correctement les graphes,
- la régression visuelle vient de la **géométrie du chart** et du **placement / parentage des labels Y**,
- il ne faut **rien changer** à l’alimentation des points.

## Contraintes strictes

Ne pas modifier :
- la logique d’alimentation des graphes,
- les buffers / séries / update des points,
- le mode `LV_CHART_UPDATE_MODE_SHIFT`,
- les couleurs de séries,
- les plages dynamiques existantes,
- la logique métier,
- les signatures publiques sauf nécessité minimale.

Modifier uniquement :
- la géométrie interne du chart dans le panel,
- le parent des labels d’axe Y,
- le calcul de placement des labels Y.

## Résultat attendu

Retrouver un rendu proche v1 :
- chart placé avec une marge gauche dédiée à l’axe Y,
- espace supérieur conservé pour le titre,
- espace inférieur conservé pour la légende,
- labels Y positionnés **dans le panel** et non **dans le chart**,
- alignement propre et stable même si la taille du texte varie légèrement.

## Modifications à faire

### 1. Revenir à la géométrie v1 du chart

Dans `ui_graphs.c`, la fonction `init_chart_common()` ne doit plus utiliser un sizing automatique du type `lv_pct(100)` / pourcentage de hauteur.

Appliquer explicitement :
- position : `x = 30`, `y = 18`
- taille : `w = 186`, `h = 114`

Le chart doit donc être posé comme dans la v1, à l’intérieur du panel.

Exigence :
- conserver tous les styles actuels utiles,
- conserver `LV_CHART_TYPE_LINE`,
- conserver `LV_CHART_UPDATE_MODE_SHIFT`,
- conserver `lv_chart_set_div_line_count(chart, 5, 6)`,
- conserver `lv_chart_set_point_count(chart, GRAPH_POINT_COUNT)`.

### 2. Refaire le parentage des labels Y

Actuellement, les labels Y sont créés avec le **chart** comme parent.
Il faut les créer avec le **panel** comme parent, comme dans la v1.

Donc dans `ui_graphs_init()` :
- les labels du graphe usage doivent être créés dans `usage_panel`,
- les labels du graphe température doivent être créés dans `temp_panel`.

Ne pas changer leur style si ce style est déjà correct.

### 3. Recalculer correctement la position des labels Y

La fonction qui positionne les labels Y doit être revue pour utiliser la géométrie réelle du chart à l’intérieur du panel.

Le calcul doit utiliser :
- `chart_x`
- `chart_y`
- `chart_h`
- `pad_top`
- `pad_bottom`
- `border_width`

Objectif :
- déterminer une zone verticale utile cohérente,
- répartir les labels sur cette hauteur,
- placer les labels avec `x = 0`,
- donner aux labels une largeur calculée à partir de la position du chart : `label_w = chart_x - 4` si possible, sinon largeur minimale de sécurité.

Le positionnement ne doit plus dépendre d’un offset fixe fragile du type `x = -3` ou d’une largeur figée du type `24`.

### 4. Nettoyage minimal

Supprimer seulement ce qui devient inutile après correction géométrique, par exemple une constante de hauteur en pourcentage si elle ne sert plus.

Ne pas faire de refactor large.

## Implémentation cible

Appliquer une logique équivalente à ceci.

### `init_chart_common()`

```c
static void init_chart_common(lv_obj_t *chart)
{
    lv_obj_set_pos(chart, 30, 18);
    lv_obj_set_size(chart, 186, 114);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x0f131d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x232a38), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x2a3140), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(chart, lv_color_hex(0x90a0bc), LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(chart, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 5, 6);
    lv_chart_set_point_count(chart, GRAPH_POINT_COUNT);
}
```

### Création des labels Y

```c
for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
    s_usage_axis_labels[i] = create_axis_value_label(usage_panel);
}

for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
    s_temp_axis_labels[i] = create_axis_value_label(temp_panel);
}
```

### Positionnement des labels Y

```c
static void update_chart_axis_labels(lv_obj_t *chart, lv_obj_t *labels[AXIS_LABEL_COUNT], int range_min, int range_max)
{
    if (chart == NULL) {
        return;
    }

    lv_obj_update_layout(chart);

    lv_coord_t chart_x = lv_obj_get_x(chart);
    lv_coord_t chart_y = lv_obj_get_y(chart);
    lv_coord_t chart_h = lv_obj_get_height(chart);
    if (chart_h < 1) {
        chart_h = 1;
    }

    lv_coord_t pad_top = lv_obj_get_style_pad_top(chart, LV_PART_MAIN);
    lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(chart, LV_PART_MAIN);
    lv_coord_t border = lv_obj_get_style_border_width(chart, LV_PART_MAIN);

    lv_coord_t plot_top = chart_y + pad_top + border;
    lv_coord_t plot_h = chart_h - pad_top - pad_bottom - (border * 2);
    if (plot_h < 1) {
        plot_h = chart_h;
        plot_top = chart_y;
    }

    lv_coord_t label_w = (chart_x > 4) ? (chart_x - 4) : 10;

    char buf[12];
    float span = (float)(range_max - range_min);

    for (size_t i = 0; i < AXIS_LABEL_COUNT; ++i) {
        lv_obj_t *label = labels[i];
        if (label == NULL) {
            continue;
        }

        float ratio = (AXIS_LABEL_COUNT > 1)
            ? ((float)i / (float)(AXIS_LABEL_COUNT - 1))
            : 0.0f;

        float value = (float)range_max - (span * ratio);
        snprintf(buf, sizeof(buf), "%.0f", (double)value);
        set_text_if_changed(label, buf);

        lv_obj_set_width(label, label_w);

        lv_coord_t label_h = lv_obj_get_height(label);
        lv_coord_t y = plot_top + (lv_coord_t)((float)plot_h * ratio) - (label_h / 2);

        lv_obj_set_pos(label, 0, y);
    }
}
```

## Vérifications attendues après modification

Vérifier visuellement et fonctionnellement :
- le chart n’occupe plus toute la largeur du panel,
- une colonne gauche existe pour les labels Y,
- les labels Y ne sont plus inclus dans le chart,
- le haut du panel garde de la place pour le titre,
- le bas du panel garde de la place pour la légende,
- aucune régression sur l’alimentation des courbes,
- aucune modification de la logique de données.

## Livrable attendu de Codex

Fournir :
1. les modifications directement appliquées dans `ui_graphs.c` et `ui_graphs.h` si nécessaire,
2. un diff ou patch lisible,
3. un résumé très court des changements,
4. la confirmation explicite que seule la partie **géométrie + labels** a été touchée et que l’alimentation actuelle des graphes est inchangée.

## Rappel final

Le but n’est pas de refaire la v1 complète.
Le but est strictement de retrouver son rendu visuel sur les graphes via :
- géométrie interne du chart,
- labels Y externes au chart,
- placement propre des labels,
- sans toucher au flux de données actuel.

