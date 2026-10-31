/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange frame of a container from the spread of its seed
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

/* a composite of two unconnected states under a machine running right */
static HTDocument* column(HTree** tree, HTreeNode** comp, HTreeNode** a, HTreeNode** b, int vertical)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* x = lc_state(sm, "x");
	*comp = lc_composite(sm, "comp");
	*a = lc_state(*comp, "a");
	*b = lc_state(*comp, "b");
	htree_add_node(*tree, sm);
	htree_node_set_rect(x, 0, 100, 100, 60);
	htree_node_set_rect(*comp, 300, 0, 400, 400);
	htree_node_set_rect(*a, 350, 50, 100, 60);
	if (vertical) htree_node_set_rect(*b, 350, 250, 100, 60);
	else htree_node_set_rect(*b, 550, 50, 100, 60);
	lc_edge(*tree, "e-x-comp", x, *comp, 0.0, 0.0);
	return doc;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *comp, *a, *b;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	printf("=== column ===\n");
	doc = column(&tree, &comp, &a, &b, 1);
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(lc_cy(a) < lc_cy(b) && fabs(lc_cx(a) - lc_cx(b)) < LC_EPS, "a above b", "column");
	lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", "column");
	htree_print_document(doc);
	htree_destroy_document(doc);

	printf("=== row ===\n");
	doc = column(&tree, &comp, &a, &b, 0);
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(lc_cx(a) < lc_cx(b) && fabs(lc_cy(a) - lc_cy(b)) < LC_EPS, "a left of b", "row");
	lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", "row");
	htree_print_document(doc);
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
