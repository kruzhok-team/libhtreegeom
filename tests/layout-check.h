/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The shared builders and readability checks of the layout tests
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

#ifndef LAYOUT_CHECK_H
#define LAYOUT_CHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "htgeom.h"

#define LC_EPS 0.001
#define LC_MAX_CHAIN 64

static int lc_failures = 0;

static inline void lc_check(int ok, const char* what, const char* id)
{
	if (!ok) {
		fprintf(stderr, "%s failed for %s\n", what, id);
		lc_failures++;
	}
}

/* the builders */

static inline HTDocument* lc_document(HTree** tree)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	*tree = htree_new_tree();
	htree_add_tree(doc, *tree);
	return doc;
}

static inline HTreeNode* lc_node(HTreeNode* parent, HTNodeType type, const char* id, HTNodeRole role,
						  double min_w, double min_h)
{
	HTreeNode* node = htree_new_node(type, id);
	node->role = role;
	if (min_w > 0.0 || min_h > 0.0) {
		htree_node_set_min_size(node, min_w, min_h);
	}
	if (parent) {
		htree_add_child_node(parent, node);
	}
	return node;
}

static inline HTreeNode* lc_state(HTreeNode* parent, const char* id)
{
	return lc_node(parent, htSimpleNode, id, htRoleNone, 0.0, 0.0);
}

static inline HTreeNode* lc_point(HTreeNode* parent, const char* id, HTNodeRole role)
{
	return lc_node(parent, htPoint, id, role, 0.0, 0.0);
}

static inline HTreeNode* lc_composite(HTreeNode* parent, const char* id)
{
	return lc_node(parent, htCompositeNode, id, htRoleNone, 0.0, 0.0);
}

static inline HTreeNode* lc_region(HTreeNode* composite, const char* id)
{
	return lc_node(composite, htRegion, id, htRoleNone, 0.0, 0.0);
}

static inline HTreeEdge* lc_edge(HTree* tree, const char* id, HTreeNode* s, HTreeNode* t,
						  double label_w, double label_h)
{
	HTreeEdge* e = htree_new_edge(id, s->id, t->id);
	e->source = s;
	e->target = t;
	if (label_w > 0.0 || label_h > 0.0) {
		e->label_rect = htree_new_rect();
		e->label_rect->width = label_w;
		e->label_rect->height = label_h;
	}
	htree_add_edge(tree, e);
	return e;
}

/* the geometry readers */

static inline double lc_cx(const HTreeNode* n)
{
	if (n->rect) return n->rect->x + n->rect->width / 2.0;
	if (n->point) return n->point->x;
	return 0.0;
}

static inline double lc_cy(const HTreeNode* n)
{
	if (n->rect) return n->rect->y + n->rect->height / 2.0;
	if (n->point) return n->point->y;
	return 0.0;
}

static inline void lc_bounds(const HTreeNode* n, HTreeRect* r)
{
	if (n->rect) {
		*r = *n->rect;
	} else if (n->point) {
		r->x = n->point->x; r->y = n->point->y; r->width = r->height = 0.0;
	} else {
		r->x = r->y = r->width = r->height = 0.0;
	}
}

static inline int lc_bends(const HTreeEdge* e)
{
	int n = 0;
	for (const HTreePolyline* pl = e->polyline; pl; pl = pl->next) n++;
	return n;
}

/* the polyline of an edge from the source point to the target point */
static inline int lc_edge_chain(const HTreeEdge* e, HTreePoint* buf, int max)
{
	int n = 0;
	if (!e->source_point || !e->target_point) return 0;
	buf[n++] = *e->source_point;
	for (const HTreePolyline* pl = e->polyline; pl && n < max - 1; pl = pl->next) buf[n++] = pl->point;
	buf[n++] = *e->target_point;
	return n;
}

/* the checks */

