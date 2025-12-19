/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The the hierarchiceal tree geometry library
 *
 * Copyright (C) 2024-2025 Alexey Fedoseev <aleksey@fedoseev.net>
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

#include <iostream>
#include <vector>

#include "homog2d.hpp"

#include "htgeom.h"
#include "htgeom_types.h"

#ifdef __DEBUG__
#define DEBUG      std::cerr  
#else
#define DEBUG
#endif

#define PADDING             10
#define NODE_WIDTH          300
#define NODE_HEIGHT         200

/* -----------------------------------------------------------------------------
 * Geometry conversions: lib to homog2d and back
 * ----------------------------------------------------------------------------- */

h2d::Point2dD htree_point_to_homog(const HTreePoint* point)
{
	if (point) {
		return h2d::Point2dD(point->x, point->y);
	}
	return h2d::Point2dD();
}

HTreePoint* homog_point_to_htree(const h2d::Point2dD& hgpoint)
{
	HTreePoint* point = htree_new_point();
	point->x = hgpoint.getX();
//	if (point->x != 0.0 && abs(point->x) < 0.000001) point->x = 0.0;
	point->y = hgpoint.getY();
//	if (point->y != 0.0 && abs(point->y) < 0.000001) point->y = 0.0;
	return point;
}

int homog_point_to_htree(const h2d::Point2dD& hgpoint, HTreePoint& point)
{
	point.x = hgpoint.getX();
//	if (point.x != 0.0 && abs(point.x) < 0.000001) point.x = 0.0;
	point.y = hgpoint.getY();
//	if (point.y != 0.0 && abs(point.y) < 0.000001) point.y = 0.0;
	return HTREE_OK;
}

h2d::FRectD htree_rect_to_homog(const HTreeRect* rect)
{
	if (rect) {
		return h2d::FRectD(rect->x, rect->y,
						   rect->x + rect->width, rect->y + rect->height);
	}
	return h2d::FRectD();
}

HTreeRect* homog_rect_to_htree(const h2d::FRectD& hgrect)
{
	HTreeRect* rect = htree_new_rect();
	h2d::Point2dD p = hgrect.getPts().first;
	rect->x = p.getX();
	rect->y = p.getY();
	rect->width = hgrect.width();
	rect->height = hgrect.height();
	return rect;
}

int homog_rect_to_htree(const h2d::FRectD& hgrect, HTreeRect& rect)
{
	h2d::Point2dD p = hgrect.getPts().first;
	rect.x = p.getX();
	rect.y = p.getY();
	rect.width = hgrect.width();
	rect.height = hgrect.height();
	return HTREE_OK;
}

h2d::OPolylineD htree_polyline_to_homog(const HTreePoint* source, const HTreePoint* target,
										const HTreePolyline* polyline)
{
	std::vector<h2d::Point2dD> points;
	points.push_back(htree_point_to_homog(source));
	if (polyline->next) {
		/* more than one point */
		while (polyline) {
			points.push_back(htree_point_to_homog(&(polyline->point)));
			polyline = polyline->next;
		}
	}
	points.push_back(htree_point_to_homog(target));
	return h2d::OPolyline(points);
}

HTreePolyline* homog_polyline_to_htree(const h2d::OPolylineD& hgpolyline)
{
	HTreePolyline* polyline = NULL;
	
	if (hgpolyline.size() > 0) {
		std::vector<h2d::Point2dD> points = hgpolyline.getPts();
		
		HTreePolyline* prev = NULL;

		for (std::vector<h2d::Point2dD>::const_iterator i = points.begin(); i != points.end(); i++) {
			if (i == points.begin() || std::next(i) == points.end()) {
				// skip the first and last point
				continue;
			}
			
			HTreePolyline* pl = htree_new_polyline();
			pl->point.x = i->getX();
			pl->point.y = i->getY();			
			if (!polyline) {
				polyline = pl;
				prev = pl;
			} else {
				prev->next = pl;
				prev = pl;
			}
		}
	}

	return polyline;
}

static int htree_get_nodes_collections(const HTreeNode* nodes,
									   std::vector<h2d::Point2dD>& points,
									   std::vector<h2d::FRectD>& rects)
{
	if (!nodes) {
		return HTREE_BAD_PARAMETER;
	}
	for (const HTreeNode* node = nodes; node; node = node->next) {
		if (node->point) {
			points.push_back(htree_point_to_homog(node->point));
		}
		if (node->rect) {
			rects.push_back(htree_rect_to_homog(node->rect));
		}
		if (node->children) {
			int res = htree_get_nodes_collections(node->children, points, rects);
			if (res != HTREE_OK) {
				return res;
			}
		}
	}
	return HTREE_OK;
}

