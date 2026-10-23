/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The reconstruction test: the comment links to a state, to a transition
 * and to a fragment get their ends
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

static int on_border(const HTreePoint* p, const HTreeRect* r)
{
	int in_x = p->x > r->x - LC_EPS && p->x < r->x + r->width + LC_EPS;
	int in_y = p->y > r->y - LC_EPS && p->y < r->y + r->height + LC_EPS;
	return (in_y && (fabs(p->x - r->x) < LC_EPS || fabs(p->x - r->x - r->width) < LC_EPS)) ||
		(in_x && (fabs(p->y - r->y) < LC_EPS || fabs(p->y - r->y - r->height) < LC_EPS));
}

/* the distance of p from the path of a two-point edge */
static double off_path(const HTreePoint* p, const HTreeEdge* e)
{
	double dx = e->target_point->x - e->source_point->x, dy = e->target_point->y - e->source_point->y;
	double len = sqrt(dx * dx + dy * dy);
	return fabs((p->x - e->source_point->x) * dy - (p->y - e->source_point->y) * dx) / len;
}

/* a -> b, the comment c linked to b, to the transition and to a fragment of
   b carrying a stale target point */
static HTDocument* build(HTree** tree, HTreeNode** b, HTreeNode** c,
						 HTreeEdge** ab, HTreeEdge** to_b, HTreeEdge** to_ab, HTreeEdge** chunk)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* a;
	htree_node_set_rect(sm, 0, 0, 800, 600);
	a = lc_state(sm, "a");
	htree_node_set_rect(a, 100, 100, 200, 100);
	*b = lc_state(sm, "b");
	htree_node_set_rect(*b, 500, 100, 200, 100);
	*c = lc_node(sm, htComment, "c", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*c, 300, 400, 100, 40);
	htree_add_node(*tree, sm);
	*ab = lc_edge(*tree, "e-a-b", a, *b, 0.0, 0.0);
	*to_b = lc_edge(*tree, "l-c-b", *c, *b, 0.0, 0.0);
	htree_edge_set_link(*to_b, htEdgeLink, NULL);
	*to_ab = lc_edge(*tree, "l-c-ab", *c, a, 0.0, 0.0);
	htree_edge_set_link(*to_ab, htEdgeLink, "e-a-b");
	(*to_ab)->target_edge = *ab;
	*chunk = lc_edge(*tree, "l-c-b#1", *c, *b, 0.0, 0.0);
	htree_edge_set_link(*chunk, htEdgeChunkLink, NULL);
	(*chunk)->target_point = htree_new_point_coord(600, 100);
	return doc;
}

static void check_links(HTreeNode* b, HTreeNode* c, HTreeEdge* ab, HTreeEdge* to_b,
						HTreeEdge* to_ab, HTreeEdge* chunk, const char* what)
{
	lc_check(to_b->source_point && on_border(to_b->source_point, c->rect) &&
			 to_b->target_point && on_border(to_b->target_point, b->rect), "the link to b on the borders", what);
	lc_check(ab->source_point && ab->target_point && to_ab->target_point &&
			 off_path(to_ab->target_point, ab) < LC_EPS, "the link to the transition ends on its path", what);
	lc_check(to_ab->source_point && on_border(to_ab->source_point, c->rect), "its source on c", what);
	lc_check(chunk->source_point && on_border(chunk->source_point, c->rect) && !chunk->target_point,
			 "the fragment link without a target point", what);
}

int main(void)
{
	HTree* tree;
	HTDocument *doc, *copy;
	HTreeNode *b, *c;
	HTreeEdge *ab, *to_b, *to_ab, *chunk;
	HTLayoutOptions opts;

	/* the preserving reconstruction: the transition first, the links on it */
	doc = build(&tree, &b, &c, &ab, &to_b, &to_ab, &chunk);
	printf("=== preserving ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	check_links(b, c, ab, to_b, to_ab, chunk, "preserving");
	lc_check(to_ab->target_point && fabs(to_ab->target_point->x - 350) < LC_EPS &&
			 fabs(to_ab->target_point->y - 150) < LC_EPS, "the nearest point of the path", "preserving");
	lc_check(to_ab->source_point && fabs(to_ab->source_point->x - 350) < LC_EPS &&
			 fabs(to_ab->source_point->y - 400) < LC_EPS, "straight up from the comment", "preserving");
	htree_print_document(doc);

	/* the copy keeps the kinds and the commented edge */
	copy = htree_copy_document(doc);
	lc_check(copy && copy->trees->edges->next->next->type == htEdgeLink &&
			 copy->trees->edges->next->next->target_edge == copy->trees->edges,
			 "the copy resolves the commented edge", "copy");
	htree_destroy_document(copy);

	/* the Qt frame: the link to the transition is local to its source a */
	printf("convert: %d\n", htree_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter,
															 coordLocalCenter, edgeBorder));
	lc_check(to_ab->target_point && fabs(to_ab->target_point->x - 150) < LC_EPS &&
			 fabs(to_ab->target_point->y) < LC_EPS, "local to the centre of a", "qt");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* a label over the nearest point: the foot keeps to its border, off the arrow */
	doc = build(&tree, &b, &c, &ab, &to_b, &to_ab, &chunk);
	ab->label_rect = htree_new_rect();
	ab->label_rect->x = 320;
	ab->label_rect->y = 140;
	ab->label_rect->width = 60;
	ab->label_rect->height = 20;
	printf("=== label ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	lc_check(to_ab->target_point && fabs(to_ab->target_point->x - 380) < LC_EPS &&
			 fabs(to_ab->target_point->y - 150) < LC_EPS, "the foot beside the label", "label");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* a label over the whole free middle: the foot beside it, inside the margin */
	doc = build(&tree, &b, &c, &ab, &to_b, &to_ab, &chunk);
	ab->label_rect = htree_new_rect();
	ab->label_rect->x = 335;
	ab->label_rect->y = 140;
	ab->label_rect->width = 130;
	ab->label_rect->height = 20;
	printf("=== wide label ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, NULL));
	lc_check(to_ab->target_point && fabs(to_ab->target_point->x - 335) < LC_EPS &&
			 fabs(to_ab->target_point->y - 150) < LC_EPS, "the foot beside the wide label", "wide label");
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* the layered layout attaches the links after placing the comment */
	doc = build(&tree, &b, &c, &ab, &to_b, &to_ab, &chunk);
	htree_default_layout_options(&opts);
	printf("=== layout ===\n");
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));
	check_links(b, c, ab, to_b, to_ab, chunk, "layout");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "layout");
	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
