/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the entry and exit points of a machine are laid out
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
	HTDocument* doc;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);
	opts.mode = htLayoutFixed;
	opts.direction = htFlowRight;

	/* (a) the full layout of a machine with an entry and an exit point */
	{
		HTreeNode *sm, *in, *a, *out;
		HTreeEdge *e1, *e2;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		in = lc_point(sm, "in", htRoleEntryPoint);
		a = lc_state(sm, "a");
		out = lc_point(sm, "out", htRoleExitPoint);
		htree_add_node(tree, sm);
		e1 = lc_edge(tree, "e-in-a", in, a, 0.0, 0.0);
		e2 = lc_edge(tree, "e-a-out", a, out, 0.0, 0.0);
		printf("=== full ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(in->point && out->point, "the points placed", "full");
		lc_check(in->point && out->point && lc_cx(in) < lc_cx(a) && lc_cx(a) < lc_cx(out),
				 "the flow in, a, out", "full");
		lc_check(e1->source_point && e1->target_point && e2->source_point && e2->target_point,
				 "the edges routed", "full");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "full");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) the update: a new entry point of a drawn machine */
	{
		HTreeNode *sm, *in, *a, *b;
		HTreeEdge* e;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 0, 0, 750, 300);
		a = lc_state(sm, "a");
		htree_node_set_rect(a, 50, 50, 300, 200);
		b = lc_state(sm, "b");
		htree_node_set_rect(b, 400, 50, 300, 200);
		in = lc_point(sm, "in", htRoleEntryPoint);
		htree_add_node(tree, sm);
		e = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		htree_edge_set_points(e, 350, 150, 400, 150);
		e = lc_edge(tree, "e-in-a", in, a, 0.0, 0.0);
		printf("=== update ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(a->rect, 50, 50, 300, 200) && lc_same_rect(b->rect, 400, 50, 300, 200),
				 "the drawn states stay", "update");
		lc_check(in->point && lc_cx(in) < a->rect->x, "the point placed before a", "update");
		lc_check(e->source_point && e->target_point, "the edge routed", "update");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
