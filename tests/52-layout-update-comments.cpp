/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The update test: the new comments around the drawn content, the drawn ones kept
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

static int same(const HTreeNode* n, double x, double y, double w, double h)
{
	return lc_same_rect(n->rect, x, y, w, h);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTLayoutOptions opts;
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode *a, *b, *comp, *r, *i1, *i2, *c0, *n1, *n2, *n3, *n4;
	HTreeEdge *e, *l;
	HTreeRect again[4];

	htree_node_set_rect(sm, 0, 0, 800, 600);
	a = lc_state(sm, "a");
	htree_node_set_rect(a, 100, 100, 200, 100);
	b = lc_state(sm, "b");
	htree_node_set_rect(b, 500, 100, 200, 100);
	comp = lc_composite(sm, "comp");
	htree_node_set_rect(comp, 100, 250, 300, 120);
	r = lc_region(comp, "r");
	htree_node_set_rect(r, 110, 270, 280, 90);
	i1 = lc_state(r, "i1");
	htree_node_set_rect(i1, 120, 280, 100, 60);
	i2 = lc_state(r, "i2");
	htree_node_set_rect(i2, 260, 280, 100, 60);
	c0 = lc_node(sm, htComment, "c0", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(c0, 500, 400, 100, 40);
	/* the new comments: on b, two unlinked, one inside the composite on i1 */
	n1 = lc_node(sm, htComment, "n1", htRoleNone, 100.0, 40.0);
	n2 = lc_node(sm, htComment, "n2", htRoleNone, 100.0, 40.0);
	n3 = lc_node(sm, htComment, "n3", htRoleNone, 100.0, 40.0);
	n4 = lc_node(r, htComment, "n4", htRoleNone, 100.0, 40.0);
	htree_add_node(tree, sm);
	e = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	htree_edge_set_points(e, 300, 150, 500, 150);
	e = lc_edge(tree, "e-i1-i2", i1, i2, 0.0, 0.0);
	htree_edge_set_points(e, 220, 310, 260, 310);
	l = lc_edge(tree, "l-n1-b", n1, b, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);
	l = lc_edge(tree, "l-n4-i1", n4, i1, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);

	htree_default_layout_options(&opts);
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));

	lc_check(same(a, 100, 100, 200, 100) && same(b, 500, 100, 200, 100) && same(i1, 120, 280, 100, 60) &&
			 same(i2, 260, 280, 100, 60) && same(c0, 500, 400, 100, 40), "the drawn elements kept", "sm");
	lc_check(n1->rect && fabs(n1->rect->x - b->rect->x) < LC_EPS &&
			 fabs(n1->rect->y - (b->rect->y + b->rect->height + opts.node_gap)) < LC_EPS,
			 "the new comment under its drawn state (the inferred right-running flow)", "n1");
	lc_check(n2->rect && n3->rect && !lc_rects_overlap(n2->rect, n3->rect) &&
			 !lc_rects_overlap(n2->rect, c0->rect) && !lc_rects_overlap(n3->rect, c0->rect),
			 "the new unlinked comments on the shelf, apart", "n2");
	lc_check(n4->rect && lc_rect_inside(n4->rect, comp->rect, 0.0) && lc_rect_inside(n4->rect, r->rect, 0.0),
			 "the composite's new comment inside the grown composite", "n4");
	lc_check(fabs(comp->rect->x - 100) < LC_EPS && fabs(comp->rect->y - 250) < LC_EPS &&
			 comp->rect->height > 120 + LC_EPS, "the composite grew down from its corner", "comp");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment with the comments", "sm");
	lc_check(lc_count_comment_hits(tree) == 0, "no route over a comment", "sm");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "sm");
	htree_print_document(doc);

	again[0] = *n1->rect; again[1] = *n2->rect; again[2] = *n3->rect; again[3] = *comp->rect;
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(n1->rect, again[0].x, again[0].y, again[0].width, again[0].height) &&
			 lc_same_rect(n2->rect, again[1].x, again[1].y, again[1].width, again[1].height) &&
			 lc_same_rect(n3->rect, again[2].x, again[2].y, again[2].width, again[2].height) &&
			 lc_same_rect(comp->rect, again[3].x, again[3].y, again[3].width, again[3].height),
			 "a second update changes nothing", "idempotent");
	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
