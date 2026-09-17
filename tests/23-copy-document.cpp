/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The copy test: the document with several state machines
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

#include <stdio.h>
#include "htgeom.h"

int main()
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	const char* ids[] = { "sm-1", "sm-2", "sm-3" };

	for (int i = 0; i < 3; i++) {
		HTree* tree = htree_new_tree();
		htree_add_tree(doc, tree);
		HTreeNode* sm = htree_new_node(htTree, ids[i]);
		htree_node_set_rect(sm, i * 200, 0, 150, 100);
		htree_add_node(tree, sm);
		HTreeNode* state = htree_new_node(htSimpleNode, "s");
		htree_node_set_rect(state, i * 200 + 10, 10, 50, 40);
		htree_add_child_node(sm, state);
	}
	htree_build_bounding_rect(doc, &(doc->bounding_rect));

	printf("=== original ===\n");
	htree_print_document(doc);

	HTDocument* copy = htree_copy_document(doc);
	printf("=== copy ===\n");
	htree_print_document(copy);
	htree_destroy_document(copy);
	htree_destroy_document(doc);

	/* the empty document copies to an empty one */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	copy = htree_copy_document(doc);
	printf("=== empty copy ===\n");
	htree_print_document(copy);
	htree_destroy_document(copy);
	htree_destroy_document(doc);
	return 0;
}
