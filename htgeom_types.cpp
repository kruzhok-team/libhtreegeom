/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The the hierarchiceal tree geometry library
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

#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "htgeom.h"
#include "htgeom_types.h"

#define MAX_STR_LEN	4096
#define OSTREAM     std::cout

int htree_copy_string(char** target, size_t* size, const char* source)
{
	char* target_str;
	size_t strsize;
	if (!source) {
		*target = NULL;
		*size = 0;
		return HTREE_OK;
	}
	strsize = strlen(source);  
	if (strsize > MAX_STR_LEN - 1) {
		strsize = MAX_STR_LEN - 1;
	}
	target_str = (char*)malloc(strsize + 1);
	strcpy(target_str, source);
	target_str[strsize] = 0;
	*target = target_str;
	if (size) {
		*size = strsize;
	}
	return HTREE_OK;
}

HTreePoint* htree_new_point(void)
{
	HTreePoint* p = (HTreePoint*)malloc(sizeof(HTreePoint));
	memset(p, 0, sizeof(HTreePoint));
	return p;
}

HTreePoint* htree_new_point_coord(float x, float y)
{
	HTreePoint* p = htree_new_point();
	p->x = x;
	p->y = y;
	return p;
}

int htree_set_point(HTreePoint* dst, HTreePoint* src)
{
	if (!dst || !src) {
		return HTREE_BAD_PARAMETER;
	}
	dst->x = src->x;
	dst->y = src->y;
	return HTREE_OK;
}

HTreePoint* htree_copy_point(HTreePoint* src)
{
	HTreePoint* dst;
	if (!src) {
		return NULL;
	}
	dst = htree_new_point();
	htree_set_point(dst, src);
	return dst;
}

static void htree_update_point(HTreePoint* p, double dx, double dy)
{
	if (p) {
		p->x += dx;
		p->y += dy;
	}
}

static double round_number(double num, unsigned int signs)
{
	double factor = 1.0;
 	for (unsigned int i = 0; i < signs; i++) {
		factor *= 10.0;
	}
	double value = (int)(num * factor + .5);
    return (double)value / factor;
}

int htree_round_point(HTreePoint* p, unsigned int signs)
{
	if (!p) {
		return HTREE_BAD_PARAMETER;
	}
	p->x = round_number(p->x, signs);
	p->y = round_number(p->y, signs);
	return HTREE_OK;
}

int htree_destroy_point(HTreePoint* p)
{
	if (!p) {
		return HTREE_BAD_PARAMETER;
	}
	free(p);
	return HTREE_OK;
}

HTreePoint* htree_rect_center_point(const HTreeRect* r, HTCoordFormat geometry_format)
{
	if (!r) {
		return NULL;
	}

	HTreePoint* point = htree_new_point();
	
	if (geometry_format == coordLocalCenter) {
		point->x = r->x;
		point->y = r->y;
	} else {
		point->x = r->x + r->width / 2.0;
		point->y = r->y + r->height / 2.0;
	}
	
	return point;
}

int htree_print_point(const HTreeRect* r)
{
	OSTREAM << r;
	return HTREE_OK;
}

HTreeRect* htree_new_rect(void)
{
	HTreeRect* r = (HTreeRect*)malloc(sizeof(HTreeRect));
	memset(r, 0, sizeof(HTreeRect));
	return r;
}

HTreeRect* htree_new_rect_coord(float x, float y, float w, float h)
{
	HTreeRect* r = htree_new_rect();
	r->x = x;
	r->y = y;
	r->width = w;
	r->height = h;
	return r;
}

int htree_set_rect(HTreeRect* dst, HTreeRect* src)
{
	if (!src || !dst) {
		return HTREE_BAD_PARAMETER;
	}
	dst->x = src->x;
	dst->y = src->y;
	dst->width = src->width;
	dst->height = src->height;
	return HTREE_OK;
}

HTreeRect* htree_copy_rect(HTreeRect* src)
{
	HTreeRect* dst;
	if (!src) {
		return NULL;
	}
	dst = htree_new_rect();
	htree_set_rect(dst, src);
	return dst;	
}

