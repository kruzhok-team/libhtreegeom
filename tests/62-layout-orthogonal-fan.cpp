/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the orthogonal routing of the fans - the ends of a state
 * spread on its side, the ends of a choice at its tip, the runs in channels
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 * ----------------------------------------------------------------------------- */
#include "layout-check.h"

#define LABEL_W 60.0
#define LABEL_H 20.0
#define FANS 3

/* a state and a choice, each to three states */
static HTDocument* fans(HTree** tree, HTreeNode** s, HTreeNode** ch)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	const char* ids[2][FANS] = {{"a", "b", "c"}, {"d", "f", "g"}};
	char id[32];
	*s = lc_state(sm, "s");
	*ch = lc_node(sm, htSimpleNode, "ch", htRoleChoice, 0.0, 0.0);
	htree_add_node(*tree, sm);
	lc_edge(*tree, "e-init-s", init, *s, 0.0, 0.0);
	for (int k = 0; k < 2; k++) {
		for (int i = 0; i < FANS; i++) {
			HTreeNode* to = k == 0 && i == FANS - 1 ? *ch : lc_state(sm, ids[k][i]);
			snprintf(id, sizeof(id), "e-%s-%s", k ? "ch" : "s", to->id);
			lc_edge(*tree, id, k ? *ch : *s, to, LABEL_W, LABEL_H);
		}
	}
	return doc;
}

/* the ends leaving a node: on its border, on one side, apart */
static int spread(const HTree* tree, const HTreeNode* n)
{
	HTreePoint p[LC_MAX_CHAIN];
	int count = 0;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		if (e->source != n || !e->source_point) continue;
		if (lc_rect_side(*e->source_point, n->rect) != 0) return 0;
		p[count++] = *e->source_point;
	}
	for (int i = 0; i < count; i++) {
		for (int j = i + 1; j < count; j++) {
			if (lc_same_point(p[i], p[j])) return 0;
			if (fabs(p[i].x - p[j].x) > LC_EPS && fabs(p[i].y - p[j].y) > LC_EPS) return 0;
		}
	}
	return count > 1;
}

/* the ends leaving a choice: one tip of its rhombus */
static int one_tip(const HTree* tree, const HTreeNode* n)
{
	const HTreeRect* r = n->rect;
	HTreePoint tips[4];
	const HTreePoint* first = NULL;
	tips[0].x = r->x; tips[0].y = r->y + r->height / 2.0;
	tips[1].x = r->x + r->width; tips[1].y = tips[0].y;
	tips[2].x = r->x + r->width / 2.0; tips[2].y = r->y;
	tips[3].x = tips[2].x; tips[3].y = r->y + r->height;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		int tip = 0;
		if (e->source != n || !e->source_point) continue;
		for (int k = 0; k < 4; k++) tip |= lc_same_point(*e->source_point, tips[k]);
		if (!tip || (first && !lc_same_point(*first, *e->source_point))) return 0;
		first = e->source_point;
	}
	return first != NULL;
}

int main(void)
{
	const char* frames[] = {"down", "right", "adaptive"};
	for (int f = 0; f < 3; f++) {
		HTree* tree;
		HTreeNode *s, *ch;
		HTDocument* doc = fans(&tree, &s, &ch);
		HTLayoutOptions opts;
		htree_default_layout_options(&opts);
		opts.routing = htRoutingOrthogonal;
		if (f < 2) {
			opts.mode = htLayoutFixed;
			opts.direction = f == 0 ? htFlowDown : htFlowRight;
		}
		printf("=== %s ===\n", frames[f]);
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", frames[f]);
		lc_check(spread(tree, s), "the ends of the state spread on its side", frames[f]);
		lc_check(one_tip(tree, ch), "the ends of the choice at one tip", frames[f]);
		lc_check(lc_count_shared_runs(tree) == 0, "no runs on each other", frames[f]);
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", frames[f]);
		lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", frames[f]);
		lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", frames[f]);
		lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", frames[f]);
		htree_print_document(doc);
		htree_destroy_document(doc);
	}
	return lc_failures ? 1 : 0;
}
