/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a drawn group put into a new container keeps its drawing
 * and its routes
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

/* the bends of an edge relative to a node corner */
static int bends_from(const HTreeEdge* e, const HTreeNode* n, HTreePoint* buf, int max)
{
	int k = 0;
	for (const HTreePolyline* pl = e->polyline; pl && k < max; pl = pl->next, k++) {
		buf[k].x = pl->point.x - n->rect->x;
		buf[k].y = pl->point.y - n->rect->y;
	}
	return k;
}

static int same_bends(const HTreePoint* a, int na, const HTreePoint* b, int nb)
{
	if (na != nb || na == 0) return 0;
	for (int i = 0; i < na; i++) {
		if (!lc_same_point(a[i], b[i])) return 0;
	}
	return 1;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *x, *w, *wr, *a, *ar, *s1, *s2, *b;
	HTreeEdge *e12, *eab, *exw;
	HTreePolyline* pl;
	HTreePoint inner[LC_MAX_CHAIN], outer[LC_MAX_CHAIN], now[LC_MAX_CHAIN];
	int ninner, nouter, nnow;
	double s1x, s1y, s2x, s2y, bx, by;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* x stays in the machine; the drawn composite a with s1, s2 and the
	   drawn b went into the new composite w */
	doc = lc_document(&tree);
	sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(sm, 0, 0, 1300, 450);
	x = lc_state(sm, "x");
	htree_node_set_rect(x, 50, 150, 200, 100);
	w = lc_composite(sm, "w");
	wr = lc_region(w, "wr");
	a = lc_composite(wr, "a");
	htree_node_set_rect(a, 350, 50, 500, 350);
	ar = lc_region(a, "ar");
	s1 = lc_state(ar, "s1");
	htree_node_set_rect(s1, 400, 120, 200, 60);
	s2 = lc_state(ar, "s2");
	htree_node_set_rect(s2, 400, 300, 200, 60);
	b = lc_state(wr, "b");
	htree_node_set_rect(b, 1000, 150, 200, 100);
	htree_add_node(tree, sm);

	e12 = lc_edge(tree, "e-s1-s2", s1, s2, 40.0, 20.0);
	htree_edge_set_points(e12, 600, 150, 600, 330);
	pl = htree_new_polyline_coord(700, 150);
	htree_polyline_add_point(pl, 700, 330);
	e12->polyline = pl;
	e12->label_rect->x = 710;
	e12->label_rect->y = 230;
	eab = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
	htree_edge_set_points(eab, 850, 100, 1100, 150);
	eab->polyline = htree_new_polyline_coord(1100, 100);
	exw = lc_edge(tree, "e-x-w", x, w, 0.0, 0.0);

	ninner = bends_from(e12, a, inner, LC_MAX_CHAIN);
	nouter = bends_from(eab, a, outer, LC_MAX_CHAIN);
	s1x = s1->rect->x - a->rect->x;
	s1y = s1->rect->y - a->rect->y;
	s2x = s2->rect->x - a->rect->x;
	s2y = s2->rect->y - a->rect->y;
	bx = b->rect->x - a->rect->x;
	by = b->rect->y - a->rect->y;

	printf("=== wrap ===\n");
	printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
	lc_check(lc_same_rect(x->rect, 50, 150, 200, 100), "x stays", "wrap");
	lc_check(w->rect && fabs(a->rect->width - 500) < LC_EPS && fabs(a->rect->height - 350) < LC_EPS &&
			 fabs(b->rect->width - 200) < LC_EPS && fabs(b->rect->height - 100) < LC_EPS,
			 "the group keeps its sizes", "wrap");
	lc_check(fabs(s1->rect->x - a->rect->x - s1x) < LC_EPS && fabs(s1->rect->y - a->rect->y - s1y) < LC_EPS &&
			 fabs(s2->rect->x - a->rect->x - s2x) < LC_EPS && fabs(s2->rect->y - a->rect->y - s2y) < LC_EPS &&
			 fabs(b->rect->x - a->rect->x - bx) < LC_EPS && fabs(b->rect->y - a->rect->y - by) < LC_EPS,
			 "the group keeps its drawing", "wrap");
	nnow = bends_from(e12, a, now, LC_MAX_CHAIN);
	lc_check(same_bends(inner, ninner, now, nnow), "the inner route moves with the group", "wrap");
	lc_check(e12->label_rect && fabs(e12->label_rect->x - a->rect->x - 360) < LC_EPS &&
			 fabs(e12->label_rect->y - a->rect->y - 180) < LC_EPS, "the label moves with the group", "wrap");
	nnow = bends_from(eab, a, now, LC_MAX_CHAIN);
	lc_check(same_bends(outer, nouter, now, nnow), "the route between the members moves with them", "wrap");
	lc_check(exw->source_point && exw->target_point, "the new edge routed", "wrap");
	lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "wrap");
	htree_print_document(doc);
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
