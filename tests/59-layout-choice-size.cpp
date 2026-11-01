/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a choice is a small square
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

/* a -> c (a choice) -> b, c -> d: the choice in the middle of the flow */
static HTDocument* fork_tree(HTree** tree, HTreeNode** sm, HTreeNode** a, HTreeNode** c,
							 HTreeNode** b, HTreeNode** d)
{
	HTDocument* doc = lc_document(tree);
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	*a = lc_state(*sm, "a");
	*c = lc_node(*sm, htSimpleNode, "c", htRoleChoice, 0.0, 0.0);
	*b = lc_state(*sm, "b");
	*d = lc_state(*sm, "d");
	htree_add_node(*tree, *sm);
	lc_edge(*tree, "e-a-c", *a, *c, 0.0, 0.0);
	lc_edge(*tree, "e-c-b", *c, *b, 60.0, 20.0);
	lc_edge(*tree, "e-c-d", *c, *d, 60.0, 20.0);
	return doc;
}

static int square(const HTreeNode* n, double side)
{
	return n->rect && fabs(n->rect->width - side) < LC_EPS && fabs(n->rect->height - side) < LC_EPS;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *c, *b, *d;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	printf("=== full ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(square(c, opts.choice_size), "the choice a square of the choice size", "full");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "full");
	htree_print_document(doc);
	htree_destroy_document(doc);

	printf("=== preset ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_min_size(c, 60.0, 20.0);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(square(c, 60.0), "a preset: the square of its larger side", "preset");
	htree_destroy_document(doc);

	printf("=== seed ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_rect(a, 0, 0, 100, 60);
	htree_node_set_rect(c, 200, 0, 100, 50);
	htree_node_set_rect(b, 400, 0, 100, 60);
	htree_node_set_rect(d, 400, 200, 100, 60);
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(square(c, 100.0), "a seed: the square of its larger side", "seed");
	htree_destroy_document(doc);

	printf("=== fill-in ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_rect(sm, 0, 0, 1000, 600);
	printf("fill-in: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	lc_check(square(c, opts.choice_size), "the shelf: a square of the choice size", "fill-in");
	htree_print_document(doc);
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
