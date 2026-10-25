/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The reconstruction test: a comment link to a choice ends anywhere on the
 * border of its rhombus, not only on its vertices
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

/* on the border of the rhombus inscribed in r */
static int on_rhombus(const HTreePoint* p, const HTreeRect* r)
{
	double hw = r->width / 2.0, hh = r->height / 2.0;
	double cx = r->x + hw, cy = r->y + hh;
	return fabs(fabs(p->x - cx) / hw + fabs(p->y - cy) / hh - 1.0) < LC_EPS;
}

/* one of the four vertices of the rhombus inscribed in r */
static int on_vertex(const HTreePoint* p, const HTreeRect* r)
{
	double cx = r->x + r->width / 2.0, cy = r->y + r->height / 2.0;
	return (fabs(p->x - cx) < LC_EPS && (fabs(p->y - r->y) < LC_EPS || fabs(p->y - r->y - r->height) < LC_EPS)) ||
		(fabs(p->y - cy) < LC_EPS && (fabs(p->x - r->x) < LC_EPS || fabs(p->x - r->x - r->width) < LC_EPS));
}

/* the comment c linked to the choice ch, and drawn to it with a stored end */
static HTDocument* build(HTree** tree, HTreeNode** ch, HTreeEdge** link, HTreeEdge** drawn)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* c;
	htree_node_set_rect(sm, 0, 0, 800, 600);
	*ch = lc_node(sm, htSimpleNode, "ch", htRoleChoice, 0.0, 0.0);
	htree_node_set_rect(*ch, 500, 100, 60, 60);
	c = lc_node(sm, htComment, "c", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(c, 300, 400, 100, 40);
	htree_add_node(*tree, sm);
	*link = lc_edge(*tree, "l-c-ch", c, *ch, 0.0, 0.0);
	htree_edge_set_link(*link, htEdgeLink, NULL);
	*drawn = lc_edge(*tree, "l-c-ch#1", c, *ch, 0.0, 0.0);
	htree_edge_set_link(*drawn, htEdgeLink, NULL);
	htree_edge_set_points(*drawn, 400, 410, 545, 115);
	return doc;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode* ch;
	HTreeEdge *link, *drawn;
	HTLayoutOptions opts;

	/* the reconstruction aims at the comment centre and crosses the rhombus */
	doc = build(&tree, &ch, &link, &drawn);
	printf("=== reconstruct ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	lc_check(link->target_point && on_rhombus(link->target_point, ch->rect) &&
			 !on_vertex(link->target_point, ch->rect), "the end on an edge of the rhombus", "l-c-ch");
	lc_check(link->target_point && fabs(link->target_point->x - (530.0 - 180.0 * 30.0 / 470.0)) < LC_EPS &&
			 fabs(link->target_point->y - (130.0 + 290.0 * 30.0 / 470.0)) < LC_EPS,
			 "toward the comment centre", "l-c-ch");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* the update fills the missing end the same way and keeps a drawn one */
	doc = build(&tree, &ch, &link, &drawn);
	htree_default_layout_options(&opts);
	printf("=== update ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(link->target_point && on_rhombus(link->target_point, ch->rect) &&
			 !on_vertex(link->target_point, ch->rect), "the end on an edge of the rhombus", "update");
	lc_check(drawn->target_point && fabs(drawn->target_point->x - 545) < LC_EPS &&
			 fabs(drawn->target_point->y - 115) < LC_EPS, "the drawn end kept", "update");
	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