static int htree_get_tree_collections(const HTree* tree,
									  std::vector<h2d::Point2dD>& points,
									  std::vector<h2d::FRectD>& rects,
									  std::vector<h2d::OPolyline>& polylines)
{
	if (!tree) {
		return HTREE_BAD_PARAMETER;
	}

	if (tree->nodes) {
		int res = htree_get_nodes_collections(tree->nodes, points, rects);
		if (res != HTREE_OK) {
			return res;
		}
	}
	
	if (tree->edges) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (!edge->source || edge->target) continue;
			if (edge->polyline) {
				HTreePoint source, target;

				if (edge->source_point) {
					source = *(edge->source_point);
				} else if (edge->source->rect) {
					HTreePoint* center = htree_rect_center_point(edge->source->rect, coordAbsolute);
					source = *center;
					htree_destroy_point(center);
				} else if (edge->source->point) {
					source = *(edge->source->point);
				} else {
					continue;
				}

				if (edge->target_point) {
					target = *(edge->target_point);
				} else if (edge->target->rect) {
					HTreePoint* center = htree_rect_center_point(edge->target->rect, coordAbsolute);
					target = *center;
					htree_destroy_point(center);
				} else if (edge->target->point) {
					source = *(edge->target->point);
				} else {
					continue;
				}
				
				polylines.push_back(htree_polyline_to_homog(&source, &target, edge->polyline));
			}
			if (edge->label_point) {
				points.push_back(htree_point_to_homog(edge->label_point));
			}
			if (edge->label_rect) {
				rects.push_back(htree_rect_to_homog(edge->label_rect));
			}
		}
	}
	
	return HTREE_OK;
}

static int htree_get_collections(const HTree* trees,
								 std::vector<h2d::Point2dD>& points,
								 std::vector<h2d::FRectD>& rects,
								 std::vector<h2d::OPolyline>& polylines)
{
	if (!trees) {
		return HTREE_BAD_PARAMETER;
	}

	points.clear();
	rects.clear();
	polylines.clear();
	
	for (const HTree* tree = trees; tree; tree = tree->next) {
		int res = htree_get_tree_collections(tree, points, rects, polylines);
		if (res != HTREE_OK) return res;
	}

	return HTREE_OK;
}

static int htree_has_toplevel_rect(const HTDocument* doc)
{
	if (!doc) return 0;
	if (!doc->trees) return 0;
	
	int found = 0;
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		if (tree->nodes && htree_node_has_toplevel_geometry(tree->nodes)) {
			if (found) return 0;
			found = 1;
		}
	}
	return found;
}

/* -----------------------------------------------------------------------------
 * Base geometry transformations
 * ----------------------------------------------------------------------------- */

/*static int htree_extend_rect_point(HTreeRect** result, double x, double y)
{
	double delta;
	if (!result) {
		return HTREE_BAD_PARAMETER;
	}
	if (!*result) {
		*result = htree_new_rect();
	}
	HTreeRect* rect = *result;
	if (htree_empty_rect(rect)) {
		rect->x = x;
		rect->y = y;
		return HTREE_OK;
	}
	if (x < rect->x) {
		delta = rect->x - x;
		rect->width += delta;
		rect->x -= delta;
	}
	if (x > rect->x + rect->width) {
		delta = x - rect->x - rect->width;
		rect->width += delta;
	}
	if (y < rect->y) {
		delta = rect->y - y;
		rect->height += delta;
		rect->y -= delta;
	}
	if (y > rect->y + rect->height) {
		delta = y - rect->y - rect->height;
		rect->height += delta;
	}
	return HTREE_OK;	
}

static int htree_extend_rect_polyline(HTreeRect** result, HTreePolyline* polyline)
{
	double delta;
	if (!result || !polyline) {
		return HTREE_BAD_PARAMETER;
	}
	for (HTreePolyline* pl = polyline; pl; pl = pl->next) {
		htree_extend_rect_point(result, pl->point.x, pl->point.y);
	}
	return HTREE_OK;
}

static int htree_extend_rect(HTreeRect** result, HTreeRect* src)
{
	double delta;
	if (!result || !src) {
		return HTREE_BAD_PARAMETER;
	}
	htree_extend_rect_point(result, src->x, src->y);
	htree_extend_rect_point(result, src->x + src->width, src->y + src->height);	
	return HTREE_OK;	
	}*/

/* -----------------------------------------------------------------------------
 * Geometry transformations implementation: format to absolute
 * ----------------------------------------------------------------------------- */

static int htree_convert_point_geometry_to_absolute(HTreePoint* point,
													HTreePoint* parent,
													HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}
	point->x += parent->x;
	point->y += parent->y;
	return HTREE_OK;
}

static int htree_convert_point_geometry_to_absolute(HTreePoint* point,
													HTreeRect* parent,
													HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}
	if (format == coordLeftTop) {
		point->x += parent->x;
		point->y += parent->y;
	} else {
		// format == coordLocalCenter
		point->x += parent->x + parent->width / 2.0;
		point->y += parent->y + parent->height / 2.0;
	}
	return HTREE_OK;
}

