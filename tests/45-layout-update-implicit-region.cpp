/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the update keeps a region without a rect whose vertices
 * are drawn
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

/* a drawn composite whose single region carries no rect (the implicit
   region of a file) with two drawn children, a drawn neighbour */
static HTDocument* drawn(HTree** tree, HTreeNode** sm, HTreeNode** comp, HTreeNode** r,
						 HTreeNode** s1, HTreeNode** s2, HTreeNode** n)
{
	HTDocument* doc = lc_document(tree);
	HTreeEdge* e;
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*sm, 0, 0, 900, 400);
	*comp = lc_composite(*sm, "comp");
	htree_node_set_rect(*comp, 50, 50, 500, 300);
	*r = lc_region(*comp, "r");
	*s1 = lc_state(*r, "s1");
	htree_node_set_rect(*s1, 100, 100, 200, 100);
	*s2 = lc_state(*r, "s2");
	htree_node_set_rect(*s2, 100, 230, 200, 100);
	*n = lc_state(*sm, "n");
	htree_node_set_rect(*n, 650, 100, 200, 100);
	htree_add_node(*tree, *sm);
	e = lc_edge(*tree, "e-comp-n", *comp, *n, 0.0, 0.0);
	htree_edge_set_points(e, 550, 150, 650, 150);
	return doc;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *comp, *r, *s1, *s2, *n;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) nothing new: nothing moves */
	doc = drawn(&tree, &sm, &comp, &r, &s1, &s2, &n);
	printf("=== implicit region ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(s1->rect, 100, 100, 200, 100) && lc_same_rect(s2->rect, 100, 230, 200, 100),
			 "the drawn children stay", "implicit region");
	lc_check(lc_same_rect(comp->rect, 50, 50, 500, 300) && lc_same_rect(n->rect, 650, 100, 200, 100),
			 "the composite and its neighbour stay", "implicit region");
	lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "implicit region");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* (b) a new region beside the implicit one goes after it */
	doc = drawn(&tree, &sm, &comp, &r, &s1, &s2, &n);
	{
		HTreeNode* r2 = lc_region(comp, "r2");
		HTreeNode* x = lc_state(r2, "x");
		printf("=== new region ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(s1->rect, 100, 100, 200, 100) && lc_same_rect(s2->rect, 100, 230, 200, 100),
				 "the drawn children stay", "new region");
		lc_check(x->rect && r2->rect && r->rect && !lc_rects_overlap(r->rect, r2->rect),
				 "the new region apart from the drawn one", "new region");
		lc_check(lc_check_containment(comp, 0.0) == 0, "containment", "new region");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