int htree_init_rect(HTreeRect* rect)
{
	if (!rect) {
		return HTREE_BAD_PARAMETER;
	}
	memset(rect, 0, sizeof(HTreeRect));
	return HTREE_OK;
}

int htree_empty_rect(const HTreeRect* r)
{
	if (!r) return 0;
	return r->x == 0.0 && r->y == 0.0 && r->width == 0.0 && r->height == 0.0;
}

int htree_compare_rects(HTreeRect* a, HTreeRect* b)
{
	if (!a || !b) return -1;
	return a->x != b->x || a->y != b->y || a->width != b->width || a->height != b->height;	
}

static void htree_update_rect(HTreeRect* r, double dx, double dy)
{
	if (!r) return ;
	r->x += dx;
	r->y += dy;
}

int htree_round_rect(HTreeRect* r, unsigned int signs)
{
	if (!r) return HTREE_BAD_PARAMETER;
	r->x = round_number(r->x, signs);
	r->y = round_number(r->y, signs);
	r->width = round_number(r->width, signs);
	r->height = round_number(r->height, signs);
	return HTREE_OK;
}

int htree_destroy_rect(HTreeRect* r)
{
	if (!r) return HTREE_BAD_PARAMETER;
	free(r);
	return HTREE_OK;
}

int htree_print_rect(const HTreeRect* r)
{
	OSTREAM << r;
	return HTREE_OK;
}

HTreePolyline* htree_new_polyline(void)
{
	HTreePolyline* pl = (HTreePolyline*)malloc(sizeof(HTreePolyline));
	memset(pl, 0, sizeof(HTreePolyline));
	return pl;
}

HTreePolyline* htree_new_polyline_coord(float x, float y)
{
	HTreePolyline* pl = htree_new_polyline();
	pl->point.x = x;
	pl->point.y = y;	
	return pl;
}

void htree_polyline_add_point(HTreePolyline* pl, float x, float y)
{
	if (!pl) return ;
	
	HTreePolyline* new_point = htree_new_polyline();
	new_point->point.x = x;
	new_point->point.y = y;	

	if (pl->next) {
		HTreePolyline* prev = pl->next;
		while (prev->next) prev = prev->next;
		prev->next = new_point;
	} else {
		pl->next = new_point;
	}
}

int htree_set_polyline(HTreePolyline* dst, HTreePolyline* src)
{
	HTreePolyline *dst_pl, *dst_prev, *src_pl;
	if (!dst || !src) {
		return HTREE_BAD_PARAMETER;
	}

	src_pl = src;
	dst_pl = dst;
	dst_prev = NULL;

	while (src_pl) {
		if (!dst_pl) {
			dst_pl = htree_new_polyline();
			dst_prev->next = dst_pl;
		}
		dst_prev = dst_pl;
		htree_set_point(&(dst_pl->point), &(src_pl->point)); 
		dst_pl = dst_pl->next;
		src_pl = src_pl->next;
	}

	if (dst_pl) {
		dst_prev->next = NULL;
		htree_destroy_polyline(dst_pl);
	}
	
	return HTREE_OK;
}

HTreePolyline* htree_copy_polyline(HTreePolyline* src)
{
	HTreePolyline *dst = NULL, *prev, *pl;
	if (!src) {
		return NULL;
	}
	do {
		pl = htree_new_polyline();
		htree_set_point(&(pl->point), &(src->point));
		if (dst) {
			prev->next = pl;
		} else {
			dst = pl;
		}
		prev = pl;
		src = src->next;
	} while (src);
	return dst;	
}

static void htree_update_polyline(HTreePolyline* pl, double dx, double dy)
{
	if (!pl) return ;
	do {
		htree_update_point(&(pl->point), dx, dy);
		pl = pl->next;
	} while (pl);
}

int htree_destroy_polyline(HTreePolyline* polyline)
{
	if (!polyline) return HTREE_BAD_PARAMETER;
	HTreePolyline* pl;
	do {
		pl = polyline;
		polyline = polyline->next;
		free(pl);
	} while (polyline);
	return HTREE_OK;
}