static int htree_convert_rect_geometry_to_absolute(HTreeRect* rect,
												   HTreeRect* parent,
												   HTCoordFormat format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordLeftTop) {
		rect->x += parent->x;
		rect->y += parent->y;		
	} else {
		// format == coordLocalCenter
		rect->x += parent->x + parent->width / 2.0 - rect->width / 2.0;
		rect->y += parent->y + parent->height / 2.0 - rect->height / 2.0;
	}
	return HTREE_OK;
}

static int htree_convert_rect_geometry_to_absolute(HTreeRect* rect,
												   HTreePoint* parent,
												   HTCoordFormat format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}

	rect->x += parent->x;
	rect->y += parent->y;		

	return HTREE_OK;
}

/*
static int htree_build_nodes_bounding_rect(HTreeNode* nodes,
										   HTreeRect** result)
{
	if (!nodes || !result) {
		return HTREE_BAD_PARAMETER;
	}
	for (HTreeNode* node = nodes; node; node = node->next) {
		if (node->point) {
			htree_extend_rect_point(result, node->point->x, node->point->y);
		}
		if (node->rect) {
			htree_extend_rect(result, node->rect);
		}
		if (node->children) {
			int res = htree_build_nodes_bounding_rect(node->children, result);
			if (res != HTREE_OK) {
				return res;
			}
		}
	}
	return HTREE_OK;	
}

static int htree_build_edges_bounding_rect(HTreeEdge* edges,
										   HTreeRect** result)
{
	if (!edges || !result) {
		return HTREE_BAD_PARAMETER;
	}
	while (edges) {
		if (edges->polyline) {
			htree_extend_rect_polyline(result, edges->polyline);
		}
		edges = edges->next;
	}
	return HTREE_OK;	
}*/

static int htree_construct_bounding_rect(std::vector<h2d::Point2dD>& points,
										 std::vector<h2d::FRectD>& rects,
										 std::vector<h2d::OPolyline>& polylines,
										 HTreeRect** result)
{
	h2d::FRectD br;

/*	DEBUG << "BR points: " << points.size() << " rects: " << rects.size() << " pls: " << polylines.size() << std::endl;
	if (rects.size() > 0) {
		DEBUG << "rect: " << rects.front() << std::endl;
		}*/
	
	if (points.size() == 1 && rects.size() > 0) {
		// in a case of a single point we need to add any other point to have a bounding box
		h2d::FRectD a_rect = rects.front();
		h2d::Point2dD a_point = a_rect.getPts().first;
		points.push_back(a_point);
	}
	
	if (points.size() > 0) {
		try {
			rects.push_back(h2d::getBB(points));
		} catch (const std::runtime_error& e) {
		}
	}

	if (polylines.size() > 0) {
		try {
			rects.push_back(h2d::getBB(polylines));
		} catch (const std::runtime_error& e) {
		}
	}

	if (rects.size() == 0) {
		// empty bounding rect
		if (*result) {
			HTreeRect* r = *result;
			r->x = r->y = r->width = r->height = 0.0;
		} else {
			*result = htree_new_rect();
		}
		return HTREE_OK;
	}

	try {
		br = h2d::getBB(rects);
	} catch (const std::runtime_error& e) {
	} 

	//DEBUG << "br rect: " << br << std::endl;

	if (result) {
		if (!*result) {
			*result = htree_new_rect();
		}
		homog_rect_to_htree(br, **result);
	}

	return HTREE_OK;

}

static int htree_build_nodes_bounding_rect(HTreeNode* nodes,
										   HTreeRect** result)
{
	int res;
	if (!nodes) {
		return HTREE_BAD_PARAMETER;
	}

	std::vector<h2d::Point2dD> points;
	std::vector<h2d::FRectD> rects;
	std::vector<h2d::OPolyline> polylines;
	
	res = htree_get_nodes_collections(nodes, points, rects);
	if (res != HTREE_OK) return res;

	res = htree_construct_bounding_rect(points, rects, polylines, result);
	
	return res;
}

static int htree_build_tree_bounding_rect(HTree* tree,
										  HTreeRect** result)
{
	int res;
	if (!tree) {
		return HTREE_BAD_PARAMETER;
	}

	std::vector<h2d::Point2dD> points;
	std::vector<h2d::FRectD> rects;
	std::vector<h2d::OPolyline> polylines;
	
	res = htree_get_tree_collections(tree, points, rects, polylines);
	if (res != HTREE_OK) return res;

	res = htree_construct_bounding_rect(points, rects, polylines, result);
	
	return res;
}

int htree_build_bounding_rect(HTDocument* doc, HTreeRect** result)
{
	int res;
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	std::vector<h2d::Point2dD> points;
	std::vector<h2d::FRectD> rects;
	std::vector<h2d::OPolyline> polylines;
	
	res = htree_get_collections(doc->trees, points, rects, polylines);
	if (res != HTREE_OK) return res;

	res = htree_construct_bounding_rect(points, rects, polylines, result);
	
	return res;
}

