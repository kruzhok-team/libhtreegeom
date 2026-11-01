/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the orthogonal routing in the update and the arrange, the
 * labels beside their runs, the polylines the default
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

/* the label of a transition beside a run of its route, clear of the states */
static int labels_beside(const HTree* tree, double pad)
{
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		HTreePoint a[LC_MAX_CHAIN];
		int n, near = 0;
		const HTreeRect* r = e->label_rect;
		if (!lc_edge_is_transition(e) || !r || e->source == e->target) continue;
		for (const HTreeNode* s = tree->nodes->children; s; s = s->next) {
			if (s->rect && s->type != htComment && lc_rects_overlap(r, s->rect)) return 0;
		}
		n = lc_edge_chain(e, a, LC_MAX_CHAIN);
		for (int i = 0; i + 1 < n && !near; i++) {
			double x0 = fmin(a[i].x, a[i + 1].x), x1 = fmax(a[i].x, a[i + 1].x);
			double y0 = fmin(a[i].y, a[i + 1].y), y1 = fmax(a[i].y, a[i + 1].y);
			double dx = fmax(0.0, fmax(x0 - (r->x + r->width), r->x - x1));
			double dy = fmax(0.0, fmax(y0 - (r->y + r->height), r->y - y1));
			near = dx <= pad + LC_EPS && dy <= pad + LC_EPS;
		}
		if (!near) return 0;
	}
	return 1;
}

static HTDocument* chain(HTree** tree, HTreeNode** sm, HTreeNode** v)
{
	HTDocument* doc = lc_document(tree);
	const char* ids[] = {"a", "b", "c", "d"};
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_add_node(*tree, *sm);
	for (int i = 0; i < 4; i++) v[i] = lc_state(*sm, ids[i]);
	lc_edge(*tree, "e-a-b", v[0], v[1], 60.0, 20.0);
	lc_edge(*tree, "e-a-c", v[0], v[2], 60.0, 20.0);
	lc_edge(*tree, "e-b-d", v[1], v[3], 60.0, 20.0);
	lc_edge(*tree, "e-c-d", v[2], v[3], 60.0, 20.0);
	lc_edge(*tree, "e-d-a", v[3], v[0], 60.0, 20.0);
	return doc;
}

int main(void)
{
	HTree *tree, *tree2;
	HTreeNode *sm, *sm2, *v[4], *v2[4];
	HTDocument *doc, *poly;
	HTLayoutOptions opts, def;

	htree_default_layout_options(&def);
	lc_check(def.routing == htRoutingPolyline, "the polylines the default", "options");

	/* (a) the labels beside their runs */
	htree_default_layout_options(&opts);
	opts.routing = htRoutingOrthogonal;
	doc = chain(&tree, &sm, v);
	poly = chain(&tree2, &sm2, v2);
	printf("=== labels ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
	printf("reconstruct polylines: %d\n", htree_reconstruct_document_geometry(poly, 1, &def));
	lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", "labels");
	lc_check(labels_beside(tree, opts.padding), "the labels beside their runs", "labels");
	lc_check(lc_count_oblique(tree2) > 0, "the polylines oblique", "labels");
	htree_print_document(doc);
	htree_destroy_document(poly);

	/* (b) the update: the drawn routes kept, a new transition orthogonal */
	{
		HTreeNode* e = lc_state(sm, "e");
		HTreeEdge* drawn = tree->edges;
		HTreePoint s0 = *drawn->source_point, t0 = *drawn->target_point;
		HTreeEdge* added = lc_edge(tree, "e-d-e", v[3], e, 60.0, 20.0);
		printf("=== update ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 1, &opts));
		lc_check(lc_same_point(s0, *drawn->source_point) && lc_same_point(t0, *drawn->target_point),
				 "the drawn route kept", "update");
		lc_check(added->source_point && added->target_point && lc_count_oblique(tree) == 0,
				 "the new transition along the axes", "update");
		lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", "update");
	lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", "update");
		htree_print_document(doc);
	}

	/* (c) the arrange */
	printf("=== arrange ===\n");
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(lc_count_oblique(tree) == 0, "the runs along the axes", "arrange");
	lc_check(lc_count_shared_runs(tree) == 0, "no runs on each other", "arrange");
	lc_check(lc_count_line_rect_crossings(tree) == 0, "no route through a state", "arrange");
	lc_check(lc_count_own_crossings(tree) == 0, "no route through its own ends", "arrange");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "arrange");
	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
