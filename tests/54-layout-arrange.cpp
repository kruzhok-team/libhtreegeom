/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange keeps the bands and the order of the seed
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

#define LABEL_W 80.0
#define LABEL_H 20.0

/* the label of the edge over no simple state */
static int label_clear(const HTreeEdge* e, const HTreeNode* nodes)
{
	for (const HTreeNode* n = nodes; n; n = n->next) {
		if (n->type == htSimpleNode && n->rect && lc_rects_overlap(e->label_rect, n->rect)) return 0;
		if (n->children && !label_clear(e, n->children)) return 0;
	}
	return 1;
}

static int labels_clear(const HTree* tree, const HTreeNode* nodes)
{
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		if (e->label_rect && !label_clear(e, nodes)) return 0;
	}
	return 1;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) a chain drawn from right to left keeps its drawn bands */
	{
		HTreeNode *sm, *a, *b, *c;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		a = lc_state(sm, "a");
		b = lc_state(sm, "b");
		c = lc_state(sm, "c");
		htree_add_node(tree, sm);
		htree_node_set_rect(a, 900, 50, 100, 60);
		htree_node_set_rect(b, 500, 50, 100, 60);
		htree_node_set_rect(c, 100, 50, 100, 60);
		lc_edge(tree, "e-a-b", a, b, LABEL_W, LABEL_H);
		lc_edge(tree, "e-b-c", b, c, LABEL_W, LABEL_H);
		printf("=== backward ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(lc_cx(c) < lc_cx(b) && lc_cx(b) < lc_cx(a), "the drawn order kept", "backward");
		lc_check(fabs(lc_cy(a) - lc_cy(b)) < LC_EPS && fabs(lc_cy(b) - lc_cy(c)) < LC_EPS, "one row", "backward");
		lc_check(a->rect->width > 100.0 + LC_EPS || a->rect->height > 60.0 + LC_EPS ||
				 fabs(a->rect->x - 900.0) > LC_EPS, "the places computed, not kept", "backward");
		lc_check(labels_clear(tree, tree->nodes), "the labels beside the states", "backward");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "backward");
		lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "backward");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a diamond whose branches are drawn swapped keeps the swap */
	{
		HTreeNode *sm, *a, *b, *c, *d;
		double bc_full;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		a = lc_state(sm, "a");
		b = lc_state(sm, "b");
		c = lc_state(sm, "c");
		d = lc_state(sm, "d");
		htree_add_node(tree, sm);
		lc_edge(tree, "e-a-b", a, b, LABEL_W, LABEL_H);
		lc_edge(tree, "e-a-c", a, c, LABEL_W, LABEL_H);
		lc_edge(tree, "e-b-d", b, d, LABEL_W, LABEL_H);
		lc_edge(tree, "e-c-d", c, d, LABEL_W, LABEL_H);
		opts.mode = htLayoutFixed;
		opts.direction = htFlowRight;
		printf("=== diamond ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		bc_full = lc_cy(c) - lc_cy(b);
		{
			/* the seed: b and c exchanged across the flow */
			HTreeRect rb = *b->rect, rc = *c->rect;
			htree_node_set_rect(b, rc.x, rc.y, rb.width, rb.height);
			htree_node_set_rect(c, rb.x, rb.y, rc.width, rc.height);
		}
		htree_default_layout_options(&opts);
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(bc_full * (lc_cy(c) - lc_cy(b)) < 0.0, "the swapped order kept", "diamond");
		lc_check(lc_cx(a) < lc_cx(b) && lc_cx(b) < lc_cx(d) && fabs(lc_cx(b) - lc_cx(c)) < LC_EPS,
				 "the bands kept", "diamond");
		lc_check(lc_count_crossings(tree) == 0, "no crossings", "diamond");
		lc_check(labels_clear(tree, tree->nodes), "the labels beside the states", "diamond");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "diamond");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