static int htree_convert_node_tree_geometry_to_absolute(HTreeNode* nodes,
														HTreeRect* parent,
														HTCoordFormat format)
{
	if (!nodes || !parent) {
		return HTREE_BAD_PARAMETER;
	}

	for (HTreeNode* node = nodes; node; node = node->next) {
		if (node->point) {
			//DEBUG << "convert point " << node->point << " with parent " << parent << " and format " << format << std::endl;
			htree_convert_point_geometry_to_absolute(node->point, parent, format);
			//DEBUG << "result " << node->point << std::endl;
		}
		if (node->rect) {
			htree_convert_rect_geometry_to_absolute(node->rect, parent, format);
		}
		if (node->children) {
			HTreeRect* next_parent;
			if (node->rect) {
				next_parent = node->rect;
			} else {
				next_parent = parent;
			}
			int res = htree_convert_node_tree_geometry_to_absolute(node->children, next_parent, format);
			if (res != HTREE_OK) {
				return res;
			}
		}
	}
	
	return HTREE_OK;
}

static int htree_convert_nodes_geometry_to_absolute(HTDocument* doc)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}	
	if (doc->node_coord_format == coordAbsolute &&
		doc->edge_coord_format == coordAbsolute &&
		doc->edge_pl_coord_format == coordAbsolute) {
		return HTREE_OK;
	}

	HTreeRect parent_rect;
	htree_init_rect(&parent_rect);
	if (doc->node_coord_format == coordLocalCenter && doc->bounding_rect && !htree_has_toplevel_rect(doc)) {
		htree_convert_rect_geometry_to_absolute(doc->bounding_rect, &parent_rect, coordLocalCenter);
		// DEBUG << "use bounding rect " << doc->bounding_rect << " as parent" << std::endl;
		htree_set_rect(&parent_rect, doc->bounding_rect);
	}
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		htree_convert_node_tree_geometry_to_absolute(tree->nodes, &parent_rect, doc->node_coord_format);
	}
	
	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_points(HTDocument* doc)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	if (doc->edge_coord_format == coordAbsolute && doc->edge_pl_coord_format == coordAbsolute) {
		return HTREE_OK;
	}
	
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				if (edge->source_point) {
					if (edge->source->rect) {
						htree_convert_point_geometry_to_absolute(edge->source_point,
																 edge->source->rect,
																 doc->edge_coord_format);
					} else {
						htree_convert_point_geometry_to_absolute(edge->source_point,
																 edge->source->point,
																 doc->edge_coord_format);
					}
				}
				if (edge->target_point) {
					if (edge->target->rect) {
						htree_convert_point_geometry_to_absolute(edge->target_point,
																 edge->target->rect,
																 doc->edge_coord_format);
					} else {
						htree_convert_point_geometry_to_absolute(edge->target_point,
																 edge->target->point,
																 doc->edge_coord_format);
					}
				}
				if (edge->polyline) {
					for (HTreePolyline* pl = edge->polyline; pl; pl = pl->next) {
						if (edge->source->rect) {
							htree_convert_point_geometry_to_absolute(&(pl->point),
																	 edge->source->rect,
																	 doc->edge_pl_coord_format);
						} else {
							htree_convert_point_geometry_to_absolute(&(pl->point),
																	 edge->source->point,
																	 doc->edge_pl_coord_format);
						}
					}
				}
			} else {
				// drop edge geometry if invalid
				if (edge->source_point) {
					htree_destroy_point(edge->source_point);
					edge->source_point = NULL;
				}
				if (edge->target_point) {
					htree_destroy_point(edge->target_point);
					edge->target_point = NULL;
				}
				if (edge->label_point) {
					htree_destroy_point(edge->label_point);
					edge->label_point = NULL;
				}
				if (edge->label_rect) {
					htree_destroy_rect(edge->label_rect);
					edge->label_rect = NULL;
				}
				if (edge->polyline) {
					htree_destroy_polyline(edge->polyline);
					edge->polyline = NULL;
				}			
			}
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_borders(HTDocument* doc)
{
	int res;
	
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	// now we'll find the border crossing points

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				if (!edge->source_point) {
					if (edge->source->rect) {
						edge->source_point = htree_rect_center_point(edge->source->rect, coordAbsolute);
					} else {
						edge->source_point = htree_copy_point(edge->source->point);
					}
				}
				if (!edge->target_point) {
					if (edge->target->rect) {
						edge->target_point = htree_rect_center_point(edge->target->rect, coordAbsolute);
					} else {
						edge->target_point = htree_copy_point(edge->target->point);
					}
				}

				h2d::Point2dD from_point = htree_point_to_homog(edge->source_point);
				h2d::Point2dD to_point = htree_point_to_homog(edge->target_point);

				//DEBUG << "convert edge from " << from_point << " to " << to_point << std::endl;
				
				h2d::SegmentD from_segment, to_segment;
				if (!edge->polyline) {
					from_segment = to_segment = h2d::SegmentD(from_point, to_point);
					//DEBUG << "converted from " << edge->source_point << " -> " << edge->target_point << std::endl;
				} else {
					h2d::Point2dD first_point = htree_point_to_homog(&(edge->polyline->point));
					HTreePolyline* pl = edge->polyline;
					while (pl->next) {
						pl = pl->next;
					}
					h2d::Point2dD last_point = htree_point_to_homog(&(pl->point));
					from_segment = h2d::SegmentD(from_point, first_point);
					to_segment = h2d::SegmentD(last_point, to_point);
					//DEBUG << "converted from " << from_point << " -> " << first_point << std::endl;
					//DEBUG << "converted from " << last_point << " -> " << to_point << std::endl;
				}

				if (edge->source->rect) {
					h2d::FRectD from_rect = htree_rect_to_homog(edge->source->rect);
					auto res = from_segment.intersects(from_rect);
					if (res() && res.get().size() >= 1) {
						homog_point_to_htree(res.get().front(), *edge->source_point);
					}
				}

				if (edge->target->rect) {
					h2d::FRectD to_rect = htree_rect_to_homog(edge->target->rect);
					auto res = to_segment.intersects(to_rect);
					if (res() && res.get().size() >= 1) {
						homog_point_to_htree(res.get().front(), *edge->target_point);
					}
				}

				//if (from_segment == to_segment) {
					//DEBUG << "converted to " << edge->source_point << " -> " << edge->target_point << std::endl;
				//} else {
					//DEBUG << "converted to " << edge->source_point << " ; " << edge->target_point << std::endl;
				//}
			}
		}
	}	
	
	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_labels(HTDocument* doc)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}
	
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				
				if (edge->label_point) {
					// evil hack for yEd format
					if (doc->node_coord_format == coordAbsolute &&
						doc->edge_coord_format == coordLocalCenter &&
						doc->edge_pl_coord_format == coordAbsolute &&
						doc->edge_format == edgeCenter) {

						htree_convert_point_geometry_to_absolute(edge->label_point,
																 edge->source_point,
																 doc->edge_coord_format);						
					} else if (edge->source->rect) {
						htree_convert_point_geometry_to_absolute(edge->label_point,
																 edge->source->rect,
																 doc->edge_coord_format);
					} else {
						htree_convert_point_geometry_to_absolute(edge->label_point,
																 edge->source->point,
																 doc->edge_coord_format);
					}
				}
				if (edge->label_rect) {
					if (edge->source->rect) {
						htree_convert_rect_geometry_to_absolute(edge->label_rect,
																edge->source->rect,
																doc->edge_coord_format);
					} else {
						htree_convert_rect_geometry_to_absolute(edge->label_rect,
																edge->source->point,
																doc->edge_coord_format);
					}
				}
			}
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute(HTDocument* doc)
{
	int res;
	
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	if (doc->edge_coord_format != coordAbsolute || doc->edge_pl_coord_format != coordAbsolute) {
		res = htree_convert_edges_geometry_to_absolute_points(doc);
		if (res != HTREE_OK) {
			return res;
		}
	}
	
	//DEBUG << "convert edge geometry " << doc->edge_format << std::endl;

	if (doc->edge_format != edgeBorder) {
		res = htree_convert_edges_geometry_to_absolute_borders(doc);
		if (res != HTREE_OK) {
			return res;
		}
	}

	return htree_convert_edges_geometry_to_absolute_labels(doc);
}

