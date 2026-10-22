/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The update test: a drawn comment link keeps its ends, a missing end is
 * filled, the update is idempotent
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

static int same_point(const HTreePoint* p, double x, double y)
{
	return p && fabs(p->x - x) < LC_EPS && fabs(p->y - y) < LC_EPS;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode *a, *b, *c;
	HTreeEdge *ab, *drawn, *half, *to_ab;
	HTLayoutOptions opts;
	HTreePoint again;

	htree_node_set_rect(sm, 0, 0, 800, 600);
	a = lc_state(sm, "a");
	htree_node_set_rect(a, 100, 100, 200, 100);
	b = lc_state(sm, "b");
	htree_node_set_rect(b, 500, 100, 200, 100);
	c = lc_node(sm, htComment, "c", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(c, 300, 400, 100, 40);
	htree_add_node(tree, sm);
	ab = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	htree_edge_set_points(ab, 300, 150, 500, 150);
	/* drawn off the straight line on purpose: the update must not move it */
	drawn = lc_edge(tree, "l-c-b", c, b, 0.0, 0.0);
	htree_edge_set_link(drawn, htEdgeLink, NULL);
	htree_edge_set_points(drawn, 400, 410, 650, 200);
	/* only the source end drawn */
	half = lc_edge(tree, "l-c-a", c, a, 0.0, 0.0);
	htree_edge_set_link(half, htEdgeLink, NULL);
	half->source_point = htree_new_point_coord(300, 420);
	to_ab = lc_edge(tree, "l-c-ab", c, a, 0.0, 0.0);
	htree_edge_set_link(to_ab, htEdgeLink, "e-a-b");
	to_ab->target_edge = ab;

	htree_default_layout_options(&opts);
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(same_point(drawn->source_point, 400, 410) && same_point(drawn->target_point, 650, 200),
			 "the drawn link kept", "l-c-b");
	lc_check(same_point(half->source_point, 300, 420) && half->target_point &&
			 fabs(half->target_point->y - (a->rect->y + a->rect->height)) < LC_EPS,
			 "the source kept, the target filled on the bottom of a", "l-c-a");
	lc_check(to_ab->target_point && fabs(to_ab->target_point->y - 150) < LC_EPS &&
			 to_ab->target_point->x > 300 - LC_EPS && to_ab->target_point->x < 500 + LC_EPS,
			 "the link to the transition on its path", "l-c-ab");
	htree_print_document(doc);

	again = *half->target_point;
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(same_point(drawn->source_point, 400, 410) && same_point(half->target_point, again.x, again.y),
			 "a second update changes nothing", "idempotent");
	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
