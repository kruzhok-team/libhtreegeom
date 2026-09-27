/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the preset minimum sizes
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

int main(void)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTLayoutOptions opts;
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* s1 = lc_node(sm, htSimpleNode, "s1", htRoleNone, 120.0, 50.0);
	HTreeNode* c = lc_node(sm, htCompositeNode, "c", htRoleNone, 600.0, 40.0);
	HTreeNode* r = lc_region(c, "r");
	HTreeNode* s2 = lc_node(r, htSimpleNode, "s2", htRoleNone, 100.0, 60.0);
	HTreeNode* s3 = lc_node(sm, htSimpleNode, "s3", htRoleNone, 500.0, 300.0);

	htree_add_node(tree, sm);
	lc_edge(tree, "e-s1-c", s1, c, 0.0, 0.0);
	lc_edge(tree, "e-c-s3", c, s3, 0.0, 0.0);

	htree_default_layout_options(&opts);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(s1->rect && fabs(s1->rect->width - 120.0) < LC_EPS && fabs(s1->rect->height - 50.0) < LC_EPS,
			 "the small state keeps its size", "s1");
	lc_check(s2->rect && fabs(s2->rect->width - 100.0) < LC_EPS && fabs(s2->rect->height - 60.0) < LC_EPS,
			 "the nested state keeps its size", "s2");
	lc_check(s3->rect && fabs(s3->rect->width - 500.0) < LC_EPS && fabs(s3->rect->height - 300.0) < LC_EPS,
			 "the big state keeps its size", "s3");
	lc_check(c->rect && c->rect->width >= 600.0 - LC_EPS, "the composite widened to its title", "c");
	lc_check(fabs(lc_cx(s2) - lc_cx(c)) < LC_EPS, "the content centred", "c");
	lc_check(r->rect && c->rect && r->rect->y >= c->rect->y + 40.0 + opts.padding - LC_EPS,
			 "the region below the title block", "c");
	lc_check(fabs(lc_cx(s1) - lc_cx(c)) < LC_EPS && fabs(lc_cx(c) - lc_cx(s3)) < LC_EPS,
			 "the chain aligned", "sm");
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", "sm");

	htree_print_document(doc);
	htree_destroy_document(doc);
	return lc_failures ? 1 : 0;
}