static int htree_convert_document_geometry_to_absolute(HTDocument* doc)
{
	int res;
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}
	res = htree_convert_nodes_geometry_to_absolute(doc);
	if (res != HTREE_OK) {
		return res;
	}
	
/*	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && edge->source->rect) {
				if (edge->abs_source_rect) {
					free(edge->abs_source_rect);
				}
				edge->abs_source_rect = htree_copy_rect(edge->source->rect);
			}
			if (edge->target && edge->target->rect) {
				if (edge->abs_target_rect) {
					free(edge->abs_target_rect);
				}
				edge->abs_target_rect = htree_copy_rect(edge->target->rect);
			}
		}
		}*/
	
	res = htree_convert_edges_geometry_to_absolute(doc);
	if (res != HTREE_OK) {
		return res;
	}
	doc->node_coord_format = coordAbsolute;	
	doc->edge_coord_format = coordAbsolute;	
	doc->edge_pl_coord_format = coordAbsolute;	
	doc->edge_format = edgeBorder;
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * Geometry transformations implementation: absolute to format
 * ----------------------------------------------------------------------------- */

static int htree_convert_point_geometry_to_format(HTreePoint* point,
												  HTreePoint* parent,
												  HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}

	point->x -= parent->x;
	point->y -= parent->y;

	if (point->x != 0.0 && abs(point->x) < 0.000001) point->x = 0.0;
	if (point->y != 0.0 && abs(point->y) < 0.000001) point->y = 0.0;
	
	return HTREE_OK;
}

