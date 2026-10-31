/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the arrange of a drawn document
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

static int same_geometry(const HTreeNode* a, const HTreeNode* b)
{
	for (; a && b; a = a->next, b = b->next) {
		HTreeRect ra, rb;
		lc_bounds(a, &ra);
		lc_bounds(b, &rb);
		if (!lc_same_rect(&ra, rb.x, rb.y, rb.width, rb.height)) return 0;
		if (!same_geometry(a->children, b->children)) return 0;
	}
	return !a && !b;
}

int main(void)
{
	HTree *tree, *tree_full;
	HTDocument *doc, *full;
	HTreeNode *sm, *a, *b, *c, *note;
	HTreeEdge *ab, *bc, *link;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* a drawn chain with odd routes and a comment far away */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	a = lc_state(sm, "a");
	b = lc_state(sm, "b");
	c = lc_state(sm, "c");
	note = lc_node(sm, htComment, "note", htRoleNone, 0.0, 0.0);
	htree_add_node(tree, sm);
	htree_node_set_rect(a, 50, 50, 300, 200);
	htree_node_set_rect(b, 600, 60, 300, 200);
	htree_node_set_rect(c, 1150, 70, 300, 200);
	htree_node_set_rect(note, 2500, 1700, 200, 100);
	ab = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	htree_edge_set_points(ab, 300, 250, 650, 260);
	ab->polyline = htree_new_polyline_coord(500, 1000);
	bc = lc_edge(tree, "e-b-c", b, c, 0.0, 0.0);
	htree_edge_set_points(bc, 900, 160, 1150, 170);
	link = lc_edge(tree, "e-note-b", note, b, 0.0, 0.0);
	htree_edge_set_link(link, htEdgeLink, NULL);

	/* the full layout of the same structure */
	full = htree_copy_document(doc);
	tree_full = full->trees;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(full, 1, &opts));
	printf("arrange: %d\n", htree_arrange_document_geometry(doc, 1, &opts));
	lc_check(lc_cx(a) < lc_cx(b) && lc_cx(b) < lc_cx(c), "the drawn order kept", "drawn");
	lc_check(fabs(lc_cy(a) - lc_cy(b)) < LC_EPS && fabs(lc_cy(b) - lc_cy(c)) < LC_EPS, "one row", "drawn");
	lc_check(lc_bends(ab) == 0 && ab->source_point && fabs(ab->source_point->x - (a->rect->x + a->rect->width)) < LC_EPS,
			 "the drawn route rebuilt", "drawn");
	lc_check(same_geometry(tree->nodes, tree_full->nodes), "the full layout of the same order", "drawn");
	lc_check(lc_count_comment_hits(tree) == 0, "the comment clear", "drawn");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "drawn");
	htree_print_document(doc);
	htree_destroy_document(full);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
