/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the bands of a composite - the header (the title, entry and
 * internal actions) on top, the footer (the exit) at the bottom, the content
 * and its region between them
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

#define HEADER 70.0
#define FOOTER 40.0

/* a composite with a header and a footer, its two states in a region or
   directly in it */
static HTDocument* banded(HTree** tree, int with_region, HTreeNode** comp, HTreeNode** inner,
						  HTreeNode** a, HTreeNode** b)
{
	HTDocument* doc = lc_document(tree);
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* c = lc_composite(sm, "comp");
	HTreeNode* holder = with_region ? lc_region(c, "comp:") : c;
	htree_add_node(*tree, sm);
	htree_node_set_min_size(c, 200.0, HEADER);
	htree_node_set_footer(c, FOOTER);
	*a = lc_state(holder, "a");
	*b = lc_state(holder, "b");
	lc_edge(*tree, "e-a-b", *a, *b, 0.0, 0.0);
	*comp = c;
	*inner = holder;
	return doc;
}

/* the lowest bottom of the children */
static double content_bottom(const HTreeNode* n)
{
	double bottom = 0.0;
	for (const HTreeNode* c = n->children; c; c = c->next) {
		if (c->rect && c->rect->y + c->rect->height > bottom) bottom = c->rect->y + c->rect->height;
	}
	return bottom;
}

int main(void)
{
	const char* names[] = {"graph", "region"};
	HTLayoutOptions opts;
	htree_default_layout_options(&opts);
	for (int r = 0; r < 2; r++) {
		HTree* tree;
		HTreeNode *comp, *inner, *a, *b;
		HTDocument* doc = banded(&tree, r, &comp, &inner, &a, &b);
		double top, bottom;
		printf("=== %s ===\n", names[r]);
		printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));
		top = comp->rect->y + HEADER;
		bottom = comp->rect->y + comp->rect->height - FOOTER;
		lc_check(a->rect->y > top + LC_EPS && b->rect->y > top + LC_EPS, "the content below the header", names[r]);
		lc_check(fabs(bottom - opts.padding - content_bottom(inner)) < LC_EPS ||
				 (r && fabs(bottom - opts.padding - inner->rect->y - inner->rect->height) < LC_EPS),
				 "the content a padding above the footer", names[r]);
		if (r) {
			lc_check(inner->rect->y >= top - LC_EPS && inner->rect->y + inner->rect->height <= bottom + LC_EPS,
					 "the region between the bands", names[r]);
		}
		lc_check(lc_check_containment(tree->nodes, opts.padding) == 0, "containment", names[r]);
		/* the update keeps the footer under the drawn content */
		printf("update: %d\n", htree_update_document_geometry(doc, 1, &opts));
		lc_check(comp->rect->y + comp->rect->height - FOOTER - opts.padding >= content_bottom(inner) - LC_EPS,
				 "the update keeps the footer", names[r]);
		htree_print_document(doc);
		htree_destroy_document(doc);
	}
	/* no footer: the layout of before */
	{
		HTree* tree;
		HTreeNode *comp, *inner, *a, *b;
		HTDocument* doc = banded(&tree, 0, &comp, &inner, &a, &b);
		double with, without;
		htree_reconstruct_document_geometry(doc, 1, &opts);
		with = comp->rect->height;
		htree_destroy_document(doc);
		doc = banded(&tree, 0, &comp, &inner, &a, &b);
		htree_node_set_footer(comp, 0.0);
		htree_reconstruct_document_geometry(doc, 1, &opts);
		without = comp->rect->height;
		printf("=== no footer ===\n");
		lc_check(fabs(with - without - FOOTER) < LC_EPS, "the footer adds its height", "no footer");
		htree_destroy_document(doc);
	}
	return lc_failures ? 1 : 0;
}
