/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The bounding rect test: empty document and geometry-less nodes
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

#include "htgeom.h"

int main()
{
	/* no trees at all: the bounding rect stays empty */
	HTDocument* doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTreeRect* br = NULL;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	htree_print_document(doc);
	htree_destroy_document(doc);

	/* a tree with a geometry-less node: the bounding rect is zeroed */
	doc = htree_new_document(coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder);
	HTree* tree = htree_new_tree();
	htree_add_tree(doc, tree);
	HTreeNode* node = htree_new_node(htSimpleNode, "state");
	htree_add_node(tree, node);
	br = NULL;
	htree_build_bounding_rect(doc, &br);
	doc->bounding_rect = br;
	htree_print_document(doc);
	htree_destroy_document(doc);
	return 0;
}
