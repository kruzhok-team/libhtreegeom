/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: the center-to-border edge projection
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

int main()
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeCenter);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 0, 0, 100, 100);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 300, 0, 100, 100);
	htree_add_node(tree, b);
	HTreeNode* p = htree_new_node(htPoint, "p");
	htree_node_set_point(p, 200, 200);
	htree_add_node(tree, p);

	/* a horizontal edge between the node centers */
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	htree_edge_set_points(edge, 50, 50, 350, 50);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);

	/* a diagonal edge into a point node: the target stays on the point */
	edge = htree_new_edge("e-a-p", "a", "p");
	htree_edge_set_points(edge, 50, 50, 200, 200);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "p");
	htree_add_edge(tree, edge);

	/* a loop edge with a polyline: crossings at exact halves */
	edge = htree_new_edge("e-b-b", "b", "b");
	htree_edge_set_points(edge, 350, 50, 350, 50);
	edge->polyline = htree_new_polyline_coord(450, 25);
	htree_polyline_add_point(edge->polyline, 450, 75);
	edge->source = htree_find_node_by_id(tree->nodes, "b");
	edge->target = edge->source;
	htree_add_edge(tree, edge);

	/* an edge without explicit points: the center fallback, then projection */
	edge = htree_new_edge("e-b-a", "b", "a");
	edge->source = htree_find_node_by_id(tree->nodes, "b");
	edge->target = htree_find_node_by_id(tree->nodes, "a");
	htree_add_edge(tree, edge);

	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	printf("=== center-bound edges ===\n");
	htree_print_document(doc);
	printf("to border format: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
