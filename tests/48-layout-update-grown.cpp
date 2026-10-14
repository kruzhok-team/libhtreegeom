/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a drawn route keeps its ends on the border of a grown state
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

/* a drawn a -> b and the drawn way back from the old bottom of b, which
   grew to the given height */
static HTDocument* grown(HTree** tree, HTreeNode** a, HTreeNode** b, HTreeEdge** back, double height)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm;
	HTreeEdge* e;
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(sm, 0, 0, 600, 400);
	*a = lc_state(sm, "a");
	htree_node_set_rect(*a, 50, 50, 200, 150);
	*b = lc_state(sm, "b");
	htree_node_set_rect(*b, 350, 50, 200, height);
	htree_add_node(*tree, sm);
	e = lc_edge(*tree, "e-a-b", *a, *b, 0.0, 0.0);
	htree_edge_set_points(e, 250, 100, 350, 100);
	*back = lc_edge(*tree, "e-b-a", *b, *a, 0.0, 0.0);
	htree_edge_set_points(*back, 450, 200, 150, 200);
	(*back)->polyline = htree_new_polyline_coord(450, 320);
	htree_polyline_add_point((*back)->polyline, 150, 320);
	return doc;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *a, *b;
	HTreeEdge* back;
	HTreePoint p;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) the first bend is still outside: the end slides to the new border */
	doc = grown(&tree, &a, &b, &back, 250);
	printf("=== grown ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(a->rect, 50, 50, 200, 150) && lc_same_rect(b->rect, 350, 50, 200, 250),
			 "the drawn states stay", "grown");
	lc_check(back->source_point && fabs(back->source_point->x - 450) < LC_EPS &&
			 fabs(back->source_point->y - 300) < LC_EPS, "the end on the new border", "grown");
	lc_check(back->polyline && fabs(back->polyline->point.y - 320) < LC_EPS && back->polyline->next &&
			 fabs(back->polyline->next->point.x - 150) < LC_EPS, "the route stays", "grown");
	lc_check(back->target_point && fabs(back->target_point->y - 200) < LC_EPS, "the other end stays", "grown");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* (b) the state covers the first bend: the route is laid out anew */
	doc = grown(&tree, &a, &b, &back, 300);
	printf("=== covered ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(b->rect, 350, 50, 200, 300), "the drawn state stays", "covered");
	lc_check(back->source_point != NULL, "routed", "covered");
	if (back->source_point) {
		p = *back->source_point;
		lc_check(lc_rect_side(p, b->rect) == 0, "the new end on the border", "covered");
	}
	htree_print_document(doc);
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
