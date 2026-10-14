/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a container new in the update gets the title band
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

#define TITLE_W 300.0
#define TITLE_H 36.0

/* a composite with one region holding an initial and a state */
static HTreeNode* composite(HTree* tree, HTreeNode* parent, const char* id, HTreeNode** region, HTreeNode** inner)
{
	char name[32];
	HTreeNode* node = lc_node(parent, htCompositeNode, id, htRoleNone, TITLE_W, TITLE_H);
	HTreeNode* init;
	snprintf(name, sizeof(name), "%s-r", id);
	*region = lc_region(node, name);
	snprintf(name, sizeof(name), "%s-i", id);
	init = lc_point(*region, name, htRoleInitial);
	snprintf(name, sizeof(name), "%s-z", id);
	*inner = lc_state(*region, name);
	snprintf(name, sizeof(name), "e-%s-i-z", id);
	lc_edge(tree, name, init, *inner, 0.0, 0.0);
	return node;
}

/* the region starts below the title band of its composite */
static void check_title(const HTreeNode* node, const HTreeNode* region, const HTLayoutOptions* opts, const char* id)
{
	lc_check(node->rect && region->rect &&
			 fabs(region->rect->y - (node->rect->y + TITLE_H + opts->padding)) < LC_EPS,
			 "the region below the title band", id);
	lc_check(lc_check_containment(node, 0.0) == 0, "containment", id);
}

int main(void)
{
	HTree* tree;
	HTDocument* doc;
	HTLayoutOptions opts;

	htree_default_layout_options(&opts);

	/* (a) a new composite beside a drawn state */
	{
		HTreeNode *sm, *a, *q, *r, *z;
		HTreeEdge* e;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 0, 0, 400, 300);
		a = lc_state(sm, "a");
		htree_node_set_rect(a, 50, 50, 300, 200);
		q = composite(tree, sm, "q", &r, &z);
		htree_add_node(tree, sm);
		e = lc_edge(tree, "e-a-q", a, q, 0.0, 0.0);
		(void)e;
		printf("=== new composite ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(a->rect, 50, 50, 300, 200), "a stays", "new composite");
		check_title(q, r, &opts, "new composite");
		lc_check(lc_check_containment(sm, 0.0) == 0, "machine containment", "new composite");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (b) a drawn composite gets its first region */
	{
		HTreeNode *sm, *a, *b, *r, *init, *z;
		HTreeEdge* e;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		htree_node_set_rect(sm, 0, 0, 750, 300);
		a = lc_state(sm, "a");
		htree_node_set_rect(a, 50, 50, 300, 200);
		b = lc_node(sm, htCompositeNode, "b", htRoleNone, TITLE_W, TITLE_H);
		htree_node_set_rect(b, 400, 50, 300, 200);
		r = lc_region(b, "b-r");
		init = lc_point(r, "b-i", htRoleInitial);
		z = lc_state(r, "b-z");
		htree_add_node(tree, sm);
		e = lc_edge(tree, "e-a-b", a, b, 0.0, 0.0);
		htree_edge_set_points(e, 350, 150, 400, 150);
		lc_edge(tree, "e-b-i-z", init, z, 0.0, 0.0);
		printf("=== new region ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 0, &opts));
		lc_check(lc_same_rect(a->rect, 50, 50, 300, 200), "a stays", "new region");
		lc_check(fabs(b->rect->x - 400.0) < LC_EPS && fabs(b->rect->y - 50.0) < LC_EPS, "b keeps its corner", "new region");
		check_title(b, r, &opts, "new region");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	/* (c) nothing drawn: the update gives the insets of the full layout */
	{
		HTreeNode *sm, *a, *q, *r, *z;
		doc = lc_document(&tree);
		sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
		a = lc_state(sm, "a");
		q = composite(tree, sm, "q", &r, &z);
		htree_add_node(tree, sm);
		lc_edge(tree, "e-a-q", a, q, 0.0, 0.0);
		printf("=== nothing drawn ===\n");
		printf("update: %d\n", htree_update_document_geometry(doc, 1, &opts));
		check_title(q, r, &opts, "nothing drawn");
		lc_check(sm->rect && lc_check_containment(sm, 0.0) == 0, "machine containment", "nothing drawn");
		htree_print_document(doc);
	}
	htree_destroy_document(doc);

	return lc_failures ? 1 : 0;
}