static int htree_convert_point_geometry_to_format(HTreePoint* point,
												  HTreeRect* parent,
												  HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone || format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}
	if (format == coordLeftTop) {
		point->x -= parent->x;
		point->y -= parent->y;
	} else {
		// format == coordLocalCenter
		point->x -= parent->x + parent->width / 2.0;
		point->y -= parent->y + parent->height / 2.0;
	}

	if (point->x != 0.0 && abs(point->x) < 0.000001) point->x = 0.0;
	if (point->y != 0.0 && abs(point->y) < 0.000001) point->y = 0.0;
	
	return HTREE_OK;
}

static int htree_convert_rect_geometry_to_format(HTreeRect* rect,
												 HTreePoint* parent,
												 HTCoordFormat new_format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordNone || new_format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}

	rect->x -= parent->x;
	rect->y -= parent->y;		

	return HTREE_OK;
}

static int htree_convert_rect_geometry_to_format(HTreeRect* rect,
												 HTreeRect* parent,
												 HTCoordFormat new_format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordNone || new_format == coordAbsolute) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordLeftTop) {
		rect->x -= parent->x;
		rect->y -= parent->y;		
	} else {
		// new_format == coordLocalCenter
		rect->x -= parent->x + parent->width / 2.0 - rect->width / 2.0;
		rect->y -= parent->y + parent->height / 2.0 - rect->height / 2.0;
	}
	return HTREE_OK;
}

static int htree_convert_node_tree_geometry_to_format(HTreeNode* nodes,
													  HTreeRect* parent,
													  HTCoordFormat format)
{
	if (!nodes || !parent) {
		return HTREE_BAD_PARAMETER;
	}

	for (HTreeNode* node = nodes; node; node = node->next) {
		if (node->children) {
			HTreeRect* next_parent;
			if (node->rect) {
				next_parent = node->rect;
			} else {
				next_parent = parent;
			}
			int res = htree_convert_node_tree_geometry_to_format(node->children, next_parent, format);
			if (res != HTREE_OK) {
				return res;
			}
		}
		if (node->point) {
			htree_convert_point_geometry_to_format(node->point, parent, format);
		}
		if (node->rect) {
			htree_convert_rect_geometry_to_format(node->rect, parent, format);
		}
	}
	
	return HTREE_OK;
}

static int htree_convert_nodes_geometry_to_format(HTDocument* doc,
												  HTCoordFormat new_format)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}	
	if (doc->node_coord_format == new_format) {
		return HTREE_OK;
	}

	HTreeRect parent_rect;
	htree_init_rect(&parent_rect);
	if (new_format == coordLocalCenter && doc->bounding_rect && !htree_has_toplevel_rect(doc)) {
		htree_set_rect(&parent_rect, doc->bounding_rect);
	}
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		htree_convert_node_tree_geometry_to_format(tree->nodes,
												   &parent_rect,
												   new_format);
	}
	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_format_points(HTDocument* doc,
														 HTCoordFormat edge_format,
														 HTCoordFormat edge_pl_format)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	//DEBUG << "convert edge point from format " << doc->edge_coord_format << " to format " << edge_format << std::endl;
	
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				if (edge->source_point) {
					if (edge->source->rect) {
						htree_convert_point_geometry_to_format(edge->source_point,
															   edge->source->rect,
															   edge_format);
					} else {
						htree_convert_point_geometry_to_format(edge->source_point,
															   edge->source->point,
															   edge_format);
					}
				}
				if (edge->target_point) {
					if (edge->target->rect) {
						htree_convert_point_geometry_to_format(edge->target_point,
															   edge->target->rect,
															   edge_format);
					} else {
						htree_convert_point_geometry_to_format(edge->target_point,
															   edge->target->point	,
															   edge_format);
					}
				}
				if (edge->polyline) {
					for (HTreePolyline* pl = edge->polyline; pl; pl = pl->next) {
						if (edge->source->rect) {
							htree_convert_point_geometry_to_format(&(pl->point),
																   edge->source->rect,
																   edge_pl_format);
						} else {
							htree_convert_point_geometry_to_format(&(pl->point),
																   edge->source->point,
																   edge_pl_format);
						}
					}
				}
			}
		}
	}

	return HTREE_OK;
}