static inline double lc_orient(HTreePoint a, HTreePoint b, HTreePoint c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static inline int lc_same_point(HTreePoint a, HTreePoint b)
{
	return fabs(a.x - b.x) < LC_EPS && fabs(a.y - b.y) < LC_EPS;
}

/* a proper crossing of two segments (shared end points excluded) */
static inline int lc_segments_cross(HTreePoint a, HTreePoint b, HTreePoint c, HTreePoint d)
{
	double o1, o2, o3, o4;
	if (lc_same_point(a, c) || lc_same_point(a, d) || lc_same_point(b, c) || lc_same_point(b, d)) return 0;
	o1 = lc_orient(a, b, c); o2 = lc_orient(a, b, d);
	o3 = lc_orient(c, d, a); o4 = lc_orient(c, d, b);
	return ((o1 > LC_EPS && o2 < -LC_EPS) || (o1 < -LC_EPS && o2 > LC_EPS)) &&
		((o3 > LC_EPS && o4 < -LC_EPS) || (o3 < -LC_EPS && o4 > LC_EPS));
}

static inline int lc_edge_is_transition(const HTreeEdge* e)
{
	return e->source && e->target && e->source->type != htComment && e->target->type != htComment;
}

/* H3: the crossings between the transitions */
static inline int lc_count_crossings(const HTree* tree)
{
	int total = 0;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		HTreePoint a[LC_MAX_CHAIN];
		int na;
		if (!lc_edge_is_transition(e)) continue;
		na = lc_edge_chain(e, a, LC_MAX_CHAIN);
		for (const HTreeEdge* f = e->next; f; f = f->next) {
			HTreePoint b[LC_MAX_CHAIN];
			int nb;
			if (!lc_edge_is_transition(f)) continue;
			nb = lc_edge_chain(f, b, LC_MAX_CHAIN);
			for (int i = 0; i + 1 < na; i++) {
				for (int j = 0; j + 1 < nb; j++) {
					if (lc_segments_cross(a[i], a[i + 1], b[j], b[j + 1])) total++;
				}
			}
		}
	}
	return total;
}

/* H2: the share of the transitions running forward along the flow axis */
static inline double lc_forward_fraction(const HTree* tree, int flow_is_y)
{
	int total = 0, forward = 0;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		double s, t;
		if (!lc_edge_is_transition(e) || e->source == e->target) continue;
		s = flow_is_y ? lc_cy(e->source) : lc_cx(e->source);
		t = flow_is_y ? lc_cy(e->target) : lc_cx(e->target);
		total++;
		if (s < t) forward++;
	}
	return total ? (double)forward / total : 1.0;
}

static inline int lc_rects_overlap(const HTreeRect* a, const HTreeRect* b)
{
	return a->x < b->x + b->width - LC_EPS && b->x < a->x + a->width - LC_EPS &&
		a->y < b->y + b->height - LC_EPS && b->y < a->y + a->height - LC_EPS;
}

static inline int lc_rect_inside(const HTreeRect* inner, const HTreeRect* outer, double pad)
{
	return inner->x >= outer->x + pad - LC_EPS && inner->y >= outer->y + pad - LC_EPS &&
		inner->x + inner->width <= outer->x + outer->width - pad + LC_EPS &&
		inner->y + inner->height <= outer->y + outer->height - pad + LC_EPS;
}

/* H1: the children inside their parent (with the padding), the siblings
   disjoint; the comments are not checked. Returns the violations */
static inline int lc_check_containment(const HTreeNode* parent, double pad)
{
	int bad = 0;
	for (const HTreeNode* c = parent->children; c; c = c->next) {
		HTreeRect r;
		if (c->type == htComment || (!c->rect && !c->point)) continue;
		lc_bounds(c, &r);
		if (parent->rect && !lc_rect_inside(&r, parent->rect, pad)) {
			fprintf(stderr, "%s escapes %s\n", c->id, parent->id);
			bad++;
		}
		for (const HTreeNode* d = c->next; d; d = d->next) {
			HTreeRect q;
			if (d->type == htComment || (!d->rect && !d->point)) continue;
			lc_bounds(d, &q);
			if (r.width > 0.0 && q.width > 0.0 && lc_rects_overlap(&r, &q)) {
				fprintf(stderr, "%s overlaps %s\n", c->id, d->id);
				bad++;
			}
		}
		if (c->children) bad += lc_check_containment(c, pad);
	}
	return bad;
}

