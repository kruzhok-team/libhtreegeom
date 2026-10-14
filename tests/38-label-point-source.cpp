/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the label rect of a transition from a point
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

#include "layout-check.h"

/* The label rect of a transition from a point: its centre relative to
   the point in the local-centre format, its left-top relative to the
   point in the left-top format, and the absolute rect after the round
   trip */
int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	HTreeNode* a = lc_state(sm, "a");
	HTreeEdge* e;

	htree_node_set_rect(sm, 0, 0, 600, 400);
	htree_node_set_point(init, 100, 50);
	htree_node_set_rect(a, 50, 150, 300, 200);
	htree_add_node(tree, sm);
	e = lc_edge(tree, "e-init-a", init, a, 0.0, 0.0);
	htree_edge_set_points(e, 100, 50, 200, 150);
	e->label_rect = htree_new_rect();
	e->label_rect->x = 120;
	e->label_rect->y = 60;
	e->label_rect->width = 200;
	e->label_rect->height = 40;

	printf("convert to local-centre: %d\n",
		   htree_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter, coordLocalCenter, edgeBorder));
	lc_check(e->label_rect && fabs(e->label_rect->x - 120.0) < LC_EPS && fabs(e->label_rect->y - 30.0) < LC_EPS,
			 "the label centre relative to the point", "local-centre");
	htree_print_document(doc);

	printf("convert to left-top: %d\n",
		   htree_convert_document_geometry(doc, coordLeftTop, coordLeftTop, coordLeftTop, edgeBorder));
	lc_check(e->label_rect && fabs(e->label_rect->x - 20.0) < LC_EPS && fabs(e->label_rect->y - 10.0) < LC_EPS,
			 "the label left-top relative to the point", "left-top");
	htree_print_document(doc);

	printf("convert to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder));
	lc_check(e->label_rect && fabs(e->label_rect->x - 120.0) < LC_EPS && fabs(e->label_rect->y - 60.0) < LC_EPS &&
			 fabs(e->label_rect->width - 200.0) < LC_EPS, "the absolute label after the round trip", "absolute");
	htree_print_document(doc);

	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
