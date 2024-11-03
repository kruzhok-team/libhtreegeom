/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The the hierarchiceal tree geometry library: output procedures
 *
 * Copyright (C) 2024 Alexey Fedoseev <aleksey@fedoseev.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#ifndef __HIERARCHICAL_TREE_GEOMETRY_TYPES_H
#define __HIERARCHICAL_TREE_GEOMETRY_TYPES_H

#include <iostream>
#include "htgeom.h"

inline std::ostream& operator<<(std::ostream& os, const HTreePoint* point)
{
	if (point) {
		os << "(";
		os << "x: " << point->x << ", y: " << point->y;
		os << ")";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTreeRect* rect)
{
	if (rect) {
		os << "(";
		os << "x: " << rect->x << ", y: " << rect->y;
		os << ", w: " << rect->width << ", h: " << rect->height;
		os << ")";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTreePolyline* polyline)
{
	if (polyline) {
		os << "Polyline [";
		for (const HTreePolyline* pl = polyline; pl; pl = pl->next) {
			os << &(pl->point);
			if (pl->next) {
				os << ", ";
			}
		}
		os << "]";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTreeNode* node)
{
	if (node) {
		os << "HTreeNode {";
		os << "id: " << node->id;
		if (node->point) {
			os << ", point: " << node->point; 
		}
		if (node->rect) {
			os << ", rect: " << node->rect;
		}
		if (node->children) {
			os << ", children: [";
			for (HTreeNode* child = node->children; child; child = child->next) {
				os << child;
				if (child->next) {
					os << ", ";
				}
			}
			os << "]";
		}
		os << "}";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTreeEdge* edge)
{
	if (edge) {
		os << "HTreeEdge {";
		os << "id: " << edge->id << ", ";
		os << "source: " << edge->source_id << ", ";
		os << "target: " << edge->target_id;
		if (edge->source_point) {
			os << ", source point: " << edge->source_point; 
		}
		if (edge->target_point) {
			os << ", target point: " << edge->target_point; 
		}
		if (edge->label_point) {
			os << ", label point: " << edge->label_point; 
		} else if (edge->label_rect) {
			os << ", label rect: " << edge->label_rect; 
		}
		if (edge->polyline) {
			os << ", polyline: " << edge->polyline; 
		}
		os << "}";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTree* tree)
{
	if (tree) {
		os << "HTree {";
		os << "nodes: [";
		for (HTreeNode* node = tree->nodes; node; node = node->next) {
			os << node;
			if (node->next) {
				os << ", ";
			}
		}
		os << "], edges: [";
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			os << edge;
			if (edge->next) {
				os << ", ";
			}
		}
		os << "]}";
	}
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const HTDocument* doc)
{
	if (doc) {
		os << "HTreeDocument {";
		os << "nodes coord: " << doc->node_coord_format <<
			", edge coord: " << doc->edge_coord_format <<
			", edge polylines coord: " << doc->edge_pl_coord_format <<
			", edge format: " << doc->edge_format;
		os << ", trees: [";
		for (HTree* tree = doc->trees; tree; tree = tree->next) {
			os << tree;
			if (tree->next) {
				os << ", ";
			}
		}
		os << "], bounding rect: ";
		if (doc->bounding_rect) {
			os << doc->bounding_rect;
		} else {
			os << "()";
		}
		os << "}";
	}
	return os;
}

#endif
