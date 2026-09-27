/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the direction alternating by depth
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

typedef struct {
	HTreeNode *init, *c, *c1, *c2, *d1, *d2;
} Nodes;

static HTDocument* build(HTree** tree, Nodes* n)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* rc;
	HTreeNode* rd;
	n->init = lc_point(sm, "init", htRoleInitial);
	n->c = lc_composite(sm, "c");
	rc = lc_region(n->c, "rc");
	n->c1 = lc_state(rc, "c1");
	n->c2 = lc_composite(rc, "c2");
	rd = lc_region(n->c2, "rd");
	n->d1 = lc_state(rd, "d1");
	n->d2 = lc_state(rd, "d2");
	htree_add_node(*tree, sm);
	lc_edge(*tree, "e-init-c", n->init, n->c, 0.0, 0.0);
	lc_edge(*tree, "e-c1-c2", n->c1, n->c2, 0.0, 0.0);
	lc_edge(*tree, "e-d1-d2", n->d1, n->d2, 0.0, 0.0);
	return doc;
}

static int flows_right(const HTreeNode* a, const HTreeNode* b)
{
	return a->rect->x + a->rect->width <= b->rect->x + LC_EPS && fabs(lc_cy(a) - lc_cy(b)) < LC_EPS;
}

static int flows_down(const HTreeNode* a, const HTreeNode* b)
{
	return a->rect->y + a->rect->height <= b->rect->y + LC_EPS && fabs(lc_cx(a) - lc_cx(b)) < LC_EPS;
}

int main(void)
{
	HTree* tree;
	Nodes n;
	HTLayoutOptions opts;
	HTDocument* doc;

	doc = build(&tree, &n);
	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("=== alternate ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(lc_cy(n.init) < lc_cy(n.c), "the SM flows down", "alternate");
	lc_check(flows_right(n.c1, n.c2), "the composite flows right", "alternate");
	lc_check(flows_down(n.d1, n.d2), "the nested composite flows down", "alternate");
	lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", "alternate");
	htree_print_document(doc);
	htree_destroy_document(doc);

	doc = build(&tree, &n);
	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutFixed;
	printf("=== down only ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(flows_down(n.c1, n.c2), "the composite flows down", "down only");
	lc_check(flows_down(n.d1, n.d2), "the nested composite flows down", "down only");
	htree_print_document(doc);
	htree_destroy_document(doc);

	doc = build(&tree, &n);
	htree_default_layout_options(&opts);
	opts.direction = htFlowRight;
	opts.mode = htLayoutAlternate;
	printf("=== right root ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(lc_cx(n.init) < lc_cx(n.c), "the SM flows right", "right root");
	lc_check(flows_down(n.c1, n.c2), "the composite flows down", "right root");
	lc_check(flows_right(n.d1, n.d2), "the nested composite flows right", "right root");
	htree_print_document(doc);
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
