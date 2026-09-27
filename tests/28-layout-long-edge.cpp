/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a long labelled edge over the chain
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
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* b = lc_state(sm, "b");
	HTreeNode* c = lc_state(sm, "c");
	HTreeNode* d = lc_state(sm, "d");
	HTreeEdge* e;

	htree_add_node(tree, sm);
	lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	lc_edge(tree, "e-b-c", b, c, 0.0, 0.0);
	lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);
	e = lc_edge(tree, "e-a-d", a, d, 80.0, 30.0);

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(lc_bends(e) == 2, "two bends", "e-a-d");
	lc_check(e->label_rect && fabs(e->label_rect->width - 80.0) < LC_EPS &&
			 fabs(e->label_rect->height - 30.0) < LC_EPS, "the label size kept", "e-a-d");
	lc_check(!e->label_point, "no label point beside the rect", "e-a-d");
	lc_check(e->label_rect && e->label_rect->y > a->rect->y + a->rect->height &&
			 e->label_rect->y + e->label_rect->height < c->rect->y, "the label in the tail layer", "e-a-d");
	lc_check(e->label_rect && !lc_rects_overlap(e->label_rect, b->rect), "the label beside b", "e-a-d");
	lc_check(lc_count_line_rect_crossings(tree) == 0, "no line-rect crossings", "sm");
	lc_check(lc_count_crossings(tree) == 0, "no crossings", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