HTreeNode* htree_new_node(HTNodeType node_type, const char* _id)
{
	HTreeNode* new_node = (HTreeNode*)malloc(sizeof(HTreeNode));
	memset(new_node, 0, sizeof(HTreeNode));
	htree_copy_string(&(new_node->id), &(new_node->id_len), _id);
	new_node->type = node_type;
	return new_node;	
}

void htree_node_set_rect(HTreeNode* node, float x, float y, float w, float h)
{
	if (!node) return ;
	if (node->rect) {
		node->rect->x = x;
		node->rect->y = y;
		node->rect->width = w;
		node->rect->height = h;		
	} else {
		HTreeRect* r = htree_new_rect_coord(x, y, w, h);
		node->rect = r;
	}
}

void htree_node_set_point(HTreeNode* node, float x, float y)
{
	if (!node) return ;
	if (node->point) {
		node->point->x = x;
		node->point->y = y;
	} else {
		HTreePoint* p = htree_new_point_coord(x, y);
		node->point = p;
	}
}

void htree_add_sibling_node(HTreeNode* node, HTreeNode* new_node)
{
	if (!node || !new_node) return ;
	if (node->next) {
		HTreeNode* prev = node->next;
		while (prev->next) prev = prev->next;
		prev->next = new_node;
	} else {
		node->next = new_node;
		node->parent->type = htCompositeNode;
	}
	new_node->parent = node->parent;
}

void htree_add_child_node(HTreeNode* node, HTreeNode* new_node)
{
	if (!node || !new_node) return ;
	if (node->children) {
		HTreeNode* prev = node->children;
		while (prev->next) prev = prev->next;
		prev->next = new_node;
	} else {
		node->children = new_node;
		node->type = htCompositeNode;
	}
	new_node->parent = node;
}

HTreeNode* htree_copy_node(HTreeNode* src)
{
	HTreeNode *dst, *n, *dst_child, *src_child;
	if (!src || !(src->id)) {
		return NULL;
	}
	dst = htree_new_node(src->type, src->id);
	if (src->point) {
		dst->point = htree_copy_point(src->point);
	}
	if (src->rect) {
		dst->rect = htree_copy_rect(src->rect);
	}
	if (src->children) {
		for (src_child = src->children; src_child; src_child = src_child->next) {
			dst_child = htree_copy_node(src_child);
			dst_child->parent = dst;
			if (dst->children) {
				n = dst->children;
				while (n->next) n = n->next;
				n->next = dst_child;
			} else {
				dst->children = dst_child;
			}
		}
	}
	return dst;	
}

HTreeNode* htree_find_node_by_id(HTreeNode* root, const char* id)
{
	HTreeNode* node;
	HTreeNode* found;
	for (node = root; node; node = node->next) {
		if (strcmp(node->id, id) == 0) {
			return node;
		}
		if (node->children) {
			found = htree_find_node_by_id(node->children, id);
			if (found) return found;
		}
	}
	return NULL;
}

static int htree_destroy_all_nodes(HTreeNode* node);

int htree_destroy_node(HTreeNode* node)
{
	if(node != NULL) {
		if (node->id) free(node->id);
		if (node->children) {
			htree_destroy_all_nodes(node->children);
		}
		if (node->point) free(node->point);
		if (node->rect) free(node->rect);
		free(node);
	}
	return HTREE_OK;
}

static int htree_destroy_all_nodes(HTreeNode* node)
{
	HTreeNode* n;
	if(node != NULL) {
		do {
			n = node;
			node = node->next;
			htree_destroy_node(n);
		} while (node);
	}
	return HTREE_OK;
}

HTreeEdge* htree_new_edge(const char* _id, const char* source_id, const char* target_id)
{
	HTreeEdge* new_edge = (HTreeEdge*)malloc(sizeof(HTreeEdge));
	memset(new_edge, 0, sizeof(HTreeEdge));
	htree_copy_string(&(new_edge->id), &(new_edge->id_len), _id);
	htree_copy_string(&(new_edge->source_id), &(new_edge->source_id_len), source_id);
	htree_copy_string(&(new_edge->target_id), &(new_edge->target_id_len), target_id);
	return new_edge;	
}

