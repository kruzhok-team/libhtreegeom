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

#include <cmath>
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

/* -----------------------------------------------------------------------------
 * Geometry conversions: lib to homog2d and back
 * ----------------------------------------------------------------------------- */

static h2d::Point2dD htree_point_to_homog(const HTreePoint* point)
{
	if (point) {
		return h2d::Point2dD(point->x, point->y);
	}
	return h2d::Point2dD();
}

static int homog_point_to_htree(const h2d::Point2dD& hgpoint, HTreePoint& point)
{
	point.x = hgpoint.getX();
//	if (point.x != 0.0 && abs(point.x) < HTREE_COORD_EPS) point.x = 0.0;
	point.y = hgpoint.getY();
//	if (point.y != 0.0 && abs(point.y) < HTREE_COORD_EPS) point.y = 0.0;
	return HTREE_OK;
}

static h2d::FRectD htree_rect_to_homog(const HTreeRect* rect)
{
	if (rect) {
		return h2d::FRectD(rect->x, rect->y,
						   rect->x + rect->width, rect->y + rect->height);
	}
	return h2d::FRectD();
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

static int htree_get_edges_collections(const HTreeEdge* edges,
									   std::vector<h2d::Point2dD>& points,
									   std::vector<h2d::FRectD>& rects,
									   std::vector<h2d::OPolyline>& polylines)
{
	if (edges) {
		for (const HTreeEdge* edge = edges; edge; edge = edge->next) {
			if (!edge->source || !edge->target) continue;
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
					target = *(edge->target->point);
				} else {
					continue;
				}
				
				/* push the plain points: an open polyline cannot carry
				   coinciding loop ends and single-vertex chains */
				points.push_back(htree_point_to_homog(&source));
				for (const HTreePolyline* pl = edge->polyline; pl; pl = pl->next) {
					points.push_back(htree_point_to_homog(&(pl->point)));
				}
				points.push_back(htree_point_to_homog(&target));
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

static int htree_get_tree_collections(const HTree* tree,
									  std::vector<h2d::Point2dD>& points,
									  std::vector<h2d::FRectD>& rects,
									  std::vector<h2d::OPolyline>& polylines)
{
	if (!tree) {
		return HTREE_BAD_PARAMETER;
	}

	/* the explicit SM border: the tree bounding rect equals the border rect */
	if (tree->nodes && !tree->nodes->next &&
		tree->nodes->type == htTree && tree->nodes->rect) {
		rects.push_back(htree_rect_to_homog(tree->nodes->rect));
		return HTREE_OK;
	}

	if (tree->nodes) {
		int res = htree_get_nodes_collections(tree->nodes, points, rects);
		if (res != HTREE_OK) {
			return res;
		}
	}

	return htree_get_edges_collections(tree->edges, points, rects, polylines);
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
													const HTreePoint* parent,
													HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}
	point->x += parent->x;
	point->y += parent->y;
	return HTREE_OK;
}

static int htree_convert_point_geometry_to_absolute(HTreePoint* point,
													const HTreeRect* parent,
													HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
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
												   const HTreeRect* parent,
												   HTCoordFormat format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
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
												   const HTreePoint* parent,
												   HTCoordFormat format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
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
	/* a manual min/max fold: h2d::getBB throws on degenerate
	   (single-point or collinear) collections */
	bool found = false;
	double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;

	if (!result) {
		return HTREE_BAD_PARAMETER;
	}

	auto add_point = [&](double x, double y) {
		if (!found) {
			min_x = max_x = x;
			min_y = max_y = y;
			found = true;
		} else {
			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
		}
	};

	for (const h2d::Point2dD& p : points) {
		add_point(p.getX(), p.getY());
	}
	for (const h2d::FRectD& r : rects) {
		auto pts = r.getPts();
		add_point(pts.first.getX(), pts.first.getY());
		add_point(pts.second.getX(), pts.second.getY());
	}
	for (const h2d::OPolyline& pl : polylines) {
		for (const h2d::Point2dD& p : pl.getPts()) {
			add_point(p.getX(), p.getY());
		}
	}

	if (!*result) {
		*result = htree_new_rect();
	}
	if (found) {
		(*result)->x = min_x;
		(*result)->y = min_y;
		(*result)->width = max_x - min_x;
		(*result)->height = max_y - min_y;
	} else {
		// empty bounding rect
		htree_init_rect(*result);
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

int htree_check_geometry(const HTDocument* doc)
{
	int res = HTREE_OK;

	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	HTDocument* copy = htree_copy_document(doc);
	if (!copy) {
		return HTREE_BAD_PARAMETER;
	}
	res = htree_convert_document_geometry(copy, coordAbsolute, coordAbsolute,
										  coordAbsolute, edgeBorder);
	if (res != HTREE_OK) {
		htree_destroy_document(copy);
		return res;
	}

	for (HTree* tree = copy->trees; tree; tree = tree->next) {
		/* only the trees with the explicit SM border are constrained */
		if (!(tree->nodes && !tree->nodes->next &&
			  tree->nodes->type == htTree && tree->nodes->rect)) {
			continue;
		}
		std::vector<h2d::Point2dD> points;
		std::vector<h2d::FRectD> rects;
		std::vector<h2d::OPolyline> polylines;
		if (tree->nodes->children) {
			res = htree_get_nodes_collections(tree->nodes->children, points, rects);
			if (res != HTREE_OK) {
				break;
			}
		}
		res = htree_get_edges_collections(tree->edges, points, rects, polylines);
		if (res != HTREE_OK) {
			break;
		}

		HTreeRect* content = NULL;
		res = htree_construct_bounding_rect(points, rects, polylines, &content);
		if (res != HTREE_OK) {
			break;
		}
		if (content) {
			if (!htree_empty_rect(content)) {
				const HTreeRect* b = tree->nodes->rect;
				if (content->x < b->x - HTREE_RECT_EPS ||
					content->y < b->y - HTREE_RECT_EPS ||
					content->x + content->width > b->x + b->width + HTREE_RECT_EPS ||
					content->y + content->height > b->y + b->height + HTREE_RECT_EPS) {
					res = HTREE_GEOMETRY_INVALID;
				}
			}
			htree_destroy_rect(content);
		}
		if (res != HTREE_OK) {
			break;
		}
	}

	htree_destroy_document(copy);
	return res;
}

static int htree_convert_node_tree_geometry_to_absolute(HTreeNode* nodes,
														const HTreeRect* parent,
														HTCoordFormat format)
{
	if (!nodes || !parent) {
		return HTREE_BAD_PARAMETER;
	}

	for (HTreeNode* node = nodes; node; node = node->next) {
		if (node->point) {
			int res = htree_convert_point_geometry_to_absolute(node->point, parent, format);
			if (res != HTREE_OK) {
				return res;
			}
		}
		if (node->rect) {
			int res = htree_convert_rect_geometry_to_absolute(node->rect, parent, format);
			if (res != HTREE_OK) {
				return res;
			}
		}
		if (node->children) {
			const HTreeRect* next_parent;
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
		int res = htree_convert_rect_geometry_to_absolute(doc->bounding_rect, &parent_rect, coordLocalCenter);
		if (res != HTREE_OK) {
			return res;
		}
		// DEBUG << "use bounding rect " << doc->bounding_rect << " as parent" << std::endl;
		htree_set_rect(&parent_rect, doc->bounding_rect);
	}
	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		if (!tree->nodes) {
			continue;
		}
		int res = htree_convert_node_tree_geometry_to_absolute(tree->nodes, &parent_rect, doc->node_coord_format);
		if (res != HTREE_OK) {
			return res;
		}
	}
	
	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_points(HTDocument* doc)
{
	int res;

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
						res = htree_convert_point_geometry_to_absolute(edge->source_point,
																	   edge->source->rect,
																	   doc->edge_coord_format);
					} else {
						res = htree_convert_point_geometry_to_absolute(edge->source_point,
																	   edge->source->point,
																	   doc->edge_coord_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->target_point) {
					if (edge->target->rect) {
						res = htree_convert_point_geometry_to_absolute(edge->target_point,
																	   edge->target->rect,
																	   doc->edge_coord_format);
					} else {
						res = htree_convert_point_geometry_to_absolute(edge->target_point,
																	   edge->target->point,
																	   doc->edge_coord_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->polyline) {
					for (HTreePolyline* pl = edge->polyline; pl; pl = pl->next) {
						if (edge->source->rect) {
							res = htree_convert_point_geometry_to_absolute(&(pl->point),
																		   edge->source->rect,
																		   doc->edge_pl_coord_format);
						} else {
							res = htree_convert_point_geometry_to_absolute(&(pl->point),
																		   edge->source->point,
																		   doc->edge_pl_coord_format);
						}
						if (res != HTREE_OK) {
							return res;
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

/* Fill the missing edge ends with the node centers and project both
   ends onto the node borders */
static int htree_project_edge_to_borders(HTreeEdge* edge)
{
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

	/* a segment cannot carry two identical points:
	   the degenerate ends keep their positions */
	bool project_source, project_target;
	h2d::Point2dD from_toward, to_toward;
	if (!edge->polyline) {
		from_toward = to_point;
		to_toward = from_point;
		project_source = project_target = (from_point != to_point);
	} else {
		h2d::Point2dD first_point = htree_point_to_homog(&(edge->polyline->point));
		HTreePolyline* pl = edge->polyline;
		while (pl->next) {
			pl = pl->next;
		}
		h2d::Point2dD last_point = htree_point_to_homog(&(pl->point));
		from_toward = first_point;
		to_toward = last_point;
		project_source = (from_point != first_point);
		project_target = (last_point != to_point);
	}

	if (project_source && edge->source->rect) {
		h2d::FRectD from_rect = htree_rect_to_homog(edge->source->rect);
		auto res = h2d::SegmentD(from_point, from_toward).intersects(from_rect);
		if (res() && res.get().size() >= 1) {
			homog_point_to_htree(res.get().front(), *edge->source_point);
		}
	}

	if (project_target && edge->target->rect) {
		h2d::FRectD to_rect = htree_rect_to_homog(edge->target->rect);
		auto res = h2d::SegmentD(to_toward, to_point).intersects(to_rect);
		if (res() && res.get().size() >= 1) {
			homog_point_to_htree(res.get().front(), *edge->target_point);
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_borders(HTDocument* doc)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	// now we'll find the border crossing points

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->source && (edge->source->rect || edge->source->point) &&
				edge->target && (edge->target->rect || edge->target->point)) {
				int res = htree_project_edge_to_borders(edge);
				if (res != HTREE_OK) {
					return res;
				}
			}
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_absolute_labels(HTDocument* doc)
{
	int res;

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

						res = htree_convert_point_geometry_to_absolute(edge->label_point,
																	   edge->source_point,
																	   doc->edge_coord_format);
					} else if (edge->source->rect) {
						res = htree_convert_point_geometry_to_absolute(edge->label_point,
																	   edge->source->rect,
																	   doc->edge_coord_format);
					} else {
						res = htree_convert_point_geometry_to_absolute(edge->label_point,
																	   edge->source->point,
																	   doc->edge_coord_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->label_rect) {
					if (edge->source->rect) {
						res = htree_convert_rect_geometry_to_absolute(edge->label_rect,
																	  edge->source->rect,
																	  doc->edge_coord_format);
					} else {
						res = htree_convert_rect_geometry_to_absolute(edge->label_rect,
																	  edge->source->point,
																	  doc->edge_coord_format);
					}
					if (res != HTREE_OK) {
						return res;
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

	/* the labels are bound to the stored source points,
	   so they convert before the border projection */
	res = htree_convert_edges_geometry_to_absolute_labels(doc);
	if (res != HTREE_OK) {
		return res;
	}

	if (doc->edge_format != edgeBorder) {
		res = htree_convert_edges_geometry_to_absolute_borders(doc);
		if (res != HTREE_OK) {
			return res;
		}
	}

	return HTREE_OK;
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
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
	}

	point->x -= parent->x;
	point->y -= parent->y;

	if (point->x != 0.0 && std::fabs(point->x) < HTREE_COORD_EPS) point->x = 0.0;
	if (point->y != 0.0 && std::fabs(point->y) < HTREE_COORD_EPS) point->y = 0.0;
	
	return HTREE_OK;
}

static int htree_convert_point_geometry_to_format(HTreePoint* point,
												  HTreeRect* parent,
												  HTCoordFormat format)
{
	if (!point) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
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

	if (point->x != 0.0 && std::fabs(point->x) < HTREE_COORD_EPS) point->x = 0.0;
	if (point->y != 0.0 && std::fabs(point->y) < HTREE_COORD_EPS) point->y = 0.0;
	
	return HTREE_OK;
}

static int htree_convert_rect_geometry_to_format(HTreeRect* rect,
												 HTreePoint* parent,
												 HTCoordFormat new_format)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
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
	if (new_format == coordNone) {
		return HTREE_BAD_PARAMETER;
	}
	if (new_format == coordAbsolute) {
		/* already in the target format */
		return HTREE_OK;
	}
	if (parent == NULL) { // no parent
		return HTREE_OK;
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
			int res = htree_convert_point_geometry_to_format(node->point, parent, format);
			if (res != HTREE_OK) {
				return res;
			}
		}
		if (node->rect) {
			int res = htree_convert_rect_geometry_to_format(node->rect, parent, format);
			if (res != HTREE_OK) {
				return res;
			}
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
		if (!tree->nodes) {
			continue;
		}
		int res = htree_convert_node_tree_geometry_to_format(tree->nodes,
															 &parent_rect,
															 new_format);
		if (res != HTREE_OK) {
			return res;
		}
	}
	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_format_points(HTDocument* doc,
														 HTCoordFormat edge_format,
														 HTCoordFormat edge_pl_format)
{
	int res;

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
						res = htree_convert_point_geometry_to_format(edge->source_point,
																	 edge->source->rect,
																	 edge_format);
					} else {
						res = htree_convert_point_geometry_to_format(edge->source_point,
																	 edge->source->point,
																	 edge_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->target_point) {
					if (edge->target->rect) {
						res = htree_convert_point_geometry_to_format(edge->target_point,
																	 edge->target->rect,
																	 edge_format);
					} else {
						res = htree_convert_point_geometry_to_format(edge->target_point,
																	 edge->target->point,
																	 edge_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->polyline) {
					for (HTreePolyline* pl = edge->polyline; pl; pl = pl->next) {
						if (edge->source->rect) {
							res = htree_convert_point_geometry_to_format(&(pl->point),
																		 edge->source->rect,
																		 edge_pl_format);
						} else {
							res = htree_convert_point_geometry_to_format(&(pl->point),
																		 edge->source->point,
																		 edge_pl_format);
						}
						if (res != HTREE_OK) {
							return res;
						}
					}
				}
			}
		}
	}

	return HTREE_OK;
}

/* The center attachment of an edge end: the perpendicular foot of the
   state center on the edge's outgoing line; the foot equals the center
   (and later reframes to (0,0)) when the edge aims at the center */
static void htree_project_center_point(HTreePoint* point, const HTreePoint* toward,
									   double center_x, double center_y)
{
	double dx = toward->x - point->x;
	double dy = toward->y - point->y;
	double len2 = dx * dx + dy * dy;
	if (len2 < HTREE_LENGTH_EPS) {
		/* coinciding ends: keep the border point */
		return;
	}
	double t = ((center_x - point->x) * dx + (center_y - point->y) * dy) / len2;
	point->x += t * dx;
	point->y += t * dy;
}

static int htree_convert_edges_geometry_to_center(HTDocument* doc)
{
	if (!doc) {
		return HTREE_BAD_PARAMETER;
	}

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		for (HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (!(edge->source && (edge->source->rect || edge->source->point) &&
				  edge->target && (edge->target->rect || edge->target->point))) {
				continue;
			}
			double scx, scy, tcx, tcy;
			if (edge->source->rect) {
				scx = edge->source->rect->x + edge->source->rect->width / 2.0;
				scy = edge->source->rect->y + edge->source->rect->height / 2.0;
			} else {
				scx = edge->source->point->x;
				scy = edge->source->point->y;
			}
			if (edge->target->rect) {
				tcx = edge->target->rect->x + edge->target->rect->width / 2.0;
				tcy = edge->target->rect->y + edge->target->rect->height / 2.0;
			} else {
				tcx = edge->target->point->x;
				tcy = edge->target->point->y;
			}
			if (edge->source_point && edge->target_point) {
				HTreePoint* toward_source;
				HTreePoint* toward_target;
				if (edge->polyline) {
					HTreePolyline* pl = edge->polyline;
					while (pl->next) pl = pl->next;
					toward_source = &(edge->polyline->point);
					toward_target = &(pl->point);
				} else {
					toward_source = edge->target_point;
					toward_target = edge->source_point;
				}
				HTreePoint old_source = *(edge->source_point);
				htree_project_center_point(edge->source_point, toward_source, scx, scy);
				htree_project_center_point(edge->target_point,
										   edge->polyline ? toward_target : &old_source,
										   tcx, tcy);
			}
		}
	}

	return HTREE_OK;
}

static int htree_convert_edges_geometry_to_format_labels(HTDocument* doc,
														 HTCoordFormat new_format,
														 HTCoordFormat new_pl_format,
														 HTEdgeFormat new_edge_format)
{
	int res;

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

						res = htree_convert_point_geometry_to_format(edge->label_point,
																	 edge->source_point,
																	 new_format);
					} else if (edge->source->rect) {
						res = htree_convert_point_geometry_to_format(edge->label_point,
																	 edge->source->rect,
																	 new_format);
					} else {
						res = htree_convert_point_geometry_to_format(edge->label_point,
																	 edge->source->point,
																	 new_format);
					}
					if (res != HTREE_OK) {
						return res;
					}
				}
				if (edge->label_rect) {
					if (edge->source->rect) {
						res = htree_convert_rect_geometry_to_format(edge->label_rect,
																	edge->source->rect,
																	new_format);
					} else {
						res = htree_convert_rect_geometry_to_format(edge->label_rect,
																	edge->source->point,
																	new_format);
					}
					if (res != HTREE_OK) {
						return res;
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

	if (new_edge_format == edgeCenter) {
		res = htree_convert_edges_geometry_to_center(doc);
		if (res != HTREE_OK) {
			return res;
		}
	}

	/* the labels are bound to the projected source points */
	res = htree_convert_edges_geometry_to_format_labels(doc,
														new_format,
														new_pl_format,
														new_edge_format);
	if (res != HTREE_OK) {
		return res;
	}

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
	res = htree_convert_rect_geometry_to_format(doc->bounding_rect, &parent, new_node_coord_format);
	if (res != HTREE_OK) {
		return res;
	}
	doc->node_coord_format = new_node_coord_format;	
	doc->edge_coord_format = new_edge_coord_format;	
	doc->edge_pl_coord_format = new_edge_pl_coord_format;	
	doc->edge_format = new_edge_format;
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * Geometry transformations interface
 * ----------------------------------------------------------------------------- */

/* The bounding rect of the node list (the children of a parent) */
static int htree_children_bounding_rect(HTreeNode* children, HTreeRect** result)
{
	std::vector<h2d::Point2dD> points;
	std::vector<h2d::FRectD> rects;
	std::vector<h2d::OPolyline> polylines;

	int res = htree_get_nodes_collections(children, points, rects);
	if (res != HTREE_OK) {
		return res;
	}
	return htree_construct_bounding_rect(points, rects, polylines, result);
}

/* The preserving reconstruction: the existing geometry is never modified,
   the missing children are shelf-placed inside the parent, the parent rect
   is created when missing or grown (grow-only) when the content overflows */
static int htree_reconstruct_nodes_geometry(HTreeNode* parent, int reconstruct_parent)
{
	double origin_x, origin_y, shelf_limit;
	double shelf_x, shelf_y, row_h;

	if (!parent) {
		return HTREE_BAD_PARAMETER;
	}

	if (parent->rect) {
		origin_x = parent->rect->x;
		origin_y = parent->rect->y;
		shelf_limit = parent->rect->width;
	} else {
		origin_x = origin_y = 0.0;
		shelf_limit = HTREE_SHELF_COLUMNS * (NODE_WIDTH + PADDING) + PADDING;
	}
	shelf_x = origin_x + PADDING;
	shelf_y = origin_y + PADDING;
	row_h = 0.0;

	for (HTreeNode* node = parent->children; node; node = node->next) {
		double w = 0.0, h = 0.0;
		int place = 0;
		if (node->type == htPoint) {
			if (!node->point) {
				w = h = PADDING;
				place = 1;
			}
		} else if (!node->rect) {
			w = NODE_WIDTH;
			h = NODE_HEIGHT;
			place = 1;
		}
		if (place) {
			if (shelf_x > origin_x + PADDING &&
				shelf_x + w + PADDING > origin_x + shelf_limit) {
				/* wrap the shelf row */
				shelf_x = origin_x + PADDING;
				shelf_y += row_h + PADDING;
				row_h = 0.0;
			}
			if (node->type == htPoint) {
				node->point = htree_new_point_coord(shelf_x, shelf_y);
			} else {
				node->rect = htree_new_rect_coord(shelf_x, shelf_y, w, h);
			}
			shelf_x += w + PADDING;
			if (h > row_h) {
				row_h = h;
			}
		}
		if (node->children) {
			int res = htree_reconstruct_nodes_geometry(node, 1);
			if (res != HTREE_OK) {
				return res;
			}
		}
	}

	if (parent->children) {
		HTreeRect* bbox = NULL;
		int res = htree_children_bounding_rect(parent->children, &bbox);
		if (res != HTREE_OK) {
			return res;
		}
		if (bbox && !htree_empty_rect(bbox)) {
			if (parent->rect) {
				/* grow-only: never shrink or move the authored rect */
				double x2 = parent->rect->x + parent->rect->width;
				double y2 = parent->rect->y + parent->rect->height;
				double bx2 = bbox->x + bbox->width + PADDING;
				double by2 = bbox->y + bbox->height + PADDING;
				if (bbox->x - PADDING < parent->rect->x) {
					parent->rect->x = bbox->x - PADDING;
				}
				if (bbox->y - PADDING < parent->rect->y) {
					parent->rect->y = bbox->y - PADDING;
				}
				parent->rect->width = (bx2 > x2 ? bx2 : x2) - parent->rect->x;
				parent->rect->height = (by2 > y2 ? by2 : y2) - parent->rect->y;
			} else if (reconstruct_parent) {
				parent->rect = htree_new_rect_coord(bbox->x - PADDING, bbox->y - PADDING,
													bbox->width + 2 * PADDING,
													bbox->height + 2 * PADDING);
			}
		}
		if (bbox) {
			htree_destroy_rect(bbox);
		}
	}

	return HTREE_OK;
}

/* The preserving edge reconstruction: only the edges with no geometry
   of their own get the straight center-to-center attachment (projected
   onto the borders) or, for the loops, a small side loop */
static int htree_reconstruct_edges_geometry(HTreeEdge* edges)
{
	for (HTreeEdge* edge = edges; edge; edge = edge->next) {
		if (!(edge->source && (edge->source->rect || edge->source->point) &&
			  edge->target && (edge->target->rect || edge->target->point))) {
			continue;
		}
		if (edge->source_point || edge->target_point || edge->polyline) {
			/* partially specified edges are preserved */
			continue;
		}
		if (edge->source == edge->target) {
			/* the side loop on the right border */
			if (!edge->source->rect) {
				continue;
			}
			const HTreeRect* r = edge->source->rect;
			double right = r->x + r->width;
			double cy = r->y + r->height / 2.0;
			edge->source_point = htree_new_point_coord(right, cy - PADDING);
			edge->target_point = htree_new_point_coord(right, cy + PADDING);
			edge->polyline = htree_new_polyline_coord(right + PADDING, cy - PADDING);
			htree_polyline_add_point(edge->polyline, right + PADDING, cy + PADDING);
		} else {
			int res = htree_project_edge_to_borders(edge);
			if (res != HTREE_OK) {
				return res;
			}
		}
	}
	return HTREE_OK;
}

/* Grow the explicit SM border (grow-only) to cover the tree content
   including the reconstructed edge geometry */
static int htree_grow_sm_border(HTree* tree)
{
	if (!(tree->nodes && !tree->nodes->next &&
		  tree->nodes->type == htTree && tree->nodes->rect)) {
		return HTREE_OK;
	}

	std::vector<h2d::Point2dD> points;
	std::vector<h2d::FRectD> rects;
	std::vector<h2d::OPolyline> polylines;
	if (tree->nodes->children) {
		int res = htree_get_nodes_collections(tree->nodes->children, points, rects);
		if (res != HTREE_OK) {
			return res;
		}
	}
	int res = htree_get_edges_collections(tree->edges, points, rects, polylines);
	if (res != HTREE_OK) {
		return res;
	}

	HTreeRect* content = NULL;
	res = htree_construct_bounding_rect(points, rects, polylines, &content);
	if (res != HTREE_OK) {
		return res;
	}
	if (content) {
		if (!htree_empty_rect(content)) {
			HTreeRect* b = tree->nodes->rect;
			double x2 = b->x + b->width;
			double y2 = b->y + b->height;
			double cx2 = content->x + content->width + PADDING;
			double cy2 = content->y + content->height + PADDING;
			if (content->x - PADDING < b->x) {
				b->x = content->x - PADDING;
			}
			if (content->y - PADDING < b->y) {
				b->y = content->y - PADDING;
			}
			b->width = (cx2 > x2 ? cx2 : x2) - b->x;
			b->height = (cy2 > y2 ? cy2 : y2) - b->y;
		}
		htree_destroy_rect(content);
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

	res = htree_convert_document_geometry_to_absolute(doc);
	if (res != HTREE_OK) {
		return res;
	}

	for (HTree* tree = doc->trees; tree; tree = tree->next) {
		if (tree->nodes) {
			res = htree_reconstruct_nodes_geometry(tree->nodes, reconstruct_sm);
			if (res != HTREE_OK) {
				return res;
			}
		}
		res = htree_reconstruct_edges_geometry(tree->edges);
		if (res != HTREE_OK) {
			return res;
		}
		res = htree_grow_sm_border(tree);
		if (res != HTREE_OK) {
			return res;
		}
	}

	if (doc->bounding_rect) {
		htree_destroy_rect(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}

	res = htree_build_bounding_rect(doc, &(doc->bounding_rect));
	if (res != HTREE_OK) {
		return res;
	}

	if (node_coord_format == coordNone) {
		/* a geometry-less document reconstructs into the canonical form */
		return HTREE_OK;
	}

	return htree_convert_document_geometry_to_format(doc, node_coord_format,
													 edge_coord_format,
													 edge_pl_coord_format,
													 edge_format);
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
	if (!doc->trees) {
		/* an empty document: just switch the format fields */
		doc->node_coord_format = new_node_coord_format;
		doc->edge_coord_format = new_edge_coord_format;
		doc->edge_pl_coord_format = new_edge_pl_coord_format;
		doc->edge_format = new_edge_format;
		return HTREE_OK;
	}

/*	DEBUG << "Start format: node coord " << doc->node_coord_format <<
		" edge coord " << doc->edge_coord_format <<
		" edge coord " << doc->edge_pl_coord_format <<
		" edge " << doc->edge_format << std::endl;

		htree_print_document(doc);*/
	
	res = htree_convert_document_geometry_to_absolute(doc);
	if (res != HTREE_OK) {
		return res;
	}

	if (doc->bounding_rect) {
		htree_destroy_rect(doc->bounding_rect);
		doc->bounding_rect = NULL;
	}
	res = htree_build_bounding_rect(doc, &(doc->bounding_rect));
	if (res != HTREE_OK) {
		return res;
	}
	
/*	DEBUG << "Absolute format: node coord " << doc->node_coord_format <<
		" edge coord " << doc->edge_coord_format <<
		" edge coord " << doc->edge_pl_coord_format <<
		" edge " << doc->edge_format << std::endl;

		htree_print_document(doc);*/
	
	return htree_convert_document_geometry_to_format(doc,
													 new_node_coord_format,
													 new_edge_coord_format,
													 new_edge_pl_coord_format,
													 new_edge_format);
}