static inline int lc_segment_crosses_rect(HTreePoint a, HTreePoint b, const HTreeRect* r)
{
	HTreePoint p[4];
	int n = 0;
	p[0].x = r->x; p[0].y = r->y;
	p[1].x = r->x + r->width; p[1].y = r->y;
	p[2].x = r->x + r->width; p[2].y = r->y + r->height;
	p[3].x = r->x; p[3].y = r->y + r->height;
	for (int i = 0; i < 4; i++) {
		if (lc_segments_cross(a, b, p[i], p[(i + 1) % 4])) n++;
	}
	return n;
}

/* 1 strictly inside the rect, -1 strictly outside, 0 on its border */
static inline int lc_rect_side(HTreePoint p, const HTreeRect* r)
{
	if (p.x < r->x - LC_EPS || p.x > r->x + r->width + LC_EPS ||
		p.y < r->y - LC_EPS || p.y > r->y + r->height + LC_EPS) return -1;
	if (p.x > r->x + LC_EPS && p.x < r->x + r->width - LC_EPS &&
		p.y > r->y + LC_EPS && p.y < r->y + r->height - LC_EPS) return 1;
	return 0;
}

/* the border crossings of an edge chain with a rect: the inside / outside
   changes along the chain (a bend on the border counts once) and the
   segments passing through the rect */
static inline int lc_count_border_crossings(const HTreeEdge* e, const HTreeRect* r)
{
	HTreePoint a[LC_MAX_CHAIN];
	int n = lc_edge_chain(e, a, LC_MAX_CHAIN), total = 0, last = 0;
	for (int i = 0; i < n; i++) {
		int s = lc_rect_side(a[i], r);
		if (s == 0) continue;
		if (last != 0 && s != last) total++;
		last = s;
	}
	for (int i = 0; i + 1 < n; i++) {
		if (lc_rect_side(a[i], r) == -1 && lc_rect_side(a[i + 1], r) == -1) {
			total += lc_segment_crosses_rect(a[i], a[i + 1], r);
		}
	}
	return total;
}

static inline int lc_is_ancestor(const HTreeNode* a, const HTreeNode* n)
{
	for (const HTreeNode* p = n ? n->parent : NULL; p; p = p->parent) if (p == a) return 1;
	return 0;
}

static inline int lc_line_rect_nodes(const HTreeNode* nodes, const HTreeEdge* e)
{
	int total = 0;
	for (const HTreeNode* n = nodes; n; n = n->next) {
		if (n->rect && n->type != htComment && n->type != htRegion && n->type != htTree &&
			n != e->source && n != e->target &&
			!lc_is_ancestor(n, e->source) && !lc_is_ancestor(n, e->target)) {
			total += lc_count_border_crossings(e, n->rect);
		}
		if (n->children) total += lc_line_rect_nodes(n->children, e);
	}
	return total;
}

/* H4: the transitions crossing the border of an unrelated state */
static inline int lc_count_line_rect_crossings(const HTree* tree)
{
	int total = 0;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		if (!lc_edge_is_transition(e)) continue;
		total += lc_line_rect_nodes(tree->nodes, e);
	}
	return total;
}

static inline int lc_finite_nodes(const HTreeNode* nodes)
{
	for (const HTreeNode* n = nodes; n; n = n->next) {
		if (n->rect && !(isfinite(n->rect->x) && isfinite(n->rect->y) &&
						 isfinite(n->rect->width) && isfinite(n->rect->height))) return 0;
		if (n->point && !(isfinite(n->point->x) && isfinite(n->point->y))) return 0;
		if (n->children && !lc_finite_nodes(n->children)) return 0;
	}
	return 1;
}

static inline int lc_all_finite(const HTree* tree)
{
	if (!lc_finite_nodes(tree->nodes)) return 0;
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		HTreePoint a[LC_MAX_CHAIN];
		int n = lc_edge_chain(e, a, LC_MAX_CHAIN);
		for (int i = 0; i < n; i++) if (!(isfinite(a[i].x) && isfinite(a[i].y))) return 0;
	}
	return 1;
}

#endif