/*static int htree_convert_edges_geometry_to_format_center(HTDocument* doc)
{
	int res;
	
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	// now we'll find the centers

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source_point && edge->source && (edge->source->rect || edge->source->point) &&
				edge->source_point && edge->target && (edge->target->rect || edge->target->point)) {

				h2d::Point2dD source_center;
				h2d::Point2dD target_center;

				if (edge->source->point) {
					source_center = htree_point_to_homog(edge->source->point);
				} else {
					h2d::FRectD rect = htree_rect_to_homog(edge->source->rect);
					source_center = rect.getCenter();
				}
				if (edge->target->point) {
					target_center = htree_point_to_homog(edge->target->point);
				} else {
					h2d::FRectD rect = htree_rect_to_homog(edge->target->rect);
					target_center = rect.getCenter();
				}
				
				if (!edge->polyline) {
					h2d::Line2dD line(htree_point_to_homog(edge->source_point),
									  htree_point_to_homog(edge->target_point));
					if (side(source_center, line) == 0) {
						// source center is on line
						edge->source_point->x = 0.0;
						edge->source_point->y = 0.0;
					} else {
						h2d::SegmentD segment = line.getOrthogSegment(source_point);
						auto pair = segment.getPts();
						if (pair.first == source_point) {
							source_point = pair.second;
						} else {
							source_point = pair.first;
						}
						homog_point_to_htree(source_point, *edge->source_point);
						if (edge->source->rect) {
							htree_convert_point_geometry_to_format(edge->source_point,
																   edge->source->rect,
																   coordLocalCenter);
						} else {
							htree_convert_point_geometry_to_format(edge->source_point,
																   edge->source->point,
																   coordLocalCenter);
																   }

						segment = line.getOrthogSegment(target_point);
						pair = segment.getPts();
						if (pair.first == target_point) {
							target_point = pair.second;
						} else {
							target_point = pair.first;
						}
						homog_point_to_htree(target_point, *edge->target_point);
						if (edge->target->rect) {
							htree_convert_point_geometry_to_format(edge->target_point,
																   edge->target->rect,
																   coordLocalCenter);
						} else {
							htree_convert_point_geometry_to_format(edge->target_point,
																   edge->target->point,
																   coordLocalCenter);
																   }
					}
				} else {
					h2d::Point2dD first_point = htree_point_to_homog(&(edge->polyline->point));
					HTreePolyline* pl = edge->polyline;
					while (pl->next) {
						pl = pl->next;
					}
					h2d::Point2dD last_point = htree_point_to_homog(&(pl->point));

					h2d::Line2dD line1(source_point, first_point);
					h2d::Line2dD line2(target_point, last_point);

					if (!line1.isParallel(line2)) {
						if (edge->source == edge->target) {
							h2d::Point2dD crossing = line1 * line2;
							h2d::FRectD the_rect = htree_rect_to_homog(); 
						}
					}
				}
			}
		}
	}	
	
	return HTREE_OK;
	}*/

static int htree_convert_edges_geometry_to_format_labels(HTDocument* doc,
														 HTCoordFormat new_format,
														 HTCoordFormat new_pl_format,
														 HTEdgeFormat new_edge_format)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}
	
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				
				if (edge->label_point) {
					// evil hack for yEd format
					if (new_format == coordLocalCenter &&
						new_pl_format == coordAbsolute &&
						new_edge_format == edgeCenter) {

						htree_convert_point_geometry_to_format(edge->label_point,
															   edge->source_point,
															   new_format);						
					} else if (edge->source->rect) {
						htree_convert_point_geometry_to_format(edge->label_point,
															   edge->source->rect,
															   new_format);
					} else {
						htree_convert_point_geometry_to_format(edge->label_point,
															   edge->source->point,
															   new_format);
					}
				}
				if (edge->label_rect) {
					if (edge->source->rect) {
						htree_convert_rect_geometry_to_format(edge->label_rect,
															  edge->source->rect,
															  new_format);
					} else {
						htree_convert_rect_geometry_to_format(edge->label_rect,
															  edge->source->point,
															  new_format);
					}
				}
			}
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_format(HTDocument* doc,
												  HTCoordFormat new_format,
												  HTCoordFormat new_pl_format,
												  HTEdgeFormat new_edge_format)
{
	int res;
	
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}
	if (doc->edge_coord_format == new_format &&
		doc->edge_pl_coord_format == new_pl_format &&
		doc->edge_format == new_edge_format) {
		return HTREE_OK;
	}

	res = htree_convert_edges_geometry_to_format_labels(doc,
														new_format,
														new_pl_format,
														new_edge_format);
	if (res != HTREE_OK) {
		return res;
	}
	
/*	DEBUG << "convert edge geometry " << doc->edge_format << " to " << new_edge_format << std::endl;

	if (new_edge_format != edgeBorder) {
		res = htree_convert_edges_geometry_to_format_center(doc);
		if (res != HTREE_OK) {
			return res;
		}
		}*/

	return htree_convert_edges_geometry_to_format_points(doc, new_format, new_pl_format);
}

static int htree_convert_document_geometry_to_format(HTDocument* doc,
													 HTCoordFormat new_node_coord_format,
													 HTCoordFormat new_edge_coord_format,
													 HTCoordFormat new_edge_pl_coord_format,
													 HTEdgeFormat new_edge_format)
{
	int res;
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}
	if (!doc->bounding_rect) {
		return HTREE_BAD_PARAMETER;
	}
	res = htree_convert_edges_geometry_to_format(doc, new_edge_coord_format,
												 new_edge_pl_coord_format, new_edge_format);
	if (res != HTREE_OK) {
		return res;
	}
	res = htree_convert_nodes_geometry_to_format(doc, new_node_coord_format);
	if (res != HTREE_OK) {
		return res;
	}
	HTreeRect parent;
	htree_init_rect(&parent);
	htree_convert_rect_geometry_to_format(doc->bounding_rect, &parent, new_node_coord_format);
	doc->node_coord_format = new_node_coord_format;	
	doc->edge_coord_format = new_edge_coord_format;	
	doc->edge_pl_coord_format = new_edge_pl_coord_format;	
	doc->edge_format = new_edge_format;
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * Geometry transformations interface
 * ----------------------------------------------------------------------------- */

