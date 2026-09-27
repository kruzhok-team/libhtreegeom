/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the degenerate documents
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

static void finish(HTDocument* doc, HTree* tree, const char* id, int res)
{
	printf("=== %s ===\n", id);
	printf("reconstruct: %d\n", res);
	lc_check(res == HTREE_OK, "reconstruct", id);
	lc_check(lc_all_finite(tree), "finite geometry", id);
	lc_check(lc_check_containment(tree->nodes, 0.0) == 0, "containment", id);
	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTLayoutOptions opts;
	HTreeNode* sm;

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;

	/* an empty state machine */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	finish(doc, tree, "empty", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* the points only */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
		lc_edge(tree, "e-init-fin", init, fin, 0.0, 0.0);
	}
	finish(doc, tree, "points", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* a single state */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	lc_state(sm, "a");
	finish(doc, tree, "single", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* the self-loops only */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* a = lc_state(sm, "a");
		lc_edge(tree, "e-a-a", a, a, 0.0, 0.0);
		lc_edge(tree, "e-a-a-2", a, a, 40.0, 20.0);
	}
	finish(doc, tree, "loops", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* two cycles without roles */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* b = lc_state(sm, "b");
		HTreeNode* c = lc_state(sm, "c");
		HTreeNode* d = lc_state(sm, "d");
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(tree, "e-b-a", b, a, 0.0, 0.0);
		lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);
		lc_edge(tree, "e-d-c", d, c, 0.0, 0.0);
	}
	finish(doc, tree, "cycles", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* the parallel edges */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* b = lc_state(sm, "b");
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(tree, "e-a-b-2", a, b, 0.0, 0.0);
		lc_check(lc_count_crossings(tree) == 0, "no crossings", "parallel");
	}
	finish(doc, tree, "parallel", htree_reconstruct_document_geometry(doc, 1, &opts));

	/* the root without a rect stays without one */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* b = lc_state(sm, "b");
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	}
	{
		int res = htree_reconstruct_document_geometry(doc, 0, &opts);
		lc_check(sm->rect == NULL, "no root rect", "no-border");
		finish(doc, tree, "no-border", res);
	}

	/* a submachine state with the entry and exit points on its border */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	{
		HTreeNode* a = lc_state(sm, "a");
		HTreeNode* sub = lc_node(sm, htSimpleNode, "sub", htRoleSubmachine, 0.0, 0.0);
		HTreeNode* entry = lc_point(sub, "entry", htRoleEntryPoint);
		HTreeNode* exit = lc_point(sub, "exit", htRoleExitPoint);
		HTreeNode* b = lc_state(sm, "b");
		int res;
		lc_edge(tree, "e-a-entry", a, entry, 0.0, 0.0);
		lc_edge(tree, "e-exit-b", exit, b, 0.0, 0.0);
		res = htree_reconstruct_document_geometry(doc, 1, &opts);
		lc_check(entry->point && sub->rect && fabs(entry->point->y - sub->rect->y) < LC_EPS,
				 "the entry point on the top border", "submachine");
		lc_check(exit->point && sub->rect && fabs(exit->point->y - sub->rect->y - sub->rect->height) < LC_EPS,
				 "the exit point on the bottom border", "submachine");
		lc_check(lc_cy(a) < lc_cy(sub) && lc_cy(sub) < lc_cy(b), "the flow through the submachine", "submachine");
		finish(doc, tree, "submachine", res);
	}

	/* a top-level list without a tree root */
	doc = lc_document(&tree);
	{
		HTreeNode* a = lc_node(NULL, htSimpleNode, "a", htRoleNone, 0.0, 0.0);
		HTreeNode* b = lc_node(NULL, htSimpleNode, "b", htRoleNone, 0.0, 0.0);
		htree_add_node(tree, a);
		htree_add_node(tree, b);
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	}
	finish(doc, tree, "list", htree_reconstruct_document_geometry(doc, 0, &opts));

	return lc_failures ? 1 : 0;
}
