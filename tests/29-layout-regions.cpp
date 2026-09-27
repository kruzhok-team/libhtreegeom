/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the regions of a composite
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
	HTreeNode* c = lc_node(sm, htCompositeNode, "c", htRoleNone, 200.0, 40.0);
	HTreeNode* r1 = lc_region(c, "r1");
	HTreeNode* s1 = lc_state(r1, "s1");
	HTreeNode* s2 = lc_state(r1, "s2");
	HTreeNode* r2 = lc_region(c, "r2");
	HTreeNode* s3 = lc_state(r2, "s3");
	HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);

	htree_add_node(tree, sm);
	lc_edge(tree, "e-init-c", init, c, 0.0, 0.0);
	lc_edge(tree, "e-s1-s2", s1, s2, 0.0, 0.0);
	lc_edge(tree, "e-c-fin", c, fin, 0.0, 0.0);

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(r1->rect && r2->rect && fabs(r1->rect->width - r2->rect->width) < LC_EPS &&
			 fabs(r1->rect->x - r2->rect->x) < LC_EPS, "the region bands aligned", "c");
	lc_check(r1->rect && r2->rect &&
			 fabs(r2->rect->y - (r1->rect->y + r1->rect->height + opts.node_gap)) < LC_EPS,
			 "the regions stacked with the gap", "c");
	lc_check(r1->rect && c->rect && r1->rect->y >= c->rect->y + 40.0 + opts.padding - LC_EPS,
			 "the regions below the title block", "c");
	lc_check(s1->rect && s2->rect && s1->rect->x + s1->rect->width <= s2->rect->x + LC_EPS &&
			 fabs(lc_cy(s1) - lc_cy(s2)) < LC_EPS, "the region content flows right", "r1");
	lc_check(s3->rect && r2->rect && lc_rect_inside(s3->rect, r2->rect, opts.padding), "s3 inside r2", "r2");
	lc_check(lc_cy(init) < lc_cy(c) && lc_cy(c) < lc_cy(fin), "the flow order", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