static int htree_reconstruct_nodes_geometry(HTreeNode* parent, int reconstruct_parent)
{
	double parent_x, parent_y;
	
	if (!parent) {
		return HTREE_BAD_PARAMETER;
	}

	if (parent->rect) {
		parent_x = parent->rect->x;
		parent_y = parent->rect->y;
	} else {
		parent_x = parent_y = 0.0; 
	}

	//DEBUG << "Reconstruct node geometry: " << parent->id << std::endl;
	
	for (HTreeNode* node = parent->children; node; node = node->next) {

		//DEBUG << "Children: " << node->id << " type: " << node->type << std::endl;
	
		if (node->type == htPoint) {
			if (!node->point) {
				node->point = htree_new_point();
				node->point->x = parent_x + PADDING;
				node->point->y = parent_y + PADDING;
			}
		} else {
			if (!node->rect) {
				node->rect = htree_new_rect();
				node->rect->x = parent_x + PADDING;
				node->rect->y = parent_y + PADDING;
				node->rect->width = NODE_WIDTH;
				node->rect->height = NODE_HEIGHT;
			}
		}
		if (node->children) {
			htree_reconstruct_nodes_geometry(node, 1);
		}
	}
	
	if (reconstruct_parent) {
		bool empty_rect = !parent->rect; 
		htree_build_nodes_bounding_rect(parent, &(parent->rect));
		if (empty_rect && parent->rect) {
			parent->rect->x -= PADDING;
			parent->rect->y -= PADDING;
			parent->rect->width += 2 * PADDING;
			parent->rect->height += 2 * PADDING;			
		}
	}
		
	return HTREE_OK;	
}

static int htree_reconstruct_edges_geometry(HTreeEdge* edges)
{
	if (!edges) {
		return HTREE_BAD_PARAMETER;
	}
	return HTREE_OK;
}

int htree_reconstruct_document_geometry(HTDocument* doc, int reconstruct_sm)
{
	int res;
	HTCoordFormat node_coord_format, edge_coord_format, edge_pl_coord_format;
	HTEdgeFormat edge_format;

	if (!doc || !doc->trees) {
		return HTREE_BAD_PARAMETER;
	}

	//DEBUG << "Reconstruct document geometry" << std::endl;
	//htree_print_document(doc);
	
	node_coord_format = doc->node_coord_format;
	edge_coord_format = doc->edge_coord_format;
	edge_pl_coord_format = doc->edge_pl_coord_format;
	edge_format = doc->edge_format;

	htree_convert_document_geometry_to_absolute(doc);

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		htree_reconstruct_nodes_geometry(tree->nodes, reconstruct_sm);
		htree_reconstruct_edges_geometry(tree->edges);
	}

	if (doc->bounding_rect) {
		htree_destroy_rect(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}

	htree_build_bounding_rect(doc, &(doc->bounding_rect));

	htree_convert_document_geometry_to_format(doc, node_coord_format,
											  edge_coord_format,
											  edge_pl_coord_format,
											  edge_format);

	//htree_print_document(doc);
	
	return HTREE_OK;
}

int htree_convert_document_geometry(HTDocument* doc,
									HTCoordFormat new_node_coord_format,
									HTCoordFormat new_edge_coord_format,
									HTCoordFormat new_edge_pl_coord_format,
									HTEdgeFormat new_edge_format)
{
	int res;
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

/*	DEBUG << "Start format: node coord " << doc->node_coord_format <<
		" edge coord " << doc->edge_coord_format <<
		" edge coord " << doc->edge_pl_coord_format <<
		" edge " << doc->edge_format << std::endl;

		htree_print_document(doc);*/
	
	htree_convert_document_geometry_to_absolute(doc);

	if (doc->bounding_rect) {
		htree_destroy_rect(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}
	htree_build_bounding_rect(doc, &(doc->bounding_rect));
	
/*	DEBUG << "Absolute format: node coord " << doc->node_coord_format <<
		" edge coord " << doc->edge_coord_format <<
		" edge coord " << doc->edge_pl_coord_format <<
		" edge " << doc->edge_format << std::endl;

		htree_print_document(doc);*/
	
	htree_convert_document_geometry_to_format(doc,
											  new_node_coord_format,
											  new_edge_coord_format,
											  new_edge_pl_coord_format,
											  new_edge_format);
	
/*	DEBUG << "Final format: node coord " << doc->node_coord_format <<
		" edge coord " << doc->edge_coord_format <<
		" edge coord " << doc->edge_pl_coord_format <<
		" edge " << doc->edge_format << std::endl;
	
		htree_print_document(doc);*/
	
	return HTREE_OK;
}
