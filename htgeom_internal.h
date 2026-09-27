/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The private interface shared by the transform and the layout sources
 *
 * Copyright (C) 2026 Alexey Fedoseev <aleksey@fedoseev.net>
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

#ifndef __HIERARCHICAL_TREE_GEOMETRY_INTERNAL_H
#define __HIERARCHICAL_TREE_GEOMETRY_INTERNAL_H

#include "htgeom.h"

/* htgeom.cpp: the transform helpers reused by the layout */
int  htree_node_is_comment(const HTreeNode* node);
int  htree_node_is_descendant(const HTreeNode* parent, const HTreeNode* node);
int  htree_convert_document_geometry_to_absolute(HTDocument* doc);
int  htree_children_bounding_rect(HTreeNode* parent, const HTreeEdge* edges, HTreeRect** result);
void htree_clean_tree_geometry(HTreeNode* node);
void htree_shift_subtree(HTreeNode* node, double dx, double dy);
int  htree_node_box(const HTreeNode* node, double* x, double* y, double* w, double* h);
int  htree_attach_edge_minimal(HTreeEdge* edge);
int  htree_project_edge_to_borders(HTreeEdge* edge);
/* the side loop of a self-transition: on the right border, or on the bottom one */
int  htree_reconstruct_edge_loop(HTreeEdge* edge, int bottom_side);
int  htree_grow_sm_border(HTree* tree);

/* htgeom_layout.cpp: the layered layout of one tree (docs/reconstruction.md) */
int  htree_layout_check_options(const HTLayoutOptions* in, HTLayoutOptions* out);
int  htree_layout_tree(HTree* tree, int reconstruct_sm, const HTLayoutOptions* opts);

#endif
