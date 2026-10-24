/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the comments inside their containers - the margins of their anchors, the shelf band
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

static int inside(const HTreeNode* n, const HTreeNode* parent)
{
	return n->rect && parent->rect && lc_rect_inside(n->rect, parent->rect, 0.0);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTLayoutOptions opts;
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* comp = lc_composite(sm, "comp");
	HTreeNode* r = lc_region(comp, "r");
	HTreeNode* i1 = lc_state(r, "i1");
	HTreeNode* i2 = lc_state(r, "i2");
	HTreeNode* i3 = lc_state(r, "i3");
	HTreeNode* lone = lc_state(sm, "lone");
	HTreeNode* b = lc_state(sm, "b");
	HTreeNode* c_in = lc_node(r, htComment, "c-in", htRoleNone, 120.0, 40.0);
	HTreeNode* c_up = lc_node(r, htComment, "c-up", htRoleNone, 120.0, 40.0);
	HTreeNode* lc = lc_node(lone, htComment, "lc", htRoleNone, 120.0, 40.0);
	HTreeNode* m1 = lc_node(sm, htComment, "m1", htRoleNone, 160.0, 60.0);
	HTreeNode* m2 = lc_node(sm, htComment, "m2", htRoleNone, 160.0, 60.0);
	HTreeNode* ct = lc_node(sm, htComment, "ct", htRoleNone, 160.0, 60.0);
	HTreeNode* cu = lc_node(sm, htComment, "cu", htRoleNone, 160.0, 60.0);
	HTreeEdge *t12, *l;

	htree_node_set_rect(sm, 0, 0, 100, 100);
	htree_add_node(tree, sm);
	lc_edge(tree, "e-a-comp", a, comp, 0.0, 0.0);
	t12 = lc_edge(tree, "e-i1-i2", i1, i2, 60.0, 30.0);
	lc_edge(tree, "e-i2-i3", i2, i3, 0.0, 0.0);
	lc_edge(tree, "e-comp-b", comp, b, 0.0, 0.0);
	lc_edge(tree, "e-b-lone", b, lone, 0.0, 0.0);
	/* a comment of the region on its sibling */
	l = lc_edge(tree, "l-cin-i3", c_in, i3, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);
	/* a comment of the region on an outer state: the region's shelf */
	l = lc_edge(tree, "l-cup-a", c_up, a, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);
	/* two machine comments on one nested state: stacked beside the composite */
	l = lc_edge(tree, "l-m1-i2", m1, i2, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);
	l = lc_edge(tree, "l-m2-i2", m2, i2, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, NULL);
	/* a machine comment on a nested transition: anchored by its source's composite */
	l = lc_edge(tree, "l-ct-t12", ct, i1, 0.0, 0.0);
	htree_edge_set_link(l, htEdgeLink, "e-i1-i2");
	l->target_edge = t12;

	htree_default_layout_options(&opts);
	opts.direction = htFlowDown;
	opts.mode = htLayoutAlternate;
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));

	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment with the comments", "sm");
	lc_check(inside(c_in, r) && inside(c_in, comp), "the region's comment inside the composite", "c-in");
	lc_check(inside(c_up, r), "the upward-linked comment on the region's shelf", "c-up");
	lc_check(inside(lc, lone) && lone->rect->width >= lc->rect->width + 2 * opts.padding,
			 "the lone comment inside its state", "lc");
	lc_check(m1->rect && comp->rect && fabs(m1->rect->x - (comp->rect->x + comp->rect->width + opts.node_gap)) < LC_EPS &&
			 fabs(m1->rect->y - comp->rect->y) < LC_EPS, "the first machine comment beside the composite", "m1");
	lc_check(m2->rect && fabs(m2->rect->x - m1->rect->x) < LC_EPS &&
			 fabs(m2->rect->y - (m1->rect->y + m1->rect->height + opts.node_gap)) < LC_EPS,
			 "the second one stacked under the first", "m2");
	lc_check(ct->rect && fabs(ct->rect->x - m1->rect->x) < LC_EPS &&
			 fabs(ct->rect->y - (m2->rect->y + m2->rect->height + opts.node_gap)) < LC_EPS,
			 "the transition comment stacked next", "ct");
	lc_check(cu->rect && inside(cu, sm) && cu->rect->y >= b->rect->y + b->rect->height + opts.layer_gap - LC_EPS &&
			 cu->rect->y >= ct->rect->y + ct->rect->height + opts.layer_gap - LC_EPS,
			 "the unlinked comment on the shelf band after everything", "cu");
	lc_check(lc_count_comment_hits(tree) == 0, "no route or label over a comment", "sm");
	for (const HTreeEdge* e = tree->edges; e; e = e->next) {
		if (e->source->type != htComment) continue;
		lc_check(e->source_point && e->target_point, "the link attached", e->id);
	}
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
