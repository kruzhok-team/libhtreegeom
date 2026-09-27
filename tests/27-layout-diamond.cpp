/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the diamond
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

	htree_add_node(tree, sm);
	lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	lc_edge(tree, "e-a-c", a, c, 0.0, 0.0);
	lc_edge(tree, "e-b-d", b, d, 0.0, 0.0);
	lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);

	htree_default_layout_options(&opts);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(lc_count_crossings(tree) == 0, "no crossings", "sm");
	lc_check(fabs(lc_cy(b) - lc_cy(c)) < LC_EPS, "b and c in one layer", "sm");
	lc_check(lc_cx(b) < lc_cx(c), "b before c", "sm");
	lc_check(fabs(lc_cx(d) - lc_cx(a)) < LC_EPS, "d under a", "sm");
	lc_check(fabs(lc_cx(a) - (lc_cx(b) + lc_cx(c)) / 2.0) < LC_EPS, "a centred over b and c", "sm");
	lc_check(fabs(lc_forward_fraction(tree, 1) - 1.0) < LC_EPS, "forward fraction", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
