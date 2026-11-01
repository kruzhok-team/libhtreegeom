/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the transitions of a choice end at its tips
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

/* every transition end on a choice is the midpoint of a side of its rect */
static int at_tips(const HTree* tree, const HTreeNode* c)
{
	const HTreeRect* r = c->rect;
	double tips[4][2] = {{r->x, r->y + r->height / 2.0}, {r->x + r->width, r->y + r->height / 2.0},
						 {r->x + r->width / 2.0, r->y}, {r->x + r->width / 2.0, r->y + r->height}};
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		const HTreePoint* p = e->source == c ? e->source_point : e->target == c ? e->target_point : NULL;
		int found = 0;
		if (!p) continue;
		for (int k = 0; k < 4; k++) {
			if (fabs(p->x - tips[k][0]) < LC_EPS && fabs(p->y - tips[k][1]) < LC_EPS) found = 1;
		}
		if (!found) return 0;
	}
	return 1;
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
	lc_check(at_tips(tree, c), "the ends at the tips", "full");
	htree_print_document(doc);
	htree_destroy_document(doc);

	printf("=== update ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	{
		HTreeNode* x = lc_state(sm, "x");
		lc_edge(tree, "e-c-x", c, x, 0.0, 0.0);
		printf("update: %d\n", htree_update_document_geometry(doc, 1, &opts));
		lc_check(at_tips(tree, c), "the new end at a tip", "update");
	}
	htree_destroy_document(doc);

	printf("=== arrange ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_rect(a, 0, 100, 100, 60);
	htree_node_set_rect(c, 200, 110, 40, 40);
	htree_node_set_rect(b, 400, 0, 100, 60);
	htree_node_set_rect(d, 400, 250, 100, 60);
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(at_tips(tree, c), "the ends at the tips", "arrange");
	htree_destroy_document(doc);

	printf("=== fill-in ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_rect(sm, 0, 0, 1000, 600);
	htree_node_set_rect(a, 20, 300, 100, 60);
	htree_node_set_rect(c, 300, 310, 40, 40);
	htree_node_set_rect(b, 600, 100, 100, 60);
	htree_node_set_rect(d, 600, 450, 100, 60);
	printf("fill-in: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	lc_check(at_tips(tree, c), "the ends at the tips", "fill-in");
	htree_print_document(doc);
	htree_destroy_document(doc);

	printf("=== drawn ===\n");
	doc = fork_tree(&tree, &sm, &a, &c, &b, &d);
	htree_node_set_rect(sm, 0, 0, 1000, 600);
	htree_node_set_rect(a, 20, 300, 100, 60);
	htree_node_set_rect(c, 300, 320, 60, 20);
	htree_node_set_rect(b, 600, 100, 100, 60);
	htree_node_set_rect(d, 600, 450, 100, 60);
	printf("update: %d\n", htree_update_document_geometry(doc, 1, &opts));
	lc_check(fabs(c->rect->width - 60.0) < LC_EPS && fabs(c->rect->height - 20.0) < LC_EPS,
			 "a drawn choice keeps its rect", "drawn");
	lc_check(at_tips(tree, c), "the ends at the tips of a flat rhombus", "drawn");
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
