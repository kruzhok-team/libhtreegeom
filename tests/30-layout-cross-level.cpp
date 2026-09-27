/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a transition crossing two composite borders
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
	HTreeNode* a = lc_composite(sm, "a");
	HTreeNode* ra = lc_region(a, "ra");
	HTreeNode* a1 = lc_state(ra, "a1");
	HTreeNode* a2 = lc_state(ra, "a2");
	HTreeNode* b = lc_composite(sm, "b");
	HTreeNode* rb = lc_region(b, "rb");
	HTreeNode* b1 = lc_state(rb, "b1");
	HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
	HTreeEdge* e;

	htree_add_node(tree, sm);
	lc_edge(tree, "e-init-a", init, a, 0.0, 0.0);
	lc_edge(tree, "e-a1-a2", a1, a2, 0.0, 0.0);
	e = lc_edge(tree, "e-a2-b1", a2, b1, 0.0, 0.0);
	lc_edge(tree, "e-b-fin", b, fin, 0.0, 0.0);

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(lc_count_border_crossings(e, a->rect) == 1, "one crossing of the a border", "e-a2-b1");
	lc_check(lc_count_border_crossings(e, b->rect) == 1, "one crossing of the b border", "e-a2-b1");
	lc_check(lc_count_line_rect_crossings(tree) == 0, "no line-rect crossings", "sm");
	lc_check(lc_cy(init) < lc_cy(a) && lc_cy(a) < lc_cy(b) && lc_cy(b) < lc_cy(fin), "the flow order", "sm");
	lc_check(lc_cx(a1) < lc_cx(a2), "a1 before a2", "ra");
	lc_check(lc_count_crossings(tree) == 0, "no crossings", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
