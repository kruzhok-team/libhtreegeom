/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange sizes and the unseeded vertices
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

	/* (a) a large seed rect is the minimum size, a small one is not */
	{
		HTreeNode *sm, *a, *b;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		a = lc_node(sm, htSimpleNode, "a", htRoleNone, 120.0, 80.0);
		b = lc_node(sm, htSimpleNode, "b", htRoleNone, 120.0, 80.0);
		htree_add_node(tree, sm);
		htree_node_set_rect(a, 0, 0, 500, 300);
		htree_node_set_rect(b, 700, 0, 50, 30);
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		printf("=== sizes ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(a->rect->width >= 500.0 - LC_EPS && a->rect->height >= 300.0 - LC_EPS, "a at least its seed", "sizes");
		lc_check(fabs(b->rect->width - 120.0) < LC_EPS && fabs(b->rect->height - 80.0) < LC_EPS,
				 "b keeps its preset size", "sizes");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a seeded composite: at least its seed rect, the title band of the
	   full layout */
	{
		HTreeNode *sm, *comp, *a, *b;
		double title_full;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		comp = lc_node(sm, htCompositeNode, "comp", htRoleNone, 100.0, 40.0);
		a = lc_state(comp, "a");
		b = lc_state(comp, "b");
		htree_add_node(tree, sm);
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		printf("=== composite ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		title_full = a->rect->y - comp->rect->y;
		htree_node_set_rect(comp, comp->rect->x, comp->rect->y, 1600, 900);
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(comp->rect->width >= 1600.0 - LC_EPS && comp->rect->height >= 900.0 - LC_EPS,
				 "the composite at least its seed", "composite");
		lc_check(fabs(a->rect->y - comp->rect->y - title_full) < LC_EPS, "the title band of the full layout", "composite");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "composite");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (c) an unseeded state between two seeded ones */
	{
		HTreeNode *sm, *a, *x, *b;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		a = lc_state(sm, "a");
		x = lc_state(sm, "x");
		b = lc_state(sm, "b");
		htree_add_node(tree, sm);
		htree_node_set_rect(a, 0, 0, 100, 60);
		htree_node_set_rect(b, 400, 0, 100, 60);
		lc_edge(tree, "e-a-x", a, x, 0.0, 0.0);
		lc_edge(tree, "e-x-b", x, b, 0.0, 0.0);
		printf("=== unseeded ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(x->rect && lc_cx(a) < lc_cx(x) && lc_cx(x) < lc_cx(b), "x between a and b", "unseeded");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "unseeded");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
