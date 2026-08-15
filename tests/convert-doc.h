/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The canonical document builder for the conversion test matrix
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

#ifndef CONVERT_DOC_H
#define CONVERT_DOC_H

#include "htgeom.h"

/* The canonical absolute document: the SM border, a nested composite,
   a point node, a straight edge, a loop edge with a polyline and edges
   with labels; the coordinates are exact in binary floating point */
static HTDocument* build_convert_doc(void)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* sm = htree_new_node(htTree, "sm");
	htree_node_set_rect(sm, 0, 0, 1000, 600);
	htree_add_node(tree, sm);
	HTreeNode* c = htree_new_node(htCompositeNode, "c");
	htree_node_set_rect(c, 100, 100, 400, 300);
	htree_add_child_node(sm, c);
	HTreeNode* c1 = htree_new_node(htSimpleNode, "c-1");
	htree_node_set_rect(c1, 150, 150, 100, 80);
	htree_add_child_node(c, c1);
	HTreeNode* init = htree_new_node(htPoint, "init");
	htree_node_set_point(init, 600, 80);
	htree_add_child_node(sm, init);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 600, 150, 200, 150);
	htree_add_child_node(sm, b);

	HTreeEdge* edge = htree_new_edge("e-c1-b", "c-1", "b");
	htree_edge_set_points(edge, 250, 190, 600, 190);
	edge->label_point = htree_new_point_coord(400, 100);
	edge->source = htree_find_node_by_id(tree->nodes, "c-1");
	edge->target = htree_find_node_by_id(tree->nodes, "b");
	htree_add_edge(tree, edge);

	edge = htree_new_edge("e-b-b", "b", "b");
	htree_edge_set_points(edge, 800, 250, 800, 300);
	edge->polyline = htree_new_polyline_coord(850, 250);
	htree_polyline_add_point(edge->polyline, 850, 300);
	edge->label_rect = htree_new_rect_coord(820, 260, 60, 30);
	edge->source = htree_find_node_by_id(tree->nodes, "b");
	edge->target = edge->source;
	htree_add_edge(tree, edge);

	edge = htree_new_edge("e-c1-init", "c-1", "init");
	htree_edge_set_points(edge, 250, 170, 600, 80);
	edge->polyline = htree_new_polyline_coord(400, 80);
	htree_polyline_add_point(edge->polyline, 500, 80);
	edge->source = htree_find_node_by_id(tree->nodes, "c-1");
	edge->target = htree_find_node_by_id(tree->nodes, "init");
	htree_add_edge(tree, edge);

	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	return doc;
}

#endif
