/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The the hierarchiceal tree geometry library
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
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
	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 10, 10, 500, 300);
	htree_add_node(tree, parent);
	HTreeNode* node0 = htree_new_node(htSimpleNode, "node-0");
	htree_node_set_rect(node0, 60, 160, 150, 100);
	htree_add_child_node(parent, node0);
	HTreeNode* initial = htree_new_node(htPoint, "initial");
	htree_node_set_point(initial, 110, 60);
	htree_add_child_node(parent, initial);
	HTreeNode* node1 = htree_new_node(htCompositeNode, "node-1");
	htree_node_set_rect(node1, 310, 60, 200, 150);
	htree_add_child_node(parent, node1);
	HTreeNode* node11 = htree_new_node(htSimpleNode, "node-1-1");
	htree_node_set_rect(node11, 330, 80, 110, 70);
	htree_add_child_node(node1, node11);
	HTreeNode* node12 = htree_new_node(htSimpleNode, "node-1-2");
	htree_node_set_rect(node12, 330, 170, 110, 70);
	htree_add_child_node(node1, node12);

	HTreeEdge* edge = htree_new_edge("e-i-0", "initial", "node-0");
	htree_edge_set_points(edge, 110, 60, 110, 160);
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-0-11", "node-0", "node-1-1");
	htree_edge_set_points(edge, 210, 210, 330, 115);
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-1-0", "node-1", "node-0");
	htree_edge_set_points(edge, 310, 250, 210, 250);
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-11-12", "node-1-1", "node-1-2");
	htree_edge_set_points(edge, 350, 150, 350, 170);
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-12-11", "node-1-2", "node-1-1");
	htree_edge_set_points(edge, 420, 170, 420, 150);
	htree_add_edge(tree, edge);

	HTreeRect* br;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
