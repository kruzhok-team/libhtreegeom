/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a new cycle in the update keeps the drawn flow
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

#define LABEL_W 40.0
#define LABEL_H 20.0
#define WIDTH_LIMIT 1600.0   /* the drawn row plus one new state */

/* the drawn chain a -> b -> c running right in a bordered machine */
static HTDocument* drawn_chain(HTree** tree, HTreeNode** sm, HTreeNode** a, HTreeNode** b, HTreeNode** c)
{
	HTDocument* doc = lc_document(tree);
	HTreeEdge* e;
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*sm, 0, 0, 1100, 300);
	*a = lc_state(*sm, "a");
	htree_node_set_rect(*a, 50, 50, 300, 200);
	*b = lc_state(*sm, "b");
	htree_node_set_rect(*b, 400, 50, 300, 200);
	*c = lc_state(*sm, "c");
	htree_node_set_rect(*c, 750, 50, 300, 200);
	htree_add_node(*tree, *sm);
	e = lc_edge(*tree, "e-a-b", *a, *b, 0.0, 0.0);
	htree_edge_set_points(e, 350, 150, 400, 150);
	e = lc_edge(*tree, "e-b-c", *b, *c, 0.0, 0.0);
	htree_edge_set_points(e, 700, 150, 750, 150);
	return doc;
}

/* the drawing stays, the machine keeps its corner and stays compact, no
   line runs through a state */
static void check_flow(const HTree* tree, const HTreeNode* sm, const HTreeNode* a, const HTreeNode* b,
					   const HTreeNode* c, const char* id)
{
	lc_check(lc_same_rect(a->rect, 50, 50, 300, 200) && lc_same_rect(b->rect, 400, 50, 300, 200) &&
			 lc_same_rect(c->rect, 750, 50, 300, 200), "the drawn states stay", id);
	lc_check(sm->rect && fabs(sm->rect->x) < LC_EPS && fabs(sm->rect->y) < LC_EPS &&
			 sm->rect->width < WIDTH_LIMIT, "the machine keeps its corner and stays compact", id);
	lc_check(lc_count_line_rect_crossings(tree) == 0, "no line through a state", id);
	lc_check(lc_check_containment(sm, 0.0) == 0, "containment", id);
}

static void cycle(int labels, const char* back, const char* id)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *b, *c, *d, *t;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	d = lc_state(sm, "d");
	t = back[0] == 'a' ? a : b;
	lc_edge(tree, "e-c-d", c, d, labels ? LABEL_W : 0.0, labels ? LABEL_H : 0.0);
	lc_edge(tree, "e-d-back", d, t, labels ? LABEL_W : 0.0, labels ? LABEL_H : 0.0);
	printf("=== %s ===\n", id);
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	check_flow(tree, sm, a, b, c, id);
	lc_check(d->rect && d->rect->x >= c->rect->x + c->rect->width + opts.node_gap - LC_EPS, "d after c", id);
	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *b, *c;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	cycle(0, "a", "back to the first");
	cycle(1, "a", "back to the first, labelled");
	cycle(0, "b", "back to the middle");

	/* a two-cycle through a new state */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeNode* x = lc_state(sm, "x");
		lc_edge(tree, "e-a-x", a, x, 0.0, 0.0);
		lc_edge(tree, "e-x-a", x, a, 0.0, 0.0);
		printf("=== two-cycle ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		check_flow(tree, sm, a, b, c, "two-cycle");
		lc_check(x->rect != NULL, "x placed", "two-cycle");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* a new transition back between two drawn states */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeEdge* ca = lc_edge(tree, "e-c-a", c, a, 0.0, 0.0);
		printf("=== drawn back edge ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		check_flow(tree, sm, a, b, c, "drawn back edge");
		lc_check(ca->source_point && ca->target_point && ca->polyline, "routed around", "drawn back edge");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
