/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The error code test: transform errors are propagated to the caller
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
	/* a NULL document is rejected */
	printf("convert NULL: %d\n",
		   htree_convert_document_geometry(NULL, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));

	/* the yEd format combination converts without errors */
	HTDocument* doc = htree_new_document(coordAbsolute, coordLocalCenter, coordAbsolute, edgeCenter);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 0, 0, 100, 100);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 200, 0, 100, 100);
	htree_add_node(tree, b);
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	htree_edge_set_points(edge, 50, 0, -50, 0);
	edge->polyline = htree_new_polyline_coord(120, 50);
	htree_polyline_add_point(edge->polyline, 180, 50);
	edge->label_point = htree_new_point_coord(10, 10);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);
	printf("convert yed to left-top: %d\n",
		   htree_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
										   coordLeftTop, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* reconstruction of a tree without edges is not an error */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 0, 0, 100, 100);
	htree_add_node(tree, parent);
	htree_add_child_node(parent, htree_new_node(htSimpleNode, "child"));
	printf("reconstruct without edges: %d\n",
		   htree_reconstruct_document_geometry(doc, 0));

	/* a NULL result pointer is rejected */
	printf("bounding rect into NULL: %d\n", htree_build_bounding_rect(doc, NULL));
	htree_destroy_document(doc);

	/* a degenerate horizontal document still has a bounding rect */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* p1 = htree_new_node(htPoint, "p1");
	htree_node_set_point(p1, 10, 50);
	htree_add_node(tree, p1);
	HTreeNode* p2 = htree_new_node(htPoint, "p2");
	htree_node_set_point(p2, 90, 50);
	htree_add_node(tree, p2);
	HTreeRect* br = NULL;
	printf("degenerate bounding rect: %d ", htree_build_bounding_rect(doc, &br));
	htree_print_rect(br);
	printf("\n");
	htree_destroy_rect(br);
	htree_destroy_document(doc);

	/* the point printer prints a point */
	HTreePoint* pt = htree_new_point_coord(1, 2);
	htree_print_point(pt);
	printf("\n");
	htree_destroy_point(pt);
	return 0;
}
