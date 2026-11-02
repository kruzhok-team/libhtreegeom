/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange with the orthogonal routes keeps the drawn sides
 * of the transition ends along the flow; the polylines ignore them
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
#include <string.h>
#include "layout-check.h"

#define W 120.0
#define H 80.0
#define CHOICE 40.0

static int same_point(const HTreePoint* p, double x, double y)
{
	return p && fabs(p->x - x) < LC_EPS && fabs(p->y - y) < LC_EPS;
}

/* a point on a side of the rect: 0 left, 1 right, 2 top, 3 bottom */
static int on_side(const HTreePoint* p, const HTreeRect* r, int side)
{
	if (!p) return 0;
	switch (side) {
	case 0: return fabs(p->x - r->x) < LC_EPS;
	case 1: return fabs(p->x - r->x - r->width) < LC_EPS;
	case 2: return fabs(p->y - r->y) < LC_EPS;
	default: return fabs(p->y - r->y - r->height) < LC_EPS;
	}
}

/* a row a b c drawn right: a -> c drawn from the bottom of a to the bottom
   of c, a -> b from the left of a (against the flow), b -> c plain; a
   choice whose branches leave its top and bottom tips */
static HTDocument* drawn(HTree** tree, HTreeNode** v)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeEdge* e;
	const char* ids[] = {"a", "b", "c", "d", "f"};
	const double at[5][2] = {{0, 300}, {400, 300}, {800, 300}, {1200, 0}, {1200, 600}};
	htree_add_node(*tree, sm);
	for (int i = 0; i < 5; i++) {
		v[i] = lc_state(sm, ids[i]);
		htree_node_set_rect(v[i], at[i][0], at[i][1], W, H);
	}
	v[5] = lc_node(sm, htSimpleNode, "ch", htRoleChoice, 0.0, 0.0);
	htree_node_set_rect(v[5], 1000, 320, CHOICE, CHOICE);
	e = lc_edge(*tree, "e-a-b", v[0], v[1], 60.0, 20.0);
	htree_edge_set_points(e, 0, 340, 400, 340);
	lc_edge(*tree, "e-b-c", v[1], v[2], 60.0, 20.0);
	e = lc_edge(*tree, "e-a-c", v[0], v[2], 60.0, 20.0);
	htree_edge_set_points(e, 60, 380, 860, 380);
	lc_edge(*tree, "e-c-ch", v[2], v[5], 0.0, 0.0);
	e = lc_edge(*tree, "e-ch-d", v[5], v[3], 60.0, 20.0);
	htree_edge_set_points(e, 1020, 320, 1200, 40);
	e = lc_edge(*tree, "e-ch-f", v[5], v[4], 60.0, 20.0);
	htree_edge_set_points(e, 1020, 360, 1200, 640);
	return doc;
}

static HTreeEdge* edge(HTree* tree, const char* id)
{
	for (HTreeEdge* e = tree->edges; e; e = e->next) if (strcmp(e->id, id) == 0) return e;
	return NULL;
}

int main(void)
{
	HTree *tree, *tree_plain;
	HTDocument *doc, *plain;
	HTreeNode *v[6], *w[6];
	HTLayoutOptions opts;

	/* (a) the orthogonal arrange: the drawn sides along the flow kept */
	htree_default_layout_options(&opts);
	opts.routing = htRoutingOrthogonal;
	doc = drawn(&tree, v);
	printf("=== orthogonal ===\n");
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	{
		HTreeEdge *ac = edge(tree, "e-a-c"), *ab = edge(tree, "e-a-b");
		HTreeEdge *chd = edge(tree, "e-ch-d"), *chf = edge(tree, "e-ch-f");
		const HTreeRect* ch = v[5]->rect;
		lc_check(on_side(ac->source_point, v[0]->rect, 3) && on_side(ac->target_point, v[2]->rect, 3),
				 "a -> c from the bottom of a to the bottom of c", "orthogonal");
		lc_check(on_side(ab->source_point, v[0]->rect, 1), "a -> b against the flow on its side", "orthogonal");
		lc_check(same_point(chd->source_point, ch->x + ch->width / 2.0, ch->y) &&
				 same_point(chf->source_point, ch->x + ch->width / 2.0, ch->y + ch->height),
				 "the branches from the top and bottom tips", "orthogonal");
		lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", "orthogonal");
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", "orthogonal");
		lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", "orthogonal");
		lc_check(lc_count_shared_runs(tree) == 0, "no runs on each other", "orthogonal");
	}
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* (b) the polylines ignore the drawn sides */
	htree_default_layout_options(&opts);
	doc = drawn(&tree, v);
	plain = drawn(&tree_plain, w);
	for (HTreeEdge* e = tree_plain->edges; e; e = e->next) {
		if (e->source_point) { htree_destroy_point(e->source_point); e->source_point = NULL; }
		if (e->target_point) { htree_destroy_point(e->target_point); e->target_point = NULL; }
	}
	printf("=== polyline ===\n");
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	printf("arrange undrawn: %d\n", htree_arrange_document_geometry(plain, 1, &opts));
	{
		int same = 1;
		for (HTreeEdge *e = tree->edges, *f = tree_plain->edges; e && f; e = e->next, f = f->next) {
			same &= (!e->source_point && !f->source_point) ||
				(e->source_point && f->source_point && lc_same_point(*e->source_point, *f->source_point));
			same &= (!e->target_point && !f->target_point) ||
				(e->target_point && f->target_point && lc_same_point(*e->target_point, *f->target_point));
		}
		lc_check(same, "the routes as without the drawn ends", "polyline");
	}
	htree_print_document(doc);
	htree_destroy_document(plain);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
