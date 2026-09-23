/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The comment placement test: a comment may sit outside the SM border
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

/* an SM with an explicit border and a state inside; a node placed outside the
   border on the right, linked to the state. use_comment marks it as a comment */
static HTDocument* build_doc(int node_outside, int use_comment)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	htree_node_set_rect(sm, 100, 100, 400, 300);
	htree_add_node(tree, sm);
	HTreeNode* s = htree_new_node(htSimpleNode, "s");
	htree_node_set_rect(s, 150, 150, 120, 80);
	htree_add_child_node(sm, s);
	int cx = node_outside ? 620 : 300;
	HTreeNode* c = htree_new_node(use_comment ? htComment : htSimpleNode, "c");
	htree_node_set_rect(c, cx, 150, 120, 60);
	htree_add_child_node(sm, c);
	HTreeEdge* edge = htree_new_edge("c-s", "c", "s");
	htree_edge_set_points(edge, cx, 180, 270, 190);
	edge->source = htree_find_node_by_id(tree->nodes, "c");
	edge->target = htree_find_node_by_id(tree->nodes, "s");
	htree_add_edge(tree, edge);
	return doc;
}

int main()
{
	/* a comment outside the border is allowed: the fit ignores it, and the
	   reconstruction neither grows the border toward it nor moves it in */
	HTDocument* doc = build_doc(1, 1);
	printf("comment outside, check: %d\n", htree_check_geometry(doc));
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0));
	htree_print_document(doc);
	printf("after reconstruct, check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* a comment inside the border is equally valid */
	doc = build_doc(0, 1);
	printf("comment inside, check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);

	/* the negative twin: the same node as a plain state escaping the border
	   is still invalid */
	doc = build_doc(1, 0);
	printf("state outside, check: %d\n", htree_check_geometry(doc));
	htree_destroy_document(doc);
	return 0;
}
