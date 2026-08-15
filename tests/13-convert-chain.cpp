/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: the pairwise chain of the main formats
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

static void step(HTDocument* doc, HTCoordFormat n, HTCoordFormat e,
				 HTCoordFormat p, HTEdgeFormat ef, const char* name)
{
	printf("=== %s: %d ===\n", name,
		   htree_convert_document_geometry(doc, n, e, p, ef));
	htree_print_document(doc);
}

int main()
{
	HTDocument* doc = build_convert_doc();
	printf("=== original ===\n");
	htree_print_document(doc);

	step(doc, coordLeftTop, coordLeftTop, coordLeftTop, edgeBorder, "cyberiada10");
	step(doc, coordLocalCenter, coordLocalCenter, coordLocalCenter, edgeBorder, "qt");
	step(doc, coordAbsolute, coordLocalCenter, coordAbsolute, edgeCenter, "yed");
	step(doc, coordLeftTop, coordLeftTop, coordLeftTop, edgeBorder, "cyberiada10 again");
	step(doc, coordAbsolute, coordAbsolute, coordAbsolute, edgeBorder, "absolute");

	htree_destroy_document(doc);
	return 0;
}
