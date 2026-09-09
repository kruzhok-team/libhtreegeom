/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: the top level of a tree without a root rect stays global
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

/* the content is not centred on the origin on purpose: a frame taken from
   the bounding rect would move it */
int main()
{
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 10, 20, 200, 100);
	htree_add_node(tree, a);
	HTreeNode* b = htree_new_node(htSimpleNode, "b");
	htree_node_set_rect(b, 300, 100, 100, 100);
	htree_add_node(tree, b);
	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	printf("absolute:\n");
	htree_print_document(doc);

	printf("to center-local: %d\n",
		   htree_convert_document_geometry(doc, coordLocalCenter, coordLocalCenter,
										   coordLocalCenter, edgeBorder));
	htree_print_document(doc);
	/* center-local at the top level is the global centre of each node */
	int ok = a->rect->x == 110 && a->rect->y == 70 && b->rect->x == 350 && b->rect->y == 150;
	printf("top level is global: %d\n", ok);

	printf("back to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	ok = a->rect->x == 10 && a->rect->y == 20 && b->rect->x == 300 && b->rect->y == 100;
	printf("round trip is identity: %d\n", ok);
	htree_destroy_document(doc);
	return 0;
}