void htree_edge_set_points(HTreeEdge* edge, float source_x, float source_y, float target_x, float target_y)
{
	if (!edge) return ;

	if (edge->source_point) {
		edge->source_point->x = source_x;
		edge->source_point->y = source_y;
	} else {
		HTreePoint* p = htree_new_point_coord(source_x, source_y);
		edge->source_point = p;
	}

	if (edge->target_point) {
		edge->target_point->x = target_x;
		edge->target_point->y = target_y;
	} else {
		HTreePoint* p = htree_new_point_coord(target_x, target_y);
		edge->target_point = p;
	}
}

HTreeEdge* htree_copy_edge(HTreeEdge* src)
{
	HTreeEdge* dst;
	if (!src) {
		return NULL;
	}
	dst = htree_new_edge(src->id, src->source_id, src->target_id);
/*	if (src->abs_source_rect) {
		dst->abs_source_rect = htree_copy_rect(src->abs_source_rect);
	}
	if (src->abs_target_rect) {
		dst->abs_target_rect = htree_copy_rect(src->abs_target_rect);
		}*/
    if (src->polyline) {
		dst->polyline = htree_copy_polyline(src->polyline);
	}
	if (src->source_point) {
		dst->source_point = htree_copy_point(src->source_point);
	}
	if (src->target_point) {
		dst->target_point = htree_copy_point(src->target_point);
	}
	if (src->label_point) {
		dst->label_point = htree_copy_point(src->label_point);
	}
	if (src->label_rect) {
		dst->label_rect = htree_copy_rect(src->label_rect);
	}
	return dst;	
}

int htree_destroy_edge(HTreeEdge* e)
{
	if (!e) {
		return HTREE_BAD_PARAMETER;
	}
	if (e->id) free(e->id);
	if (e->source_id) free(e->source_id);
	if (e->target_id) free(e->target_id);
//	if (e->abs_source_rect) free(e->abs_source_rect);
//	if (e->abs_target_rect) free(e->abs_target_rect);
	if (e->polyline) {
		htree_destroy_polyline(e->polyline);
	}
	if (e->source_point) htree_destroy_point(e->source_point); 
	if (e->target_point) htree_destroy_point(e->target_point);
	if (e->label_point) htree_destroy_point(e->label_point);
	if (e->label_rect) htree_destroy_rect(e->label_rect);
	free(e);
	return HTREE_OK;	
}

HTree* htree_new_tree(void)
{
	HTree* tree = (HTree*)malloc(sizeof(HTree));
	memset(tree, 0, sizeof(HTree));
	return tree;
}

void htree_add_node(HTree* tree, HTreeNode* n)
{
	if (!tree || !n) return ;
	if (tree->nodes) {
		HTreeNode* prev = tree->nodes;
		while(prev->next) prev = prev->next;
		prev->next = n;
	} else {
		tree->nodes = n;
	}
}

void htree_add_edge(HTree* tree, HTreeEdge* e)
{
	if (!tree || !e) return ;
	if (tree->edges) {
		HTreeEdge* prev = tree->edges;
		while(prev->next) prev = prev->next;
		prev->next = e;
	} else {
		tree->edges = e;
	}
}

HTree* htree_copy_tree(HTree* src)
{
	HTree *result = NULL, *dst;
	HTreeNode* node, *new_node, *prev_node;
	HTreeEdge *edge, *new_edge, *prev_edge;

	while (src) {
		dst = htree_new_tree();
		
		if (result) {
			HTree* t = result;
			while (t->next) t = t->next;
			t->next = dst;
		} else {
			result = dst;
		}
		
		if (src->nodes) {
			node = src->nodes;
			while (node) {
				new_node = htree_copy_node(node);
				if (dst->nodes) {
					prev_node->next = new_node;
				} else {
					dst->nodes = new_node;
				}
				prev_node = new_node;
				node = node->next;
			}
		}
		
		if (src->edges) {
			edge = src->edges; 
			while (edge) {
				new_edge = htree_copy_edge(edge);
				if (dst->edges) {
					prev_edge->next = new_edge;	
				} else {
					dst->edges = new_edge;
				}
				prev_edge = new_edge;
				edge = edge->next;	
			}
		}
	
		edge = dst->edges;

		/* reconstruct source/target nodes */
		while (edge) {
			HTreeNode* source = htree_find_node_by_id(dst->nodes, edge->source_id);
			HTreeNode* target = htree_find_node_by_id(dst->nodes, edge->target_id);
			if (!source || !target) {
				htree_destroy_tree(result);
				return NULL;
			}
			edge->source = source;
			edge->target = target;
			edge = edge->next;
		}

		src = src->next;
	}
	return dst;	
}

