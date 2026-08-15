/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: the off-preset format combinations
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
#include "convert-doc.h"

int main()
{
	/* a mixed combination round trip: left-top nodes and polylines,
	   local-center edge points */
	HTDocument* doc = build_convert_doc();
	printf("=== mixed combination ===\n");
	printf("to mixed: %d\n",
		   htree_convert_document_geometry(doc, coordLeftTop, coordLocalCenter,
										   coordLeftTop, edgeBorder));
	htree_print_document(doc);
	printf("back to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* converting to coordNone is an error */
	doc = build_convert_doc();
	printf("convert to none: %d\n",
		   htree_convert_document_geometry(doc, coordNone, coordLeftTop,
										   coordLeftTop, edgeBorder));
	htree_destroy_document(doc);

	/* converting a coordNone document with geometry is an error */
	doc = htree_new_document(coordNone, coordNone, coordNone, edgeNone);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* a = htree_new_node(htSimpleNode, "a");
	htree_node_set_rect(a, 0, 0, 100, 100);
	htree_add_node(tree, a);
	printf("convert from none: %d\n",
		   htree_convert_document_geometry(doc, coordLeftTop, coordLeftTop,
										   coordLeftTop, edgeBorder));
	htree_destroy_document(doc);

	/* an edgeNone document without edges converts */
	doc = htree_new_document(coordLeftTop, coordLeftTop, coordLeftTop, edgeNone);
	tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* parent = htree_new_node(htCompositeNode, "parent");
	htree_node_set_rect(parent, 10, 10, 200, 100);
	htree_add_node(tree, parent);
	HTreeNode* inner = htree_new_node(htSimpleNode, "inner");
	htree_node_set_rect(inner, 20, 20, 50, 40);
	htree_add_child_node(parent, inner);
	printf("convert edge-none: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
