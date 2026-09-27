/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the adaptive flow direction
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

/* the chain of the machine level: init -> a -> b -> c -> fin */
static HTDocument* chain(HTree** tree, HTreeNode** first, HTreeNode** last, int with_rect)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* b = lc_state(sm, "b");
	HTreeNode* c = lc_state(sm, "c");
	HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
	if (with_rect) htree_node_set_rect(sm, 0, 0, 100, 100);
	htree_add_node(*tree, sm);
	lc_edge(*tree, "e-init-a", init, a, 0.0, 0.0);
	lc_edge(*tree, "e-a-b", a, b, 0.0, 0.0);
	lc_edge(*tree, "e-b-c", b, c, 0.0, 0.0);
	lc_edge(*tree, "e-c-fin", c, fin, 0.0, 0.0);
	*first = a;
	*last = c;
	return doc;
}

static int runs_right(const HTreeNode* a, const HTreeNode* b)
{
	return lc_cx(a) < lc_cx(b) && fabs(lc_cy(a) - lc_cy(b)) < LC_EPS;
}

static int runs_down(const HTreeNode* a, const HTreeNode* b)
{
	return lc_cy(a) < lc_cy(b) && fabs(lc_cx(a) - lc_cx(b)) < LC_EPS;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTLayoutOptions opts;
	HTreeNode *first, *last;

	/* (a) the machine level of a chain runs left-to-right: a wide box */
	htree_default_layout_options(&opts);
	doc = chain(&tree, &first, &last, 0);
	printf("=== chain ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	lc_check(runs_right(first, last), "the chain runs right", "chain");
	lc_check(tree->nodes->rect && tree->nodes->rect->width > tree->nodes->rect->height, "a wide machine", "chain");
	lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", "chain");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* (b) a long chain inside a composite: the two starts compared by the
	   final shape - the composite content runs right, the machine level down */
	{
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* c = lc_composite(sm, "c");
		HTreeNode* r = lc_region(c, "r");
		HTreeNode* s1 = lc_state(r, "s1");
		HTreeNode* s2 = lc_state(r, "s2");
		HTreeNode* s3 = lc_state(r, "s3");
		HTreeNode* s4 = lc_state(r, "s4");
		HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
		doc = lc_document(&tree);
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-c", init, c, 0.0, 0.0);
		lc_edge(tree, "e-s1-s2", s1, s2, 0.0, 0.0);
		lc_edge(tree, "e-s2-s3", s2, s3, 0.0, 0.0);
		lc_edge(tree, "e-s3-s4", s3, s4, 0.0, 0.0);
		lc_edge(tree, "e-c-fin", c, fin, 0.0, 0.0);
		htree_default_layout_options(&opts);
		printf("=== composite chain ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(runs_right(s1, s4), "the composite content runs right", "composite chain");
		lc_check(lc_cy(init) < lc_cy(c) && lc_cy(c) < lc_cy(fin), "the machine level runs down", "composite chain");
		lc_check(sm->rect && sm->rect->width > sm->rect->height, "a wide machine", "composite chain");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "composite chain");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (c) a single state inside a composite: the tie takes the preferred
	   frame of the level (tall under the wide machine): the loop on the right */
	{
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* c = lc_composite(sm, "c");
		HTreeNode* r = lc_region(c, "r");
		HTreeNode* s = lc_state(r, "s");
		HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
		HTreeEdge* loop;
		doc = lc_document(&tree);
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-c", init, c, 0.0, 0.0);
		loop = lc_edge(tree, "e-s-s", s, s, 0.0, 0.0);
		lc_edge(tree, "e-c-fin", c, fin, 0.0, 0.0);
		htree_default_layout_options(&opts);
		printf("=== single ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(runs_right(init, fin), "the machine level runs right", "single");
		lc_check(loop->polyline && loop->polyline->point.x > s->rect->x + s->rect->width,
				 "the loop on the right: the composite frame is down", "single");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (d) a wide diamond inside a composite: the down frame is closer to
	   the target whatever the start */
	{
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* c = lc_composite(sm, "c");
		HTreeNode* r = lc_region(c, "r");
		HTreeNode* a = lc_node(r, htSimpleNode, "a", htRoleNone, 400.0, 60.0);
		HTreeNode* b = lc_node(r, htSimpleNode, "b", htRoleNone, 400.0, 60.0);
		HTreeNode* d = lc_node(r, htSimpleNode, "d", htRoleNone, 400.0, 60.0);
		HTreeNode* e = lc_node(r, htSimpleNode, "e", htRoleNone, 400.0, 60.0);
		HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
		doc = lc_document(&tree);
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-c", init, c, 0.0, 0.0);
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(tree, "e-a-d", a, d, 0.0, 0.0);
		lc_edge(tree, "e-b-e", b, e, 0.0, 0.0);
		lc_edge(tree, "e-d-e", d, e, 0.0, 0.0);
		lc_edge(tree, "e-c-fin", c, fin, 0.0, 0.0);
		htree_default_layout_options(&opts);
		printf("=== diamond ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(lc_cy(a) < lc_cy(b) && fabs(lc_cy(b) - lc_cy(d)) < LC_EPS && lc_cy(d) < lc_cy(e),
				 "the wide diamond runs down", "diamond");
		lc_check(lc_count_crossings(tree) == 0, "no crossings", "diamond");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "diamond");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (e) the root variants: the border rect preset, no rect, a top-level list */
	htree_default_layout_options(&opts);
	doc = chain(&tree, &first, &last, 1);
	printf("=== chain with border ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));
	lc_check(runs_right(first, last) && tree->nodes->rect, "the bordered chain runs right", "border");
	htree_print_document(doc);
	htree_destroy_document(doc);
	doc = chain(&tree, &first, &last, 0);
	printf("=== chain without border ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));
	lc_check(runs_right(first, last) && !tree->nodes->rect, "the rect-less chain runs right", "no border");
	htree_print_document(doc);
	htree_destroy_document(doc);
	{
		HTreeNode* a = lc_node(NULL, htSimpleNode, "a", htRoleNone, 0.0, 0.0);
		HTreeNode* b = lc_node(NULL, htSimpleNode, "b", htRoleNone, 0.0, 0.0);
		HTreeNode* c = lc_node(NULL, htSimpleNode, "c", htRoleNone, 0.0, 0.0);
		doc = lc_document(&tree);
		htree_add_node(tree, a);
		htree_add_node(tree, b);
		htree_add_node(tree, c);
		lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		lc_edge(tree, "e-b-c", b, c, 0.0, 0.0);
		printf("=== list ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));
		lc_check(runs_right(a, c), "the top-level list runs right", "list");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (f) the fixed mode lays everything down */
	{
		HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		HTreeNode* init = lc_point(sm, "init", htRoleInitial);
		HTreeNode* c = lc_composite(sm, "c");
		HTreeNode* r = lc_region(c, "r");
		HTreeNode* s1 = lc_state(r, "s1");
		HTreeNode* s2 = lc_state(r, "s2");
		doc = lc_document(&tree);
		htree_add_node(tree, sm);
		lc_edge(tree, "e-init-c", init, c, 0.0, 0.0);
		lc_edge(tree, "e-s1-s2", s1, s2, 0.0, 0.0);
		htree_default_layout_options(&opts);
		opts.mode = htLayoutFixed;
		opts.direction = htFlowDown;
		printf("=== fixed down ===\n");
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		lc_check(runs_down(init, c) && runs_down(s1, s2), "everything down", "fixed");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (g) the option checks */
	doc = chain(&tree, &first, &last, 0);
	htree_default_layout_options(&opts);
	opts.aspect = -1.0;
	lc_check(htree_reconstruct_document_geometry(doc, 1, &opts) == HTREE_OK, "the aspect falls back", "options");
	opts.mode = (HTLayoutMode)99;
	lc_check(htree_reconstruct_document_geometry(doc, 1, &opts) == HTREE_BAD_PARAMETER, "a bad mode rejected", "options");
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
