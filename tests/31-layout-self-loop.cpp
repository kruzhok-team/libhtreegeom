/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layout test: the self-loop on the cross-axis end
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

static void run(HTFlowDirection direction)
{
	HTree* tree;
	HTDocument* doc = lc_document(&tree);
	HTLayoutOptions opts;
	HTreeNode* sm = lc_node(NULL, htTree, "sm", htRoleNone, 0.0, 0.0);
	HTreeNode* init = lc_point(sm, "init", htRoleInitial);
	HTreeNode* a = lc_state(sm, "a");
	HTreeNode* fin = lc_point(sm, "fin", htRoleFinal);
	HTreeEdge* e1;
	HTreeEdge* loop;
	const char* id = direction == htFlowDown ? "down" : "right";

	htree_add_node(tree, sm);
	e1 = lc_edge(tree, "e-init-a", init, a, 0.0, 0.0);
	loop = lc_edge(tree, "e-a-a", a, a, 60.0, 20.0);
	lc_edge(tree, "e-a-fin", a, fin, 0.0, 0.0);

	htree_default_layout_options(&opts);
	opts.direction = direction;
	printf("=== %s ===\n", id);
	printf("reconstruct: %d\n", htree_reconstruct_document_geometry(doc, 1, &opts));

	lc_check(lc_bends(loop) == 2 && loop->source_point && loop->target_point, "the loop polyline", id);
	if (loop->polyline && loop->polyline->next) {
		if (direction == htFlowDown) {
			lc_check(loop->polyline->point.x > a->rect->x + a->rect->width &&
					 loop->polyline->next->point.x > a->rect->x + a->rect->width, "the loop on the right", id);
		} else {
			lc_check(loop->polyline->point.y > a->rect->y + a->rect->height &&
					 loop->polyline->next->point.y > a->rect->y + a->rect->height, "the loop below", id);
		}
	}
	lc_check(loop->label_rect && !loop->label_point && !lc_rects_overlap(loop->label_rect, a->rect),
			 "the label beside the loop", id);
	lc_check(lc_bends(e1) == 0, "the other edges straight", id);
	lc_check(lc_check_containment(sm, opts.padding) == 0, "containment", id);

	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main(void)
{
	run(htFlowDown);
	run(htFlowRight);
	return lc_failures ? 1 : 0;
}
