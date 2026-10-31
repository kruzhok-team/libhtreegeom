/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a full layout as the seed of the arrange
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
#include <string.h>

#define MAX_NODES 16

/* the trees of the tests 25-28: the chain, the cycle, the diamond, the long edge */
static HTDocument* sample(int k, HTree** tree)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* b = lc_state(sm, "b");
	HTreeNode* c = lc_state(sm, "c");
	HTreeNode* d = lc_state(sm, "d");
	htree_add_node(*tree, sm);
	lc_edge(*tree, "e-init-a", init, a, 0.0, 0.0);
	switch (k) {
	case 0:
		lc_edge(*tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(*tree, "e-b-c", b, c, 0.0, 0.0);
		lc_edge(*tree, "e-c-d", c, d, 0.0, 0.0);
		break;
	case 1:
		lc_edge(*tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(*tree, "e-b-c", b, c, 0.0, 0.0);
		lc_edge(*tree, "e-c-a", c, a, 0.0, 0.0);
		lc_edge(*tree, "e-c-d", c, d, 0.0, 0.0);
		break;
	case 2:
		lc_edge(*tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(*tree, "e-a-c", a, c, 0.0, 0.0);
		lc_edge(*tree, "e-b-d", b, d, 0.0, 0.0);
		lc_edge(*tree, "e-c-d", c, d, 0.0, 0.0);
		break;
	default:
		lc_edge(*tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(*tree, "e-b-c", b, c, 0.0, 0.0);
		lc_edge(*tree, "e-c-d", c, d, 0.0, 0.0);
		lc_edge(*tree, "e-a-d", a, d, 120.0, 20.0);
		break;
	}
	return doc;
}

/* the relative order of every pair of children across both axes */
static int arrangement(const HTreeNode* sm, int* out)
{
	const HTreeNode* n[MAX_NODES];
	int count = 0, k = 0;
	for (const HTreeNode* c = sm->children; c && count < MAX_NODES; c = c->next) n[count++] = c;
	for (int i = 0; i < count; i++) {
		for (int j = i + 1; j < count; j++) {
			double dx = lc_cx(n[j]) - lc_cx(n[i]), dy = lc_cy(n[j]) - lc_cy(n[i]);
			out[k++] = fabs(dx) < LC_EPS ? 0 : (dx > 0 ? 1 : -1);
			out[k++] = fabs(dy) < LC_EPS ? 0 : (dy > 0 ? 1 : -1);
		}
	}
	return k;
}

static int same_geometry(const HTreeNode* a, const HTreeNode* b)
{
	for (; a && b; a = a->next, b = b->next) {
		HTreeRect ra, rb;
		lc_bounds(a, &ra);
		lc_bounds(b, &rb);
		if (!lc_same_rect(&ra, rb.x, rb.y, rb.width, rb.height)) return 0;
		if (!same_geometry(a->children, b->children)) return 0;
	}
	return !a && !b;
}

int main(void)
{
	const char* names[] = {"chain", "cycle", "diamond", "long edge"};
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);
	for (int k = 0; k < 4; k++) {
		HTree* tree;
		HTDocument* doc = sample(k, &tree);
		HTDocument* again;
		int before[MAX_NODES * MAX_NODES], after[MAX_NODES * MAX_NODES], n;
		printf("=== %s ===\n", names[k]);
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		n = arrangement(tree->nodes, before);
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(arrangement(tree->nodes, after) == n && memcmp(before, after, n * sizeof(int)) == 0,
				 "the arrangement of the full layout kept", names[k]);
		again = htree_copy_document(doc);
		printf("arrange again: %d\n", htree_arrange_document_geometry(again, 1, &opts));
		lc_check(same_geometry(doc->trees->nodes, again->trees->nodes), "the second arrange changes nothing", names[k]);
		lc_check(lc_count_crossings(tree) == 0, "no crossings", names[k]);
		htree_print_document(doc);
		htree_destroy_document(again);
		htree_destroy_document(doc);
	}
	return lc_failures ? 1 : 0;
}
