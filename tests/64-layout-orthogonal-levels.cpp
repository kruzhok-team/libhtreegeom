/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the orthogonal routing across the border of a composite,
 * its inner flow along the outer one or across it
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
	const char* frames[] = {"down", "right", "adaptive", "alternate"};
	for (int f = 0; f < 4; f++) {
		HTree* tree;
		HTDocument* doc = lc_document(&tree);
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* s = lc_state(sm, "s");
		HTreeNode* ch = lc_node(sm, htSimpleNode, "ch", htRoleChoice, 0.0, 0.0);
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* comp = lc_composite(sm, "comp");
		HTreeNode* x = lc_state(comp, "x");
		HTreeNode* y = lc_state(comp, "y");
		HTreeEdge *in, *out, *back;
		HTLayoutOptions opts;
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-s", init, s, 0.0, 0.0);
		lc_edge(tree, "e-s-ch", s, ch, 60.0, 20.0);
		lc_edge(tree, "e-ch-a", ch, a, 60.0, 20.0);
		in = lc_edge(tree, "e-ch-x", ch, x, 60.0, 20.0);
		lc_edge(tree, "e-x-y", x, y, 60.0, 20.0);
		back = lc_edge(tree, "e-y-s", y, s, 60.0, 20.0);
		out = lc_edge(tree, "e-y-a", y, a, 60.0, 20.0);
		htree_default_layout_options(&opts);
		opts.routing = htRoutingOrthogonal;
		if (f < 2) {
			opts.mode = htLayoutFixed;
			opts.direction = f == 0 ? htFlowDown : htFlowRight;
		} else if (f == 3) {
			opts.mode = htLayoutAlternate;
		}
		printf("=== %s ===\n", frames[f]);
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", frames[f]);
		lc_check(lc_count_border_crossings(in, comp->rect) == 1 && lc_count_border_crossings(back, comp->rect) == 1 &&
				 lc_count_border_crossings(out, comp->rect) == 1, "the composite border crossed once", frames[f]);
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", frames[f]);
		lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", frames[f]);
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", frames[f]);
		lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", frames[f]);
		htree_print_document(doc);
		htree_destroy_document(doc);
	}
	return lc_failures ? 1 : 0;
}
