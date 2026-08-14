/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The bounding rect test: a loop edge polyline extends the rect
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

	HTreeNode* s = htree_new_node(htSimpleNode, "s");
	htree_node_set_rect(s, 100, 100, 200, 150);
	htree_add_node(tree, s);

	/* the loop goes around the right side of the state */
	HTreeEdge* edge = htree_new_edge("e-s-s", "s", "s");
	htree_edge_set_points(edge, 300, 150, 300, 200);
	edge->polyline = htree_new_polyline_coord(380, 150);
	htree_polyline_add_point(edge->polyline, 380, 200);
	edge->source = htree_find_node_by_id(tree->nodes, "s");
	edge->target = edge->source;
	htree_add_edge(tree, edge);

	HTreeRect* br = NULL;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
