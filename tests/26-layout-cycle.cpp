/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a cycle without roles
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
	int backward = 0;

	htree_add_node(tree, sm);
	lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	lc_edge(tree, "e-b-c", b, c, 0.0, 0.0);
	lc_edge(tree, "e-c-a", c, a, 0.0, 0.0);

	htree_default_layout_options(&opts);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	for (HTreeEdge* e = tree->edges; e; e = e->next) {
		if (lc_cy(e->source) > lc_cy(e->target)) backward++;
	}
	lc_check(backward == 1, "one backward edge", "sm");
	lc_check(lc_cy(a) < lc_cy(b) && lc_cy(b) < lc_cy(c), "three layers in the model order", "sm");
	lc_check(lc_count_crossings(tree) == 0, "no crossings", "sm");
	lc_check(fabs(lc_forward_fraction(tree, 1) - 2.0 / 3.0) < LC_EPS, "forward fraction", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
