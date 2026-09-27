/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the comments beside their elements
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
	HTreeNode* c1 = lc_node(sm, htComment, "c1", htRoleNone, 100.0, 40.0);
	HTreeNode* c2 = lc_node(sm, htComment, "c2", htRoleNone, 100.0, 40.0);
	HTreeEdge* link;

	htree_node_set_rect(sm, 0, 0, 100, 100);
	htree_add_node(tree, sm);
	lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	link = lc_edge(tree, "l-c1-a", c1, a, 0.0, 0.0);

	htree_default_layout_options(&opts);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));

	lc_check(sm->rect != NULL, "the SM border kept", "sm");
	lc_check(c1->rect && a->rect && fabs(c1->rect->x - (a->rect->x + a->rect->width + opts.node_gap)) < LC_EPS &&
			 fabs(c1->rect->y - a->rect->y) < LC_EPS, "the comment beside its element", "c1");
	lc_check(c1->rect && b->rect && !lc_rects_overlap(c1->rect, b->rect), "no overlap with b", "c1");
	lc_check(!link->source_point && !link->target_point && !link->polyline, "the link unrouted", "l-c1-a");
	lc_check(c2->rect && b->rect && c2->rect->y >= b->rect->y + b->rect->height + opts.node_gap - LC_EPS,
			 "the unlinked comment on the shelf", "c2");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
