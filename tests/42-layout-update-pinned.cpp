/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the update keeps the drawn vertices pinned and runs the
 * new lines along the drawn bands
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

/* the drawn chain a -> b -> c running right in a bordered machine */
static HTDocument* drawn_chain(HTree** tree, HTreeNode** sm, HTreeNode** a, HTreeNode** b, HTreeNode** c,
							   HTreeEdge** ab)
{
	HTDocument* doc = lc_document(tree);
	HTreeEdge* bc;
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*sm, 0, 0, 1100, 300);
	*a = lc_state(*sm, "a");
	htree_node_set_rect(*a, 50, 50, 300, 200);
	*b = lc_state(*sm, "b");
	htree_node_set_rect(*b, 400, 50, 300, 200);
	*c = lc_state(*sm, "c");
	htree_node_set_rect(*c, 750, 50, 300, 200);
	htree_add_node(*tree, *sm);
	*ab = lc_edge(*tree, "e-a-b", *a, *b, 0.0, 0.0);
	htree_edge_set_points(*ab, 350, 150, 400, 150);
	bc = lc_edge(*tree, "e-b-c", *b, *c, 0.0, 0.0);
	htree_edge_set_points(bc, 700, 150, 750, 150);
	return doc;
}

static void check_drawn(const HTreeNode* a, const HTreeNode* b, const HTreeNode* c, const char* id)
{
	lc_check(lc_same_rect(a->rect, 50, 50, 300, 200) && lc_same_rect(b->rect, 400, 50, 300, 200) &&
			 lc_same_rect(c->rect, 750, 50, 300, 200), "the drawn states stay", id);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *b, *c;
	HTreeEdge* ab;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) a new state after c, also reached from a over the drawn row */
	doc = drawn_chain(&tree, &sm, &a, &b, &c, &ab);
	{
		HTreeNode* d = lc_state(sm, "d");
		lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);
		lc_edge(tree, "e-a-d", a, d, 0.0, 0.0);
		printf("=== long edge ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		check_drawn(a, b, c, "long edge");
		lc_check(ab->source_point && fabs(ab->source_point->x - 350.0) < LC_EPS &&
				 ab->target_point && fabs(ab->target_point->x - 400.0) < LC_EPS, "the drawn edge kept", "long edge");
		lc_check(d->rect && d->rect->x >= c->rect->x + c->rect->width - LC_EPS, "d after c", "long edge");
		lc_check(lc_count_crossings(tree) == 0, "no crossings", "long edge");
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no line through a state", "long edge");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "long edge");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a labelled new edge whose label slot falls in a drawn band */
	doc = drawn_chain(&tree, &sm, &a, &b, &c, &ab);
	{
		HTreeNode* x = lc_state(sm, "x");
		lc_edge(tree, "e-a-x", a, x, LABEL_W, LABEL_H);
		printf("=== label slot ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		check_drawn(a, b, c, "label slot");
		lc_check(x->rect != NULL, "x placed", "label slot");
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no line through a state", "label slot");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "label slot");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (c) a new history beside a drawn initial point */
	doc = drawn_chain(&tree, &sm, &a, &b, &c, &ab);
	{
		HTreeNode* init = lc_point(sm, "i", htRoleInitial);
		HTreeNode* h = lc_point(sm, "h", htRoleDeepHistory);
		HTreeEdge* ia;
		htree_node_set_point(init, 25, 150);
		ia = lc_edge(tree, "e-i-a", init, a, 0.0, 0.0);
		htree_edge_set_points(ia, 25, 150, 50, 150);
		lc_edge(tree, "e-h-a", h, a, 0.0, 0.0);
		printf("=== history ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		check_drawn(a, b, c, "history");
		lc_check(init->point && fabs(init->point->x - 25.0) < LC_EPS && fabs(init->point->y - 150.0) < LC_EPS,
				 "the drawn initial stays", "history");
		lc_check(h->point != NULL, "the history placed", "history");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (d) a taller drawn state in the row: a line from a toward the band
	   centre would cut its corner */
	doc = drawn_chain(&tree, &sm, &a, &b, &c, &ab);
	{
		HTreeNode* d = lc_state(sm, "d");
		htree_node_set_rect(b, 400, 50, 300, 300);
		htree_node_set_rect(sm, 0, 0, 1100, 400);
		lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);
		lc_edge(tree, "e-a-d", a, d, 0.0, 0.0);
		printf("=== tall neighbour ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(b->rect, 400, 50, 300, 300), "the tall state stays", "tall neighbour");
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no line through a state", "tall neighbour");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
