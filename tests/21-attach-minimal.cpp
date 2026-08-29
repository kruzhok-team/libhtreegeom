/* -----------------------------------------------------------------------------
 * The hierarchical tree geometry library
 *
 * The minimal edge attachment test
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "htgeom.h"

#define EPS 0.001

static int failures = 0;

static void check(int ok, const char* what, const char* id)
{
	if (!ok) {
		fprintf(stderr, "%s failed for %s\n", what, id);
		failures++;
	}
}

/* The attachment must not run into the border parallel to it: on a vertical
   border the edge advances at least as much across as along it */
static void check_incidence(const HTreeNode* node, const HTreePoint* p,
							const HTreePoint* other, const char* id)
{
	double dx = fabs(other->x - p->x);
	double dy = fabs(other->y - p->y);

	if (!node->rect) {
		return;
	}
	if (fabs(p->x - node->rect->x) < EPS ||
		fabs(p->x - node->rect->x - node->rect->width) < EPS) {
		check(dx >= dy - EPS, "vertical border incidence", id);
	} else {
		check(dy >= dx - EPS, "horizontal border incidence", id);
	}
}

static void pair(const char* id, int point_source,
				 double ax, double ay, double aw, double ah,
				 double bx, double by, double bw, double bh)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute,
										 coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	HTreeNode *sm, *a, *b;
	HTreeEdge* edge;

	htree_add_tree(doc, tree);
	sm = htree_new_node(htTree, "sm");
	htree_node_set_rect(sm, -1000, -1000, 4000, 4000);
	htree_add_node(tree, sm);
	if (point_source) {
		a = htree_new_node(htPoint, "a");
		htree_node_set_point(a, ax, ay);
	} else {
		a = htree_new_node(htSimpleNode, "a");
		htree_node_set_rect(a, ax, ay, aw, ah);
	}
	htree_add_child_node(sm, a);
	b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, bx, by, bw, bh);
	htree_add_child_node(sm, b);

	edge = htree_new_edge("e", "a", "b");
	edge->source = a;
	edge->target = b;
	htree_add_edge(tree, edge);

	check(htree_reconstruct_document_geometry(doc, 0) == HTREE_OK,
		  "reconstruction", id);
	check(edge->source_point != NULL && edge->target_point != NULL,
		  "attachment", id);
	if (edge->source_point && edge->target_point) {
		check_incidence(a, edge->source_point, edge->target_point, id);
		check_incidence(b, edge->target_point, edge->source_point, id);
		printf("%-22s (%g, %g) -> (%g, %g)  length %.1f\n", id,
			   edge->source_point->x, edge->source_point->y,
			   edge->target_point->x, edge->target_point->y,
			   sqrt(pow(edge->target_point->x - edge->source_point->x, 2) +
					pow(edge->target_point->y - edge->source_point->y, 2)));
	}
	htree_destroy_document(doc);
}

int main(void)
{
	pair("band-horizontal", 0, 100, 100, 200, 100, 500, 100, 200, 100);
	pair("band-vertical",   0, 100, 100, 200, 100, 100, 400, 200, 100);
	pair("band-partial",    0,   0,   0, 300, 400, 500, 300, 150, 100);
	pair("diagonal-wide",   0,   0,   0, 150, 100, 500, 200, 150, 100);
	pair("diagonal-tall",   0,   0,   0, 150, 100, 200, 600, 150, 100);
	pair("diagonal-flat",   0,   0,   0, 600, 100, 650, 300, 100, 100);
	pair("diagonal-even",   0,   0,   0, 100, 100, 300, 300, 100, 100);
	pair("point-diagonal",  1, 350, 250,   0,   0, 100, 100, 200, 100);
	pair("point-band",      1, 250,  50,   0,   0, 100, 100, 200, 100);
	pair("nested",          0,   0,   0, 600, 400, 100, 100, 200, 100);
	pair("nested-inner",    0, 100, 100, 200, 100,   0,   0, 600, 400);

	if (failures) {
		fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	return 0;
}
