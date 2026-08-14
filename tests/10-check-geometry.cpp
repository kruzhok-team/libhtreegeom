/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The geometry check test: the content must fit the explicit SM border
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
#include "htgeom.h"

static HTDocument* build_doc(int sm_rect, int escape)
{
	float loop_x = escape ? 700.0f : 450.0f;
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	if (sm_rect) {
		htree_node_set_rect(sm, 100, 100, 500, 400);
	}
	htree_add_node(tree, sm);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 150, 150, 100, 80);
	htree_add_child_node(sm, a);
	HTreeEdge* edge = htree_new_edge("e-a-a", "a", "a");
	htree_edge_set_points(edge, 250, 180, 250, 200);
	edge->polyline = htree_new_polyline_coord(loop_x, 180);
	htree_polyline_add_point(edge->polyline, loop_x, 200);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = edge->source;
	htree_add_edge(tree, edge);
	return doc;
}

int main()
{
	/* the border contains the content */
	HTDocument* doc = build_doc(1, 0);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* the loop polyline escapes the border */
	doc = build_doc(1, 1);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* no border - nothing to violate */
	doc = build_doc(0, 1);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);
	return 0;
}
