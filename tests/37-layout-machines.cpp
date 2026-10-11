/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: a document with several machines
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

static HTree* machine(HTDocument* doc, const char* prefix)
{
	char id[32];
	HTree* tree = htree_new_tree();
	HTreeNode* sm;
	HTreeNode* init;
	HTreeNode* a;
	HTreeNode* b;
	HTreeNode* fin;
	htree_add_tree(doc, tree);
	snprintf(id, sizeof(id), "%s", prefix);
	sm = lc_node(NULL, htTree, id, htRoleNone, 0.0, 0.0);
	snprintf(id, sizeof(id), "%s-init", prefix);
	init = lc_point(sm, id, htRoleInitial);
	snprintf(id, sizeof(id), "%s-a", prefix);
	a = lc_state(sm, id);
	snprintf(id, sizeof(id), "%s-b", prefix);
	b = lc_state(sm, id);
	snprintf(id, sizeof(id), "%s-fin", prefix);
	fin = lc_point(sm, id, htRoleFinal);
	htree_add_node(tree, sm);
	snprintf(id, sizeof(id), "%s-e1", prefix);
	lc_edge(tree, id, init, a, 0.0, 0.0);
	snprintf(id, sizeof(id), "%s-e2", prefix);
	lc_edge(tree, id, a, b, 0.0, 0.0);
	snprintf(id, sizeof(id), "%s-e3", prefix);
	lc_edge(tree, id, b, fin, 0.0, 0.0);
	return tree;
}

int main(void)
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTLayoutOptions opts;
	HTree* first = machine(doc, "m1");
	HTree* second = machine(doc, "m2");
	HTreeNode* note = lc_node(first->nodes, htComment, "m1-note", htRoleNone, 100.0, 40.0);
	HTreeRect* r1;
	HTreeRect* r2;

	htree_default_layout_options(&opts);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 0, &opts));

	r1 = first->nodes->rect;
	r2 = second->nodes->rect;
	lc_check(r1 && r2, "both machines get their rects", "machines");
	if (r1 && r2) {
		lc_check(!lc_rects_overlap(r1, r2), "the machines do not overlap", "machines");
		lc_check(r2->x >= r1->x + r1->width + 2 * opts.node_gap - LC_EPS, "the second machine right of the first", "machines");
		lc_check(fabs(r1->y - r2->y) < LC_EPS, "the tops aligned", "machines");
		lc_check(note->rect && !lc_rects_overlap(note->rect, r2), "the comment of the first machine clear of the second", "machines");
		lc_check(doc->bounding_rect && doc->bounding_rect->x <= r1->x + LC_EPS &&
				 doc->bounding_rect->x + doc->bounding_rect->width >= r2->x + r2->width - LC_EPS,
				 "the document bounding rect covers both", "machines");
	}
	lc_check(lc_check_containment(first->nodes, opts.padding) == 0 &&
			 lc_check_containment(second->nodes, opts.padding) == 0, "containment", "machines");
	lc_check(htree_check_geometry(doc) == HTREE_OK, "geometry check", "machines");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
