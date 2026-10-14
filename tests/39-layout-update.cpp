/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the geometry update seeded by the drawing
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
#include <string.h>

/* the drawn chain a -> b -> c running right in a bordered machine */
static HTDocument* drawn_chain(HTree** tree, HTreeNode** sm, HTreeNode** a, HTreeNode** b, HTreeNode** c)
{
	HTDocument* doc = lc_document(tree);
	*sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	htree_node_set_rect(*sm, 0, 0, 1100, 300);
	*a = lc_state(*sm, "a");
	htree_node_set_rect(*a, 50, 50, 300, 200);
	*b = lc_state(*sm, "b");
	htree_node_set_rect(*b, 400, 50, 300, 200);
	*c = lc_state(*sm, "c");
	htree_node_set_rect(*c, 750, 50, 300, 200);
	htree_add_node(*tree, *sm);
	return doc;
}

static HTreeEdge* drawn_edge(HTree* tree, const char* id, HTreeNode* s, HTreeNode* t,
							 double sx, double sy, double tx, double ty)
{
	HTreeEdge* e = lc_edge(tree, id, s, t, 0.0, 0.0);
	htree_edge_set_points(e, sx, sy, tx, ty);
	return e;
}

static int same_rect(const HTreeRect* r, double x, double y, double w, double h)
{
	return r && fabs(r->x - x) < LC_EPS && fabs(r->y - y) < LC_EPS &&
		fabs(r->width - w) < LC_EPS && fabs(r->height - h) < LC_EPS;
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTreeNode *sm, *a, *b, *c;
	HTreeEdge *ab, *bc;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) a new state spliced into the drawn chain */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeNode* x = lc_state(sm, "x");
		ab = drawn_edge(tree, "e-a-x", a, x, 350, 150, 400, 150);
		bc = drawn_edge(tree, "e-b-c", b, c, 700, 150, 750, 150);
		lc_edge(tree, "e-x-b", x, b, 0.0, 0.0);
		printf("=== splice ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(x->rect && lc_cx(a) < lc_cx(x) && lc_cx(x) < lc_cx(b) && lc_cx(b) < lc_cx(c),
				 "x between a and b, the chain order kept", "splice");
		lc_check(fabs(lc_cy(x) - lc_cy(a)) < LC_EPS && fabs(lc_cy(a) - 150.0) < LC_EPS &&
				 fabs(lc_cy(c) - 150.0) < LC_EPS, "the row kept", "splice");
		lc_check(same_rect(a->rect, 50, 50, 300, 200), "a stays", "splice");
		lc_check(bc->source_point && fabs(bc->source_point->x - (b->rect->x + b->rect->width)) < LC_EPS,
				 "b -> c attached to the moved b", "splice");
		lc_check(ab->target_point && fabs(ab->target_point->x - x->rect->x) < LC_EPS,
				 "a -> x attached to the new x", "splice");
		lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "splice");
		lc_check(lc_count_crossings(tree) == 0, "no crossings", "splice");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a new state appended after the chain */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeNode* d = lc_state(sm, "d");
		drawn_edge(tree, "e-a-b", a, b, 350, 150, 400, 150);
		bc = drawn_edge(tree, "e-b-c", b, c, 700, 150, 750, 150);
		lc_edge(tree, "e-c-d", c, d, 0.0, 0.0);
		printf("=== append ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(a->rect, 50, 50, 300, 200) && same_rect(b->rect, 400, 50, 300, 200) &&
				 same_rect(c->rect, 750, 50, 300, 200), "the drawn states stay", "append");
		lc_check(d->rect && d->rect->x >= c->rect->x + c->rect->width + opts.node_gap - LC_EPS &&
				 fabs(lc_cy(d) - lc_cy(c)) < LC_EPS, "d after c in the row", "append");
		lc_check(bc->source_point && fabs(bc->source_point->x - 700.0) < LC_EPS, "the drawn edge kept", "append");
		lc_check(sm->rect && sm->rect->x + sm->rect->width >= d->rect->x + d->rect->width + opts.padding - LC_EPS,
				 "the border grown", "append");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (c) a new transition between drawn states: nothing moves */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeEdge* ac;
		drawn_edge(tree, "e-a-b", a, b, 350, 150, 400, 150);
		drawn_edge(tree, "e-b-c", b, c, 700, 150, 750, 150);
		ac = lc_edge(tree, "e-a-c", a, c, 0.0, 0.0);
		printf("=== new edge ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(a->rect, 50, 50, 300, 200) && same_rect(b->rect, 400, 50, 300, 200) &&
				 same_rect(c->rect, 750, 50, 300, 200) && same_rect(sm->rect, 0, 0, 1100, 300),
				 "nothing moves", "new edge");
		lc_check(ac->source_point && ac->target_point, "the new edge routed", "new edge");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (d) a drawn composite gets a third child */
	{
		HTreeNode *comp, *r, *s1, *s2, *s3;
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
		drawn_edge(tree, "e-s1-s2", s1, s2, 380, 200, 420, 200);
		lc_edge(tree, "e-s2-s3", s2, s3, 0.0, 0.0);
		printf("=== composite ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(s1->rect, 80, 100, 300, 200) && same_rect(s2->rect, 420, 100, 300, 200),
				 "the drawn children stay", "composite");
		lc_check(s3->rect && lc_cx(s3) > lc_cx(s2) && fabs(lc_cy(s3) - lc_cy(s2)) < LC_EPS, "s3 after s2", "composite");
		lc_check(comp->rect && comp->rect->x + comp->rect->width >= s3->rect->x + s3->rect->width + opts.padding - LC_EPS &&
				 fabs(comp->rect->x - 50.0) < LC_EPS, "the composite grows to the right", "composite");
		lc_check(lc_check_containment(sm, 0.0) == 0, "containment", "composite");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (e) a drawn top-to-bottom chain keeps its direction */
	{
		HTreeNode* x;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 0, 0, 400, 800);
		a = lc_state(sm, "a");
		htree_node_set_rect(a, 50, 50, 300, 200);
		b = lc_state(sm, "b");
		htree_node_set_rect(b, 50, 300, 300, 200);
		c = lc_state(sm, "c");
		htree_node_set_rect(c, 50, 550, 300, 200);
		x = lc_state(sm, "x");
		htree_add_node(tree, sm);
		drawn_edge(tree, "e-a-b", a, b, 200, 250, 200, 300);
		drawn_edge(tree, "e-b-c", b, c, 200, 500, 200, 550);
		lc_edge(tree, "e-c-x", c, x, 0.0, 0.0);
		printf("=== down ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(a->rect, 50, 50, 300, 200) && same_rect(c->rect, 50, 550, 300, 200), "the column stays", "down");
		lc_check(x->rect && lc_cy(x) > lc_cy(c) && fabs(lc_cx(x) - lc_cx(c)) < LC_EPS, "x below c", "down");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (f) a fully drawn document: the update changes nothing */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTreeEdge* e1 = drawn_edge(tree, "e-a-b", a, b, 350, 150, 400, 150);
		HTreeEdge* e2 = drawn_edge(tree, "e-b-c", b, c, 700, 150, 750, 150);
		e1->label_rect = htree_new_rect();
		e1->label_rect->x = 355; e1->label_rect->y = 100; e1->label_rect->width = 40; e1->label_rect->height = 20;
		printf("=== idempotent ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(a->rect, 50, 50, 300, 200) && same_rect(b->rect, 400, 50, 300, 200) &&
				 same_rect(c->rect, 750, 50, 300, 200) && same_rect(sm->rect, 0, 0, 1100, 300), "the states stay", "idempotent");
		lc_check(e1->source_point && fabs(e1->source_point->x - 350.0) < LC_EPS && e1->label_rect &&
				 fabs(e1->label_rect->x - 355.0) < LC_EPS && e2->target_point && fabs(e2->target_point->x - 750.0) < LC_EPS,
				 "the edges stay", "idempotent");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (g) a drawn machine and a new one */
	doc = drawn_chain(&tree, &sm, &a, &b, &c);
	{
		HTree* second = htree_new_tree();
		HTreeNode* sm2 = lc_node(NULL, htTree, "sm2", htRoleNone, 0.0, 0.0);
		HTreeNode* p = lc_state(sm2, "p");
		HTreeNode* q = lc_state(sm2, "q");
		htree_add_tree(doc, second);
		htree_add_node(second, sm2);
		drawn_edge(tree, "e-a-b", a, b, 350, 150, 400, 150);
		lc_edge(second, "e-p-q", p, q, 0.0, 0.0);
		printf("=== machines ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(same_rect(sm->rect, 0, 0, 1100, 300), "the drawn machine stays", "machines");
		lc_check(sm2->rect && sm2->rect->x >= 1100.0 + 2 * opts.node_gap - LC_EPS, "the new machine after it", "machines");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
