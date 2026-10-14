/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the update edge pieces stay in the container frame
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

#define LABEL_W 40.0
#define LABEL_H 20.0

/* the bends of a new labelled edge lie between its ends, on the line of their row */
static void check_between(const HTreeEdge* e, const HTreeNode* s, const HTreeNode* t, const char* id)
{
	int ok = e->polyline != NULL;
	for (const HTreePolyline* pl = e->polyline; pl; pl = pl->next) {
		ok = ok && pl->point.x > s->rect->x + s->rect->width - LC_EPS && pl->point.x < t->rect->x + LC_EPS &&
			fabs(pl->point.y - lc_cy(s)) < LC_EPS;
	}
	lc_check(ok, "the bends between the ends", id);
	lc_check(e->label_rect && e->label_rect->x > s->rect->x + s->rect->width - LC_EPS &&
			 e->label_rect->x + e->label_rect->width < t->rect->x + LC_EPS, "the label between the ends", id);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) a labelled new transition inside a drawn composite */
	{
		HTreeNode *sm, *comp, *r, *s1, *s2, *s3;
		HTreeEdge *e12, *e23;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 0, 0, 800, 400);
		comp = lc_composite(sm, "comp");
		htree_node_set_rect(comp, 50, 50, 700, 300);
		r = lc_region(comp, "r");
		htree_node_set_rect(r, 60, 80, 680, 260);
		s1 = lc_state(r, "s1");
		htree_node_set_rect(s1, 80, 100, 300, 200);
		s2 = lc_state(r, "s2");
		htree_node_set_rect(s2, 420, 100, 300, 200);
		s3 = lc_state(r, "s3");
		htree_add_node(tree, sm);
		e12 = lc_edge(tree, "e-s1-s2", s1, s2, 0.0, 0.0);
		htree_edge_set_points(e12, 380, 200, 420, 200);
		e23 = lc_edge(tree, "e-s2-s3", s2, s3, LABEL_W, LABEL_H);
		printf("=== composite ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(s1->rect, 80, 100, 300, 200) && lc_same_rect(s2->rect, 420, 100, 300, 200),
				 "the drawn children stay", "composite");
		lc_check(s3->rect && lc_cx(s3) > lc_cx(s2) && fabs(lc_cy(s3) - lc_cy(s2)) < LC_EPS, "s3 after s2", "composite");
		lc_check(comp->rect && fabs(comp->rect->x - 50.0) < LC_EPS && fabs(comp->rect->y - 50.0) < LC_EPS,
				 "the composite keeps its corner", "composite");
		check_between(e23, s2, s3, "composite");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "composite");
		lc_check(sm->rect && sm->rect->x + sm->rect->width >= comp->rect->x + comp->rect->width - LC_EPS &&
				 sm->rect->width < 1500.0, "the machine grows only to its content", "composite");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a machine border away from the origin */
	{
		HTreeNode *sm, *a, *b, *c, *d;
		HTreeEdge *cd, *e;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 100, 100, 1100, 300);
		a = lc_state(sm, "a");
		htree_node_set_rect(a, 150, 150, 300, 200);
		b = lc_state(sm, "b");
		htree_node_set_rect(b, 500, 150, 300, 200);
		c = lc_state(sm, "c");
		htree_node_set_rect(c, 850, 150, 300, 200);
		d = lc_state(sm, "d");
		htree_add_node(tree, sm);
		e = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		htree_edge_set_points(e, 450, 250, 500, 250);
		e = lc_edge(tree, "e-b-c", b, c, 0.0, 0.0);
		htree_edge_set_points(e, 800, 250, 850, 250);
		cd = lc_edge(tree, "e-c-d", c, d, LABEL_W, LABEL_H);
		printf("=== moved border ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(a->rect, 150, 150, 300, 200) && lc_same_rect(c->rect, 850, 150, 300, 200),
				 "the drawn states stay", "moved border");
		lc_check(d->rect && d->rect->x >= c->rect->x + c->rect->width - LC_EPS && fabs(lc_cy(d) - lc_cy(c)) < LC_EPS,
				 "d after c", "moved border");
		check_between(cd, c, d, "moved border");
		lc_check(fabs(sm->rect->x - 100.0) < LC_EPS && fabs(sm->rect->y - 100.0) < LC_EPS,
				 "the border keeps its corner", "moved border");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "moved border");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
