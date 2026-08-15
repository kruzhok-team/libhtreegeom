/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The conversion test: absolute to the three main formats and back
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

static void round_trip(HTCoordFormat n, HTCoordFormat e, HTCoordFormat p,
					   HTEdgeFormat ef, const char* name)
{
	HTDocument* doc = build_convert_doc();
	printf("=== %s ===\n", name);
	printf("to format: %d\n", htree_convert_document_geometry(doc, n, e, p, ef));
	htree_print_document(doc);
	printf("back to absolute: %d\n",
		   htree_convert_document_geometry(doc, coordAbsolute, coordAbsolute,
										   coordAbsolute, edgeBorder));
	htree_print_document(doc);
	htree_destroy_document(doc);
}

int main()
{
	HTDocument* doc = build_convert_doc();
	printf("=== original ===\n");
	htree_print_document(doc);
	htree_destroy_document(doc);

	round_trip(coordAbsolute, coordLocalCenter, coordAbsolute, edgeCenter, "yed");
	round_trip(coordLeftTop, coordLeftTop, coordLeftTop, edgeBorder, "cyberiada10");
	round_trip(coordLocalCenter, coordLocalCenter, coordLocalCenter, edgeBorder, "qt");
	return 0;
}
