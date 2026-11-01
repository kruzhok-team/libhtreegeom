/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the orthogonal routing of the transitions against the flow
 * and of a loop
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
	const char* frames[] = {"down", "right", "adaptive"};
	for (int f = 0; f < 3; f++) {
		HTree* tree;
		HTDocument* doc = lc_document(&tree);
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* b = lc_state(sm, "b");
		HTreeNode* c = lc_state(sm, "c");
		HTreeNode* d = lc_state(sm, "d");
		HTLayoutOptions opts;
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-a", init, a, 0.0, 0.0);
		lc_edge(tree, "e-a-b", a, b, 60.0, 20.0);
		lc_edge(tree, "e-b-c", b, c, 60.0, 20.0);
		lc_edge(tree, "e-c-a", c, a, 60.0, 20.0);
		lc_edge(tree, "e-c-b", c, b, 60.0, 20.0);
		lc_edge(tree, "e-a-c", a, c, 60.0, 20.0);
		lc_edge(tree, "e-c-d", c, d, 60.0, 20.0);
		lc_edge(tree, "e-d-d", d, d, 60.0, 20.0);
		htree_default_layout_options(&opts);
		opts.routing = htRoutingOrthogonal;
		if (f < 2) {
			opts.mode = htLayoutFixed;
			opts.direction = f == 0 ? htFlowDown : htFlowRight;
		}
		printf("=== %s ===\n", frames[f]);
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", frames[f]);
		lc_check(lc_count_shared_runs(tree) == 0, "no runs on each other", frames[f]);
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", frames[f]);
		lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", frames[f]);
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", frames[f]);
		lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", frames[f]);
		htree_print_document(doc);
		htree_destroy_document(doc);
	}
	return lc_failures ? 1 : 0;
}