int htree_destroy_tree(HTree* tree)
{
	HTree *t;
	HTreeEdge *edge, *e;
	while (tree) {
		if (tree->nodes) {
			htree_destroy_all_nodes(tree->nodes);
		}
		if (tree->edges) {
			edge = tree->edges;
			do {
				e = edge;
				edge = edge->next;
				htree_destroy_edge(e);
			} while (edge);
		}
		t = tree;
		tree = tree->next;
		free(t);
	}
	return HTREE_OK;
}

int htree_node_has_geometry(const HTreeNode* node)
{
	while (node) {
		if (node->point || node->rect) {
			return 1;
		}
		if (node->children) {
			if (htree_node_has_geometry(node->children)) {
				return 1;
			}
		}
		node = node->next;
	}
	return 0;
}

int htree_node_has_toplevel_geometry(const HTreeNode* node)
{
	int found = 0;
	
	for (const HTreeNode* n = node; n; n = n->next) {
		if (n->point || n->rect) {
			if (found) return 0;
			found = 1;
		}
	}

	if (found) return 1;

	for (const HTreeNode* n = node; n; n = n->next) {	
		if (n->children) {
			if (htree_node_has_toplevel_geometry(n->children)) {
				if (found) return 0;
				found = 1;
			}
		}
	}
	
	return found;
}

int htree_tree_has_geometry(const HTree* tree)
{
	if (!tree) {
		return 0;
	}

	for (; tree; tree = tree->next) {
		if (htree_node_has_geometry(tree->nodes)) {
			return 1;
		}
		for (const HTreeEdge* edge = tree->edges; edge; edge = edge->next) {
			if (edge->polyline ||
				edge->source_point ||
				edge->target_point ||
				edge->label_point ||
				edge->label_rect) {
				
				return 1;
			}
		}	
	}
	return 0;
}

HTDocument* htree_new_document(HTCoordFormat _node_coord_format,
							   HTCoordFormat _edge_coord_format,
							   HTCoordFormat _edge_pl_coord_format,
							   HTEdgeFormat _edge_format)
{
	HTDocument* doc = (HTDocument*)malloc(sizeof(HTDocument));
	memset(doc, 0, sizeof(HTDocument));
	doc->node_coord_format = _node_coord_format;
	doc->edge_coord_format = _edge_coord_format;
	doc->edge_pl_coord_format = _edge_pl_coord_format;
	doc->edge_format = _edge_format;
	return doc;
}

void htree_add_tree(HTDocument* doc, HTree* tree)
{
	if (!doc || !tree) return ;
	if (doc->trees) {
		HTree* prev = doc->trees;
		while(prev->next) prev = prev->next;
		prev->next = tree;
	} else {
		doc->trees = tree;
	}
}

HTDocument* htree_copy_document(HTDocument* src)
{
	HTDocument* dst;
	if (!src) {
		return NULL;
	}

	dst = htree_new_document(src->node_coord_format,
							 src->edge_coord_format,
							 src->edge_pl_coord_format,
							 src->edge_format);
	if (src->trees) {
		dst->trees = htree_copy_tree(src->trees);
	}
	if (src->bounding_rect) {
		dst->bounding_rect = htree_copy_rect(src->bounding_rect);
	}
	return dst;
}

int htree_destroy_document(HTDocument* doc)
{
	if (doc) {
		if (doc->trees) {
			htree_destroy_tree(doc->trees);
		}
		if (doc->bounding_rect) {
			htree_destroy_rect(doc->bounding_rect);
		}
		free(doc);
	}
	return HTREE_OK;
}

int htree_print_document(const HTDocument* doc)
{
	OSTREAM << doc << std::endl;
	return HTREE_OK;
}
