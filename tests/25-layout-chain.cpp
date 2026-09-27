/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the chain from the initial to the final pseudostate
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

int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTLayoutOptions opts;
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* b = lc_state(sm, "b");
	HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
	HTreeEdge* e1;
	HTreeEdge* e2;
	HTreeEdge* e3;

	htree_add_node(tree, sm);
	e1 = lc_edge(tree, "e-init-a", init, a, 0.0, 0.0);
	e2 = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	e3 = lc_edge(tree, "e-b-fin", b, fin, 0.0, 0.0);

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(sm->rect != NULL, "the SM rect created", "sm");
	lc_check(lc_cy(init) < lc_cy(a) && lc_cy(a) < lc_cy(b) && lc_cy(b) < lc_cy(fin), "the flow order", "sm");
	lc_check(fabs(lc_cx(init) - lc_cx(a)) < LC_EPS && fabs(lc_cx(a) - lc_cx(b)) < LC_EPS,
			 "the chain aligned", "sm");
	lc_check(fabs(lc_forward_fraction(tree, 1) - 1.0) < LC_EPS, "forward fraction", "sm");
	lc_check(lc_count_crossings(tree) == 0, "no crossings", "sm");
	lc_check(lc_bends(e1) == 0 && lc_bends(e2) == 0 && lc_bends(e3) == 0, "straight edges", "sm");
	lc_check(e1->source_point && e1->target_point && e3->source_point && e3->target_point, "edge ends", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
