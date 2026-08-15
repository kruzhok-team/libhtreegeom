/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: six levels of nested states
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

/* sm > l1 > l2 > l3 > l4 > l5, plus the sibling r1 under l1;
   an edge from the deepest state into the other branch and a deep loop */
static HTDocument* build_deep_doc(void)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	htree_node_set_rect(sm, 40, 20, 1200, 800);
	htree_add_node(tree, sm);
	HTreeNode* l1 = htree_new_node(htCompositeNode, "l1");
	htree_node_set_rect(l1, 100, 100, 1000, 600);
	htree_add_child_node(sm, l1);
	HTreeNode* l2 = htree_new_node(htCompositeNode, "l2");
	htree_node_set_rect(l2, 150, 150, 800, 400);
	htree_add_child_node(l1, l2);
	HTreeNode* l3 = htree_new_node(htCompositeNode, "l3");
	htree_node_set_rect(l3, 200, 200, 600, 300);
	htree_add_child_node(l2, l3);
	HTreeNode* l4 = htree_new_node(htCompositeNode, "l4");
	htree_node_set_rect(l4, 250, 250, 400, 200);
	htree_add_child_node(l3, l4);
	HTreeNode* l5 = htree_new_node(htSimpleNode, "l5");
	htree_node_set_rect(l5, 300, 300, 100, 60);
	htree_add_child_node(l4, l5);
	HTreeNode* r1 = htree_new_node(htSimpleNode, "r1");
	htree_node_set_rect(r1, 900, 600, 150, 100);
	htree_add_child_node(l1, r1);

	/* from the deepest state into the other branch */
	HTreeEdge* edge = htree_new_edge("e-l5-r1", "l5", "r1");
	htree_edge_set_points(edge, 400, 330, 900, 650);
	edge->polyline = htree_new_polyline_coord(600, 330);
	htree_polyline_add_point(edge->polyline, 600, 650);
	edge->label_point = htree_new_point_coord(620, 500);
	edge->source = htree_find_node_by_id(tree->nodes, "l5");
	edge->target = htree_find_node_by_id(tree->nodes, "r1");
	htree_add_edge(tree, edge);

	/* the deep loop */
	edge = htree_new_edge("e-l5-l5", "l5", "l5");
	htree_edge_set_points(edge, 400, 310, 400, 350);
	edge->polyline = htree_new_polyline_coord(450, 310);
	htree_polyline_add_point(edge->polyline, 450, 350);
	edge->source = htree_find_node_by_id(tree->nodes, "l5");
	edge->target = edge->source;
	htree_add_edge(tree, edge);

	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	return doc;
}

static void round_trip(HTCoordFormat n, HTCoordFormat e, HTCoordFormat p,
					   HTEdgeFormat ef, const char* name)
{
	HTDocument* doc = build_deep_doc();
	printf("=== %s ===\n", name);
	printf("to format: %d\n", htree_convert_document_geometry(doc, n, e, p, ef));
	htree_print_document(doc);
	printf("back to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main()
{
	HTDocument* doc = build_deep_doc();
	printf("=== original ===\n");
	htree_print_document(doc);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	round_trip(coordAbsolute, coordLocalCenter, coordAbsolute, edgeCenter, "yed");
	round_trip(coordLeftTop, coordLeftTop, coordLeftTop, edgeBorder, "cyberiada10");
	round_trip(coordLocalCenter, coordLocalCenter, coordLocalCenter, edgeBorder, "qt");
	return 0;
}
