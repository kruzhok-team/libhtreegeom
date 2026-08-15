/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The reconstruction test: the fill-in preserves the authored geometry
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
	/* the authored composite keeps its rect, the geometry-less children
	   (the internal pseudostate case) shelf inside it */
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 100, 100, 400, 300);
	htree_add_node(tree, parent);
	HTreeNode* c1 = htree_new_node(htSimpleNode, "c-1");
	htree_node_set_rect(c1, 150, 150, 100, 80);
	htree_add_child_node(parent, c1);
	htree_add_child_node(parent, htree_new_node(htPoint, "init"));
	htree_add_child_node(parent, htree_new_node(htSimpleNode, "s-2"));
	printf("=== fill in the authored parent ===\n");
	htree_print_document(doc);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* the too-small authored parent grows and the growth cascades
	   to the SM border */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	htree_node_set_rect(sm, 0, 0, 200, 150);
	htree_add_node(tree, sm);
	HTreeNode* small = htree_new_node(htCompositeNode, "small");
	htree_node_set_rect(small, 0, 0, 150, 100);
	htree_add_child_node(sm, small);
	htree_add_child_node(small, htree_new_node(htSimpleNode, "n-1"));
	htree_add_child_node(small, htree_new_node(htSimpleNode, "n-2"));
	printf("=== grow the parent and the border ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* the fully authored document is not modified */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 10, 10, 100, 60);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 200, 10, 100, 60);
	htree_add_node(tree, b);
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	htree_edge_set_points(edge, 110, 40, 200, 40);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);
	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	printf("=== authored no-op ===\n");
	htree_print_document(doc);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
