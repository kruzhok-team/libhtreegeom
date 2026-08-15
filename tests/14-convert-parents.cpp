/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: the implicit and explicit local-center parents
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

static void round_trip(HTDocument* doc, const char* name)
{
	printf("=== %s ===\n", name);
	htree_print_document(doc);
	printf("to qt: %d\n",
		   htree_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter,
										   coordLocalCenter, edgeBorder));
	htree_print_document(doc);
	printf("back to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main()
{
	/* two top-level states: the document bounding rect is the implicit parent */
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 0, 0, 100, 100);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 200, 0, 100, 100);
	htree_add_node(tree, b);
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	htree_edge_set_points(edge, 100, 50, 200, 50);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);
	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	round_trip(doc, "implicit bounding rect parent");

	/* a single top-level composite: the node rect is the parent */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 10, 10, 400, 200);
	htree_add_node(tree, parent);
	HTreeNode* inner = htree_new_node(htSimpleNode, "inner");
	htree_node_set_rect(inner, 50, 50, 100, 60);
	htree_add_child_node(parent, inner);
	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	round_trip(doc, "explicit toplevel parent");
	return 0;
}
