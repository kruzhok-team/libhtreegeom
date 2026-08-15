/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The reconstruction test: the edges get the missing geometry
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

static HTDocument* build_doc(int sm_rect)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	if (sm_rect) {
		htree_node_set_rect(sm, 0, 0, 800, 600);
	}
	htree_add_node(tree, sm);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 100, 100, 200, 100);
	htree_add_child_node(sm, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 500, 100, 200, 100);
	htree_add_child_node(sm, b);
	HTreeNode* init = htree_new_node(htPoint, "init");
	htree_node_set_point(init, 350, 250);
	htree_add_child_node(sm, init);

	/* the straight edge, the loop, the edge from a point node -
	   all without geometry - and a partially specified edge */
	HTreeEdge* edge = htree_new_edge("e-a-b", "a", "b");
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-b-b", "b", "b");
	edge->source = htree_find_node_by_id(tree->nodes, "b");
	edge->target = edge->source;
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-init-a", "init", "a");
	edge->source = htree_find_node_by_id(tree->nodes, "init");
	edge->target = htree_find_node_by_id(tree->nodes, "a");
	htree_add_edge(tree, edge);
	edge = htree_new_edge("e-partial", "a", "b");
	edge->source_point = htree_new_point_coord(300, 120);
	edge->source = htree_find_node_by_id(tree->nodes, "a");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);

	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	return doc;
}

int main()
{
	/* the absolute document with the SM border */
	HTDocument* doc = build_doc(1);
	printf("=== absolute ===\n");
	htree_print_document(doc);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* the same document in the Qt format keeps its format */
	doc = build_doc(1);
	htree_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter,
									coordLocalCenter, edgeBorder);
	printf("=== qt ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* the SM border reconstructed around the content and the loop */
	doc = build_doc(0);
	printf("=== reconstruct sm ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1));
	htree_print_document(doc);
	printf("geometry check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);
	return 0;
}
