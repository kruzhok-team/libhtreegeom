/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a new state does not sit on a drawn route
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

/* no segment of the edge runs through the rect */
static int clear_of(const HTreeEdge* e, const HTreeRect* r)
{
	HTreePoint a[LC_MAX_CHAIN];
	int n = lc_edge_chain(e, a, LC_MAX_CHAIN);
	for (int i = 0; i + 1 < n; i++) {
		if (lc_segment_crosses_rect(a[i], a[i + 1], r)) return 0;
	}
	return 1;
}

/* the drawn row a -> b -> c -> d, the drawn way back from d to a running
   under the row */
static HTDocument* drawn_row(HTree** tree, HTreeNode** sm, HTreeNode** a, HTreeNode** b, HTreeNode** c,
							 HTreeNode** d, HTreeEdge** back)
{
	HTDocument* doc = lc_document(tree);
	HTreeEdge* e;
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*sm, 0, 0, 1250, 320);
	*a = lc_state(*sm, "a");
	htree_node_set_rect(*a, 50, 50, 200, 150);
	*b = lc_state(*sm, "b");
	htree_node_set_rect(*b, 350, 50, 200, 150);
	*c = lc_state(*sm, "c");
	htree_node_set_rect(*c, 650, 50, 200, 150);
	*d = lc_state(*sm, "d");
	htree_node_set_rect(*d, 950, 100, 200, 150);
	htree_add_node(*tree, *sm);
	e = lc_edge(*tree, "e-a-b", *a, *b, 0.0, 0.0);
	htree_edge_set_points(e, 250, 100, 350, 100);
	e = lc_edge(*tree, "e-b-c", *b, *c, 0.0, 0.0);
	htree_edge_set_points(e, 550, 100, 650, 100);
	e = lc_edge(*tree, "e-c-d", *c, *d, 0.0, 0.0);
	htree_edge_set_points(e, 850, 150, 950, 150);
	*back = lc_edge(*tree, "e-d-a", *d, *a, 0.0, 0.0);
	htree_edge_set_points(*back, 950, 250, 250, 190);
	(*back)->polyline = htree_new_polyline_coord(300, 250);
	return doc;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *b, *c, *d, *r;
	HTreeEdge* back;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* a new state reached from a and back */
	doc = drawn_row(&tree, &sm, &a, &b, &c, &d, &back);
	r = lc_state(sm, "r");
	lc_edge(tree, "e-a-r", a, r, 0.0, 0.0);
	lc_edge(tree, "e-r-a", r, a, 0.0, 0.0);
	printf("=== under the row ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(a->rect, 50, 50, 200, 150) && lc_same_rect(b->rect, 350, 50, 200, 150) &&
			 lc_same_rect(c->rect, 650, 50, 200, 150) && lc_same_rect(d->rect, 950, 100, 200, 150),
			 "the drawn states stay", "under the row");
	lc_check(back->polyline && fabs(back->polyline->point.x - 300) < LC_EPS &&
			 fabs(back->polyline->point.y - 250) < LC_EPS, "the drawn way back stays", "under the row");
	lc_check(r->rect && clear_of(back, r->rect), "the new state off the drawn route", "under the row");
	lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "under the row");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* a new state after b: the new edge from a bends in the band of b */
	doc = drawn_row(&tree, &sm, &a, &b, &c, &d, &back);
	r = lc_state(sm, "r");
	lc_edge(tree, "e-b-r", b, r, 0.0, 0.0);
	{
		HTreeEdge* ar = lc_edge(tree, "e-a-r", a, r, 40.0, 20.0);
		int off = 1;
		printf("=== a bend under the row ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(b->rect, 350, 50, 200, 150) && lc_same_rect(d->rect, 950, 100, 200, 150),
				 "the drawn states stay", "a bend under the row");
		lc_check(r->rect && clear_of(back, r->rect), "the new state off the drawn route", "a bend under the row");
		for (const HTreePolyline* pl = ar->polyline; pl; pl = pl->next) {
			if (pl->point.x > 300 && pl->point.x < 950 && fabs(pl->point.y - 250) < opts.padding - LC_EPS) off = 0;
		}
		lc_check(ar->polyline && off, "the bends off the drawn route", "a bend under the row");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "a bend under the row");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
