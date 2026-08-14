/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The bounding rect test: straight edge points do not extend the rect
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

	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 0, 0, 400, 300);
	htree_add_node(tree, parent);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 20, 20, 100, 80);
	htree_add_child_node(parent, a);
	HTreeNode* c = htree_new_node(htCompositeNode, "c");
	htree_node_set_rect(c, 200, 100, 150, 120);
	htree_add_child_node(parent, c);
	HTreeNode* c1 = htree_new_node(htSimpleNode, "c-1");
	htree_node_set_rect(c1, 220, 120, 60, 40);
	htree_add_child_node(c, c1);

	/* the edge points lie outside the node union
	   but must not extend the bounding rect */
	HTreeEdge* edge = htree_new_edge("e-a-c1", "a", "c-1");
	htree_edge_set_points(edge, 450, 350, 50, 50);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "c-1");
	htree_add_edge(tree, edge);

	HTreeRect* br = NULL;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
