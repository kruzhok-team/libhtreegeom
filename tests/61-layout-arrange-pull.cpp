/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange pulls the seeded vertices to their scaled seed
 * places - the proportions, the lines, a fixed point
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

/* the drawn boxes: the extents along the flow apart, else one band; the
   outlier pair drawn almost on each other, thin; the stretch of the seed it
   may cause at most (the library's SEED_PULL_STRETCH) */
#define BOX 120.0
#define THIN 10.0
#define OVERLAP 30.0
#define STRETCH 3.0

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

/* a chain of four in a row, drawn at the centres xs */
static HTDocument* row(const double* xs, double width, HTreeNode** v)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	const char* ids[] = {"a", "b", "c", "d"};
	htree_add_node(tree, sm);
	for (int i = 0; i < 4; i++) {
		v[i] = lc_state(sm, ids[i]);
		htree_node_set_rect(v[i], xs[i] - width / 2.0, 0.0, width, 80.0);
	}
	lc_edge(tree, "e-a-b", v[0], v[1], 0.0, 0.0);
	lc_edge(tree, "e-b-c", v[1], v[2], 0.0, 0.0);
	lc_edge(tree, "e-c-d", v[2], v[3], 0.0, 0.0);
	return doc;
}

/* a diamond drawn right: the branch b far above, c close below, d in
   line with a */
static HTDocument* diamond(HTreeNode** v)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	const double at[4][2] = {{0, 1000}, {600, 0}, {600, 1300}, {1200, 1000}};
	const char* ids[] = {"a", "b", "c", "d"};
	htree_add_node(tree, sm);
	for (int i = 0; i < 4; i++) {
		v[i] = lc_state(sm, ids[i]);
		htree_node_set_rect(v[i], at[i][0], at[i][1], BOX, 80.0);
	}
	lc_edge(tree, "e-a-b", v[0], v[1], 0.0, 0.0);
	lc_edge(tree, "e-a-c", v[0], v[2], 0.0, 0.0);
	lc_edge(tree, "e-b-d", v[1], v[3], 0.0, 0.0);
	lc_edge(tree, "e-c-d", v[2], v[3], 0.0, 0.0);
	return doc;
}

/* the second arrange changes nothing */
static int fixed_point(HTDocument* doc, const HTLayoutOptions* opts)
{
	HTDocument* again = htree_copy_document(doc);
	int same;
	htree_arrange_document_geometry(again, 1, opts);
	same = same_geometry(doc->trees->nodes, again->trees->nodes);
	htree_destroy_document(again);
	return same;
}

int main(void)
{
	HTDocument* doc;
	HTreeNode* v[4];
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) the uneven gaps of a loose seed are kept as drawn */
	{
		const double xs[] = {0.0, 1000.0, 3500.0, 4500.0};
		doc = row(xs, BOX, v);
		printf("=== uneven ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(fabs(lc_cx(v[1]) - lc_cx(v[0]) - 1000.0) < LC_EPS &&
				 fabs(lc_cx(v[2]) - lc_cx(v[1]) - 2500.0) < LC_EPS &&
				 fabs(lc_cx(v[3]) - lc_cx(v[2]) - 1000.0) < LC_EPS, "the drawn gaps kept", "uneven");
		lc_check(fixed_point(doc, &opts), "the second arrange changes nothing", "uneven");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (b) a tight seed is stretched as a whole: its proportions kept, the
	   tightest gap the least one */
	{
		const double xs[] = {0.0, BOX + THIN, 3.0 * (BOX + THIN), 6.0 * (BOX + THIN)};
		double ab, bc, cd;
		doc = row(xs, BOX, v);
		printf("=== tight ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		ab = lc_cx(v[1]) - lc_cx(v[0]);
		bc = lc_cx(v[2]) - lc_cx(v[1]);
		cd = lc_cx(v[3]) - lc_cx(v[2]);
		lc_check(fabs(bc - 2.0 * ab) < LC_EPS && fabs(cd - 3.0 * ab) < LC_EPS, "the proportions kept", "tight");
		lc_check(fabs(v[1]->rect->x - v[0]->rect->x - v[0]->rect->width - opts.layer_gap) < LC_EPS,
				 "the tightest gap the layer gap", "tight");
		lc_check(fixed_point(doc, &opts), "the second arrange changes nothing", "tight");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (c) across the flow: the drawn offsets of the branches, a and d in
	   one line */
	{
		doc = diamond(v);
		printf("=== diamond ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(lc_cx(v[0]) < lc_cx(v[1]) && fabs(lc_cx(v[1]) - lc_cx(v[2])) < LC_EPS && lc_cx(v[2]) < lc_cx(v[3]),
				 "the bands kept", "diamond");
		lc_check(fabs(lc_cy(v[0]) - lc_cy(v[3])) < LC_EPS, "a and d in line", "diamond");
		lc_check(fabs(lc_cy(v[0]) - lc_cy(v[1]) - 1000.0) < LC_EPS && fabs(lc_cy(v[2]) - lc_cy(v[0]) - 300.0) < LC_EPS,
				 "the drawn offsets kept", "diamond");
		lc_check(fixed_point(doc, &opts), "the second arrange changes nothing", "diamond");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	/* (d) the share of the pull: between the order alone and the full pull */
	{
		double offset[3];
		const double shares[] = {0.0, 0.5, 1.0};
		for (int k = 0; k < 3; k++) {
			doc = diamond(v);
			opts.seed_pull = shares[k];
			htree_arrange_document_geometry(doc, 1, &opts);
			offset[k] = lc_cy(v[0]) - lc_cy(v[1]);
			htree_destroy_document(doc);
		}
		htree_default_layout_options(&opts);
		printf("=== share ===\n");
		lc_check(offset[0] < offset[1] - LC_EPS && offset[1] < offset[2] - LC_EPS &&
				 fabs(offset[1] - (offset[0] + offset[2]) / 2.0) < LC_EPS, "half the way", "share");
	}

	/* (e) a pair drawn on each other does not blow the seed up */
	{
		const double xs[] = {0.0, 1000.0, 1000.0 + OVERLAP, 2000.0 + OVERLAP};
		doc = row(xs, THIN, v);
		printf("=== overlap ===\n");
		printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
		lc_check(lc_cx(v[0]) < lc_cx(v[1]) && lc_cx(v[1]) < lc_cx(v[2]) && lc_cx(v[2]) < lc_cx(v[3]),
				 "the drawn order kept", "overlap");
		lc_check(lc_cx(v[3]) - lc_cx(v[0]) < STRETCH * xs[3], "the seed not blown up", "overlap");
		lc_check(v[2]->rect->x - v[1]->rect->x - v[1]->rect->width > opts.layer_gap - LC_EPS, "the pair apart", "overlap");
		htree_print_document(doc);
		htree_destroy_document(doc);
	}

	return lc_failures ? 1 : 0;
}
