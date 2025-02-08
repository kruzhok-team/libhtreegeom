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

#ifndef __HIERARCHICAL_TREE_GEOMETRY_H
#define __HIERARCHICAL_TREE_GEOMETRY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	
/* -----------------------------------------------------------------------------
 * the hierarchiceal tree geometry types
 * ----------------------------------------------------------------------------- */
	
typedef struct {
    double                  x, y;
} HTreePoint;

typedef struct {
    double                  x, y, width, height;
} HTreeRect;

typedef struct _HTreePolyline {
    HTreePoint              point;
    struct _HTreePolyline*  next;
} HTreePolyline;

typedef enum {
    htTree = 0,             /* a tree */
	htSimpleNode = 1,       /* a simple rect node */
	htCompositeNode = 2,    /* a composite rect node */
	htPoint = 4             /* a point */
} HTNodeType;

typedef struct _HTreeNode {
    HTNodeType              type;
    char*                   id;
    size_t                  id_len;
	HTreePoint*             point;
	HTreeRect*              rect;
    struct _HTreeNode*      parent;
    struct _HTreeNode*      children;
    struct _HTreeNode*      next;
} HTreeNode;

typedef struct _HTreeEdge {
    char*                   id;
    size_t                  id_len;
    char*                   source_id;
    size_t                  source_id_len;
    char*                   target_id;
    size_t                  target_id_len;
    HTreeNode*              source;
    HTreeNode*              target;
    HTreePolyline*          polyline;
    HTreePoint*             source_point;
    HTreePoint*             target_point;
    HTreePoint*             label_point;
    HTreeRect*              label_rect;
    struct _HTreeEdge*      next;
} HTreeEdge;

typedef struct _HTree {
    HTreeNode*              nodes;
    HTreeEdge*              edges;
    struct _HTree*          next;
} HTree;

typedef enum {
	coordNone = 0,        /* no geometry information */
	coordAbsolute = 1,    /* absolute coordinates */ 
	coordLeftTop = 2,     /* left-top-oriented relative coordinates */
	coordLocalCenter = 4, /* center-oriented relative coordinates */	
} HTCoordFormat;

typedef enum {
	edgeNone = 0,
	edgeCenter = 1,       /* source & target points are bind to the nodes' centers */
	edgeBorder = 2,       /* source & target points are placed on the nodes' borders */
} HTEdgeFormat;
	
typedef struct _HTDocument {
	HTCoordFormat           node_coord_format;     /* geometry coordinate format for nodes */
	HTCoordFormat           edge_coord_format;     /* geometry coordinate format for edge */
	HTCoordFormat           edge_pl_coord_format;  /* geometry coordinate format for edge polylines */
	HTEdgeFormat            edge_format;           /* edge format */
	HTree*                  trees;                 /* the trees of nodes and edges */
	HTreeRect*              bounding_rect;         /* bounding rect */
} HTDocument;

/* -----------------------------------------------------------------------------
 * The hierarchical tree geometry functions
 * ----------------------------------------------------------------------------- */

	/* return codes */
	#define                 HTREE_OK                      0
	#define                 HTREE_BAD_PARAMETER           1
	#define                 HTREE_NOT_FOUND               2
	#define                 HTREE_GEOMETRY_TRANFORM_ERROR 3

	HTreePoint*             htree_new_point(void);
    HTreePoint*             htree_copy_point(HTreePoint* src);
    int                     htree_set_point(HTreePoint* dst, HTreePoint* src);
	int                     htree_round_point(HTreePoint* p, unsigned int signs);
	int                     htree_destroy_point(HTreePoint* p);
	int                     htree_print_point(const HTreePoint* p);
	HTreePoint*             htree_rect_center_point(const HTreeRect* r, HTCoordFormat geometry_format);

	HTreeRect*              htree_new_rect(void);
    HTreeRect*              htree_copy_rect(HTreeRect* src);
	int                     htree_init_rect(HTreeRect* rect);
	int                     htree_set_rect(HTreeRect* dst, HTreeRect* src);
	int                     htree_round_rect(HTreeRect* r, unsigned int signs);
	int                     htree_destroy_rect(HTreeRect* r);
	int                     htree_empty_rect(const HTreeRect* r);
	int                     htree_print_rect(const HTreeRect* r);
	
	HTreePolyline*          htree_new_polyline(void);
	HTreePolyline*          htree_copy_polyline(HTreePolyline* src);
	int                     htree_set_polyline(HTreePolyline* dst, HTreePolyline* src);
	int                     htree_destroy_polyline(HTreePolyline* polyline);

	HTreeNode*              htree_new_node(HTNodeType node_type, const char* _id);
	HTreeNode*              htree_copy_node(HTreeNode* src);
	HTreeNode*              htree_find_node_by_id(HTreeNode* root, const char* id);
	int                     htree_destroy_node(HTreeNode* node);
	int                     htree_node_has_geometry(const HTreeNode* node);
	int                     htree_node_has_toplevel_geometry(const HTreeNode* node);

	HTreeEdge*              htree_new_edge(const char* _id, const char* source_id, const char* target_id);
	HTreeEdge*              htree_copy_edge(HTreeEdge* src);
	int                     htree_destroy_edge(HTreeEdge* edge);

	HTree*                  htree_new_tree(void);
	HTree*                  htree_copy_tree(HTree* src);
	int                     htree_destroy_tree(HTree* tree);
	int                     htree_tree_has_geometry(const HTree* tree);

	HTDocument*             htree_new_document(HTCoordFormat _node_coord_format,
											   HTCoordFormat _edge_coord_format,
											   HTCoordFormat _edge_pl_coord_format,
											   HTEdgeFormat _edge_format);
	HTDocument*             htree_copy_document(HTDocument* src);
	int                     htree_destroy_document(HTDocument* doc);
	int                     htree_print_document(const HTDocument* doc);

	int                     htree_reconstruct_document_geometry(HTDocument* doc, int reconstruct_sm);
	int                     htree_convert_document_geometry(HTDocument* doc,
															HTCoordFormat new_node_coord_format,
															HTCoordFormat new_edge_coord_format,
															HTCoordFormat new_edge_pl_coord_format,
															HTEdgeFormat new_edge_format);
	
#ifdef __cplusplus
}
#endif
    
#endif
