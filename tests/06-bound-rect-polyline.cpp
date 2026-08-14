/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The bounding rect test: edge polylines extend the rect
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

#include "htgeom.h"

int main()
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);

	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 0, 0, 100, 100);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 200, 0, 100, 100);
	htree_add_node(tree, b);
	HTreeNode* p = htree_new_node(htPoint, "p");
	htree_node_set_point(p, 150, 150);
	htree_add_node(tree, p);

	/* the polyline escapes the node union upwards */
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	htree_edge_set_points(edge, 100, 50, 200, 50);
	edge->polyline = htree_new_polyline_coord(140, -80);
	htree_polyline_add_point(edge->polyline, 160, -80);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);

	/* the edge into a point node: the target is taken from the node point */
	edge = htree_new_edge("e-a-p", "a", "p");
	edge->source_point = htree_new_point_coord(50, 100);
	edge->polyline = htree_new_polyline_coord(60, 120);
	htree_polyline_add_point(edge->polyline, 80, 130);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "p");
	htree_add_edge(tree, edge);

	HTreeRect* br = NULL;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
