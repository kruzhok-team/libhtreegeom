/* -----------------------------------------------------------------------------
 * The Cyberiada Hierarchical Tree Geometry library implemention
 *
 * The layered layout of the geometry reconstruction (docs/reconstruction.md)
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

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include "htgeom.h"
#include "htgeom_types.h"
#include "htgeom_internal.h"

/* -----------------------------------------------------------------------------
 * The options
 * ----------------------------------------------------------------------------- */

void htree_default_layout_options(HTLayoutOptions* opts)
{
	if (!opts) return;
	memset(opts, 0, sizeof(HTLayoutOptions));
	opts->direction = htFlowRight;
	opts->mode = htLayoutAdaptive;
	opts->aspect = LAYOUT_ASPECT;
	opts->node_gap = NODE_GAP;
	opts->layer_gap = LAYER_GAP;
	opts->padding = PADDING;
	opts->sweeps = LAYOUT_SWEEPS;
	opts->node_width = NODE_WIDTH;
	opts->node_height = NODE_HEIGHT;
	opts->point_size = POINT_SIZE;
	opts->label_width = LABEL_WIDTH;
	opts->label_height = LABEL_HEIGHT;
}

int htree_layout_check_options(const HTLayoutOptions* in, HTLayoutOptions* out)
{
	if (!in || !out) {
		return HTREE_BAD_PARAMETER;
	}
	if (in->direction != htFlowDown && in->direction != htFlowRight) {
		return HTREE_BAD_PARAMETER;
	}
	if (in->mode != htLayoutFixed && in->mode != htLayoutAlternate && in->mode != htLayoutAdaptive) {
		return HTREE_BAD_PARAMETER;
	}
	*out = *in;
	if (out->aspect <= 0.0) out->aspect = LAYOUT_ASPECT;
	if (out->node_gap <= 0.0) out->node_gap = NODE_GAP;
	if (out->layer_gap <= 0.0) out->layer_gap = LAYER_GAP;
	if (out->padding <= 0.0) out->padding = PADDING;
	if (out->sweeps <= 0) out->sweeps = LAYOUT_SWEEPS;
	if (out->node_width <= 0.0) out->node_width = NODE_WIDTH;
	if (out->node_height <= 0.0) out->node_height = NODE_HEIGHT;
	if (out->point_size <= 0.0) out->point_size = POINT_SIZE;
	if (out->label_width <= 0.0) out->label_width = LABEL_WIDTH;
	if (out->label_height <= 0.0) out->label_height = LABEL_HEIGHT;
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * The layout model: the containers of the tree and the layout graph of one
 * container (the local hierarchy)
 * ----------------------------------------------------------------------------- */

typedef enum {
	htVertexReal = 0,      /* a child of the container */
	htVertexEdgeDummy,     /* a bend of a long edge */
	htVertexLabelDummy,    /* the label slot of a transition */
	htVertexVirtual        /* ENTRY / EXIT */
} HTLayoutVertexKind;

typedef struct {
	HTreeNode*          node;
	HTNodeRole          role;
	HTLayoutVertexKind  kind;
	int                 edge;        /* the layout edge of a dummy */
	int                 layer;
	int                 order;       /* the index inside the layer */
	double              flow_size, cross_size;
	double              before, after; /* the cross-axis extents around the anchor */
	double              flow, cross; /* the anchor in the frame: the centre, or the line of a label slot */
	int                 seed;        /* the update: the vertex had geometry */
	double              seed_x, seed_y;       /* its centre in the document */
	double              seed_flow, seed_cross; /* its centre in the frame */
	int                 seed_layer;  /* its band among the seeded vertices, 1.. */
} HTLayoutVertex;

typedef struct {
	int                 source, target;   /* in the layout direction */
	HTreeEdge*          edge;
	int                 reversed;
	int                 has_label;
	double              label_w, label_h;
	int                 drawn_ends;       /* the update: a new edge between two drawn vertices */
	std::vector<int>    chain;            /* source, dummies, target by layer */
} HTLayoutEdge;

typedef struct {
	HTFlowDirection                 direction;
	int                             update;     /* the seeded layout */
	std::vector<HTLayoutVertex>     vertices;   /* ENTRY, the children, EXIT, the dummies */
	std::vector<HTLayoutEdge>       edges;
	std::vector<std::vector<int> >  layers;     /* 0 = ENTRY .. last + 1 = EXIT */
	std::vector<std::vector<int> >  up, down;   /* the neighbours in the adjacent layers */
	int                             entry, exit, last;
	std::vector<double>             band_lo, band_hi; /* the update: the flow extent of a drawn layer */
	std::vector<int>                band_drawn;
} HTLayoutGraph;

typedef enum {
	htContainerLeaf = 0,   /* no layout content */
	htContainerGraph,      /* states and pseudostates: a layered graph */
	htContainerRegions     /* the regions of a composite */
} HTLayoutContainerKind;

typedef struct {
	HTreeEdge*          edge;
	int                 source_child;   /* the vertex child index, -1 = ENTRY */
	int                 target_child;   /* -1 = EXIT */
	int                 self_loop;
	int                 has_label;
} HTLayoutSeed;

typedef struct {
	HTreeEdge*              edge;
	std::vector<HTreePoint> bends;      /* container coordinates, source to target */
	int                     has_label;
	HTreeRect               label;
} HTLayoutPiece;

typedef struct {
	HTreeNode*                  node;       /* NULL: the pseudo root of a top-level list */
	HTLayoutContainerKind       kind;
	int                         level;
	HTFlowDirection             direction;  /* of the content */
	std::vector<HTreeNode*>     children;   /* the vertex children in the child order */
	std::vector<HTLayoutSeed>   seeds;
	std::vector<HTLayoutPiece>  pieces;
	double                      shelf;      /* the comment shelf cursor */
} HTLayoutContainer;

typedef struct {
	HTree*                          tree;
	HTLayoutOptions                 opts;
	HTFlowDirection                 start;        /* the root frame of this pass */
	int                             reconstruct_sm;
	int                             sm_had_rect;
	int                             update;       /* the existing geometry is the seed */
	std::vector<HTLayoutContainer>  containers;   /* pre-order, 0 = the root */
	std::map<const HTreeNode*, int> index;        /* lookup only */
	std::map<const HTreeNode*, HTreePoint> seeds; /* the update: the centres before the layout */
	std::set<const HTreeEdge*>      edge_seeds;   /* the update: the edges that had geometry */
	std::set<const HTreeEdge*>      routed;       /* the edges routed in this pass */
} HTLayoutContext;

static int htree_layout_seeded(const HTLayoutContext* ctx, const HTreeNode* node, HTreePoint* centre)
{
	std::map<const HTreeNode*, HTreePoint>::const_iterator it = ctx->seeds.find(node);
	if (it == ctx->seeds.end()) return 0;
	if (centre) *centre = it->second;
	return 1;
}

static int htree_layout_source_role(HTNodeRole r)
{
	return r == htRoleInitial || r == htRoleEntryPoint ||
		r == htRoleShallowHistory || r == htRoleDeepHistory;
}

static int htree_layout_sink_role(HTNodeRole r)
{
	return r == htRoleFinal || r == htRoleTerminate || r == htRoleExitPoint;
}

/* an entry / exit point on the border of a nested container; the points of
   a machine are its vertices, laid out like the initial and the finals */
static int htree_layout_is_border_point(const HTreeNode* n)
{
	return n && n->type == htPoint && (n->role == htRoleEntryPoint || n->role == htRoleExitPoint) &&
		n->parent && n->parent->type != htTree;
}

/* a child that takes part in the layout graph of its container */
static int htree_layout_is_vertex(const HTreeNode* n)
{
	return n && n->type != htComment && n->type != htRegion && !htree_layout_is_border_point(n);
}

static int htree_layout_is_container(const HTreeNode* n)
{
	return n && n->type != htPoint && n->type != htComment &&
		(n->children || n->type == htCompositeNode || n->type == htRegion || n->type == htTree);
}

/* a container drawn with its own border: a composite or a submachine state */
static int htree_layout_is_composite(const HTreeNode* n)
{
	return n && n->type != htRegion && n->type != htTree;
}

static void htree_layout_to_doc(HTFlowDirection d, double flow, double cross, double* x, double* y)
{
	if (d == htFlowDown) { *x = cross; *y = flow; } else { *x = flow; *y = cross; }
}

static void htree_layout_size_to_frame(HTFlowDirection d, double w, double h,
									   double* flow_size, double* cross_size)
{
	if (d == htFlowDown) { *flow_size = h; *cross_size = w; } else { *flow_size = w; *cross_size = h; }
}

/* the preferred frame of a nesting level: the start, alternating by depth */
static HTFlowDirection htree_layout_preferred(const HTLayoutOptions* o, HTFlowDirection start, int level)
{
	if (o->mode != htLayoutFixed && (level % 2) == 1) {
		return start == htFlowDown ? htFlowRight : htFlowDown;
	}
	return start;
}

/* the target width / height of a level: a RIGHT frame means a wide box */
static double htree_layout_target(const HTLayoutOptions* o, HTFlowDirection preferred)
{
	return preferred == htFlowRight ? o->aspect : 1.0 / o->aspect;
}

/* the log-scale distance of a box shape from the target ratio */
static double htree_layout_shape_distance(double w, double h, double target)
{
	if (w <= 0.0 || h <= 0.0) return 0.0;
	return std::fabs(std::log((w / h) / target));
}

/* the box is on the target's side of the square: wide for a wide target */
static int htree_layout_on_side(double w, double h, double target)
{
	int wide = w <= 0.0 || h <= 0.0 || w >= h;
	return wide == (target >= 1.0);
}

/* the better of two shapes: the one on the target's side, among those the
   closer ratio; 0 = the first, 1 = the second, -1 = a tie */
static int htree_layout_shape_better(double w1, double h1, double w2, double h2, double target)
{
	int s1 = htree_layout_on_side(w1, h1, target), s2 = htree_layout_on_side(w2, h2, target);
	double d1, d2;
	if (s1 != s2) return s1 ? 0 : 1;
	d1 = htree_layout_shape_distance(w1, h1, target);
	d2 = htree_layout_shape_distance(w2, h2, target);
	if (std::fabs(d1 - d2) < HTREE_COORD_EPS) return -1;
	return d1 < d2 ? 0 : 1;
}

/* the frame with the better shape, the preferred one on a tie */
static HTFlowDirection htree_layout_choose(double w_down, double h_down, double w_right, double h_right,
										   double target, HTFlowDirection preferred)
{
	int better = htree_layout_shape_better(w_down, h_down, w_right, h_right, target);
	if (better < 0) return preferred;
	return better == 0 ? htFlowDown : htFlowRight;
}

static HTreeNode* htree_layout_first_child(const HTLayoutContext* ctx, int ci)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	return c.node ? c.node->children : ctx->tree->nodes;
}

/* the container whose child list holds the node, -1 for the root itself */
static int htree_layout_container_of(const HTLayoutContext* ctx, const HTreeNode* node)
{
	if (!node) return -1;
	if (!node->parent) {
		return ctx->containers[0].node == node ? -1 : 0;
	}
	std::map<const HTreeNode*, int>::const_iterator it = ctx->index.find(node->parent);
	return it == ctx->index.end() ? -1 : it->second;
}

/* the vertex child of the container that contains the node, -1 when none */
static int htree_layout_child_containing(const HTLayoutContext* ctx, int ci, const HTreeNode* x)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	while (x && x->parent != c.node) {
		x = x->parent;
	}
	if (!x) return -1;
	for (size_t k = 0; k < c.children.size(); k++) {
		if (c.children[k] == x) return (int)k;
	}
	return -1;
}

static void htree_layout_collect(HTLayoutContext* ctx, HTreeNode* node, int level)
{
	int ci = (int)ctx->containers.size();
	int has_region = 0, has_vertex = 0;
	HTreeNode* first;

	ctx->containers.push_back(HTLayoutContainer());
	ctx->containers[ci].node = node;
	ctx->containers[ci].level = level;
	ctx->containers[ci].direction = htree_layout_preferred(&ctx->opts, ctx->start, level);
	ctx->containers[ci].shelf = 0.0;
	if (node) {
		ctx->index[node] = ci;
	}
	first = htree_layout_first_child(ctx, ci);
	for (HTreeNode* child = first; child; child = child->next) {
		if (child->type == htRegion) has_region = 1;
		if (htree_layout_is_vertex(child)) {
			has_vertex = 1;
			ctx->containers[ci].children.push_back(child);
		}
	}
	ctx->containers[ci].kind = has_region ? htContainerRegions :
		(has_vertex ? htContainerGraph : htContainerLeaf);
	for (HTreeNode* child = first; child; child = child->next) {
		if (htree_layout_is_container(child)) {
			htree_layout_collect(ctx, child, level + (child->type == htRegion ? 0 : 1));
		}
	}
}

/* The lifted edges: one walk over the edge list seeds every graph container
   on the nesting path of a transition */
static void htree_layout_seed_edges(HTLayoutContext* ctx)
{
	for (HTreeEdge* e = ctx->tree->edges; e; e = e->next) {
		std::vector<int> chain_s, chain_t;
		int lca = -1;

		if (!e->source || !e->target ||
			htree_node_is_comment(e->source) || htree_node_is_comment(e->target)) {
			continue;
		}
		for (int c = htree_layout_container_of(ctx, e->source); c >= 0;
			 c = htree_layout_container_of(ctx, ctx->containers[c].node)) {
			chain_s.push_back(c);
		}
		for (int c = htree_layout_container_of(ctx, e->target); c >= 0;
			 c = htree_layout_container_of(ctx, ctx->containers[c].node)) {
			chain_t.push_back(c);
		}
		for (size_t i = 0; i < chain_t.size() && lca < 0; i++) {
			if (std::find(chain_s.begin(), chain_s.end(), chain_t[i]) != chain_s.end()) {
				lca = chain_t[i];
			}
		}
		if (lca < 0) continue;

		for (size_t i = 0; i < chain_s.size() && chain_s[i] != lca; i++) {
			HTLayoutContainer& c = ctx->containers[chain_s[i]];
			if (c.kind != htContainerGraph) continue;
			HTLayoutSeed s;
			s.edge = e;
			s.source_child = htree_layout_child_containing(ctx, chain_s[i], e->source);
			s.target_child = -1;
			s.self_loop = 0;
			s.has_label = e->label_rect && c.node == e->source->parent;
			if (s.source_child >= 0) c.seeds.push_back(s);
		}
		for (size_t i = 0; i < chain_t.size() && chain_t[i] != lca; i++) {
			HTLayoutContainer& c = ctx->containers[chain_t[i]];
			if (c.kind != htContainerGraph) continue;
			HTLayoutSeed s;
			s.edge = e;
			s.source_child = -1;
			s.target_child = htree_layout_child_containing(ctx, chain_t[i], e->target);
			s.self_loop = 0;
			s.has_label = 0;
			if (s.target_child >= 0) c.seeds.push_back(s);
		}
		HTLayoutContainer& c = ctx->containers[lca];
		if (c.kind == htContainerGraph) {
			HTLayoutSeed s;
			s.edge = e;
			s.source_child = htree_layout_child_containing(ctx, lca, e->source);
			s.target_child = htree_layout_child_containing(ctx, lca, e->target);
			if (s.source_child < 0 && s.target_child < 0) continue;
			s.self_loop = s.source_child == s.target_child;
			s.has_label = e->label_rect && !s.self_loop &&
				c.node == e->source->parent && s.source_child >= 0;
			c.seeds.push_back(s);
		}
	}
}

/* The update: the flow direction of a drawn container is inferred from its
   transitions between two drawn children (right-running against
   down-running); the regions follow their composite; a container without
   such transitions takes the preferred frame of its depth, alternating from
   the inferred root */
static void htree_layout_count_runs(const HTLayoutContext* ctx, int ci, int* right, int* down)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	for (size_t k = 0; k < c.seeds.size(); k++) {
		const HTLayoutSeed& sd = c.seeds[k];
		HTreePoint a, b;
		double dx, dy;
		if (sd.self_loop || sd.source_child < 0 || sd.target_child < 0) continue;
		if (!htree_layout_seeded(ctx, c.children[sd.source_child], &a) ||
			!htree_layout_seeded(ctx, c.children[sd.target_child], &b)) continue;
		dx = b.x - a.x;
		dy = b.y - a.y;
		if (dx > std::fabs(dy)) (*right)++;
		else if (dy > std::fabs(dx)) (*down)++;
	}
}

static void htree_layout_infer_directions(HTLayoutContext* ctx)
{
	for (size_t ci = 0; ci < ctx->containers.size(); ci++) {
		HTLayoutContainer& c = ctx->containers[ci];
		int right = 0, down = 0;
		if (c.node && c.node->type == htRegion) continue;    /* set by the composite */
		if (c.kind == htContainerRegions) {
			for (HTreeNode* r = c.node->children; r; r = r->next) {
				std::map<const HTreeNode*, int>::const_iterator it;
				if (r->type != htRegion) continue;
				it = ctx->index.find(r);
				if (it != ctx->index.end()) htree_layout_count_runs(ctx, it->second, &right, &down);
			}
		} else {
			htree_layout_count_runs(ctx, (int)ci, &right, &down);
		}
		if (right > down) c.direction = htFlowRight;
		else if (down > right) c.direction = htFlowDown;
		else c.direction = htree_layout_preferred(&ctx->opts, ctx->start, c.level);
		if (ci == 0) ctx->start = c.direction;
		if (c.kind == htContainerRegions) {
			for (HTreeNode* r = c.node->children; r; r = r->next) {
				std::map<const HTreeNode*, int>::const_iterator it;
				if (r->type != htRegion) continue;
				it = ctx->index.find(r);
				if (it != ctx->index.end()) ctx->containers[it->second].direction = c.direction;
			}
		}
	}
}

/* -----------------------------------------------------------------------------
 * The layered layout of one graph container: P2 - P6
 * ----------------------------------------------------------------------------- */

static void htree_layout_build_graph(const HTLayoutContext* ctx, int ci, HTLayoutGraph* g)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	const HTLayoutOptions* o = &ctx->opts;
	size_t n = c.children.size();

	g->direction = c.direction;
	g->update = ctx->update;
	g->vertices.assign(n + 2, HTLayoutVertex());
	g->entry = 0;
	g->exit = (int)n + 1;
	g->last = 0;
	for (size_t k = 0; k < n + 2; k++) {
		HTLayoutVertex& v = g->vertices[k];
		v.node = NULL;
		v.role = htRoleNone;
		v.kind = htVertexVirtual;
		v.edge = -1;
		v.layer = 0;
		v.order = 0;
		v.flow_size = v.cross_size = 0.0;
		v.before = v.after = 0.0;
		v.flow = v.cross = 0.0;
		v.seed = 0;
		v.seed_x = v.seed_y = v.seed_flow = v.seed_cross = 0.0;
		v.seed_layer = 0;
		if (k >= 1 && k <= n) {
			HTreeNode* node = c.children[k - 1];
			HTreePoint centre;
			v.node = node;
			v.role = node->role;
			v.kind = htVertexReal;
			if (ctx->update && htree_layout_seeded(ctx, node, &centre)) {
				v.seed = 1;
				v.seed_x = centre.x;
				v.seed_y = centre.y;
			}
			if (node->rect) {
				htree_layout_size_to_frame(c.direction, node->rect->width, node->rect->height,
										   &v.flow_size, &v.cross_size);
			} else {
				v.flow_size = v.cross_size = o->point_size;
			}
			v.before = v.after = v.cross_size / 2.0;
		}
	}
	for (size_t k = 0; k < c.seeds.size(); k++) {
		const HTLayoutSeed& s = c.seeds[k];
		HTLayoutEdge e;
		int drawn_ends;
		if (s.self_loop) continue;
		drawn_ends = ctx->update && (s.source_child < 0 || g->vertices[s.source_child + 1].seed) &&
			(s.target_child < 0 || g->vertices[s.target_child + 1].seed);
		if (drawn_ends && (s.source_child < 0 || s.target_child < 0 ||
						   ctx->edge_seeds.find(s.edge) != ctx->edge_seeds.end())) {
			/* a drawn transition between drawn elements keeps its geometry
			   and takes no part in the seeded layout; a new one is laid out
			   unless both ends share a band (see htree_layout_graph_prepare) */
			continue;
		}
		e.source = s.source_child < 0 ? g->entry : s.source_child + 1;
		e.target = s.target_child < 0 ? g->exit : s.target_child + 1;
		e.edge = s.edge;
		e.reversed = 0;
		e.drawn_ends = drawn_ends;
		/* a new edge between drawn vertices takes no label layer: the slot
		   would push the drawn target band; its label goes beside the line */
		e.has_label = s.has_label && !drawn_ends;
		e.label_w = e.label_h = 0.0;
		if (s.has_label) {
			e.label_w = s.edge->label_rect->width > 0.0 ? s.edge->label_rect->width : o->label_width;
			e.label_h = s.edge->label_rect->height > 0.0 ? s.edge->label_rect->height : o->label_height;
		}
		g->edges.push_back(e);
	}
}

static void htree_layout_reverse_edge(HTLayoutEdge& e)
{
	int t = e.source;
	e.source = e.target;
	e.target = t;
	e.reversed = !e.reversed;
}

/* P2: the role pre-pass and the depth-first search in the model order;
   the back edges are reversed. Returns the topological order. In the
   update the drawn vertices of a band form one search node and the bands
   are chained in the drawn order, band 1 searched first: a new edge that
   re-enters the drawn flow at or before its own band is the back edge, so
   a new cycle never reverses the drawing */
static void htree_layout_break_cycles(HTLayoutGraph* g, std::vector<int>& topo)
{
	size_t n = g->vertices.size();
	int bands = 0;
	std::vector<int> node_of(n), roots;
	std::vector<std::pair<int, size_t> > stack;

	for (size_t k = 0; k < g->edges.size(); k++) {
		HTLayoutEdge& e = g->edges[k];
		const HTLayoutVertex& sv = g->vertices[e.source];
		const HTLayoutVertex& tv = g->vertices[e.target];
		if (g->update && sv.seed && tv.seed) {
			/* the drawing decides: a transition running backward is the back edge */
			if (tv.seed_flow < sv.seed_flow - HTREE_COORD_EPS) htree_layout_reverse_edge(e);
			continue;
		}
		if (g->update && (sv.seed || tv.seed)) continue;   /* the roles of the drawn vertices stay drawn */
		if ((htree_layout_source_role(tv.role) && e.source != g->entry) ||
			(htree_layout_sink_role(sv.role) && e.target != g->exit)) {
			htree_layout_reverse_edge(e);
		}
	}
	for (size_t v = 0; v < n; v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		node_of[v] = (int)v;
		if (g->update && vx.seed && vx.seed_layer > 0) {
			node_of[v] = (int)n + vx.seed_layer - 1;
			if (vx.seed_layer > bands) bands = vx.seed_layer;
		}
	}
	size_t N = n + (size_t)bands;
	std::vector<std::vector<int> > out(N), members(N);
	std::vector<int> colour(N, 0), indeg(N, 0);
	for (int b = 0; b + 1 < bands; b++) out[n + b].push_back(-1);   /* the chain to the next band */
	for (size_t v = 0; v < n; v++) if (node_of[v] != (int)v) members[node_of[v]].push_back((int)v);
	for (size_t k = 0; k < g->edges.size(); k++) {
		out[node_of[g->edges[k].source]].push_back((int)k);
		indeg[node_of[g->edges[k].target]]++;
	}
	if (bands > 0) roots.push_back((int)n);
	for (size_t v = 0; v < n; v++) if (htree_layout_source_role(g->vertices[v].role)) roots.push_back(node_of[v]);
	for (size_t v = 0; v < n; v++) if (indeg[node_of[v]] == 0) roots.push_back(node_of[v]);
	for (size_t v = 0; v < N; v++) roots.push_back((int)v);

	topo.clear();
	for (size_t r = 0; r < roots.size(); r++) {
		if (colour[roots[r]]) continue;
		stack.push_back(std::make_pair(roots[r], (size_t)0));
		colour[roots[r]] = 1;
		while (!stack.empty()) {
			int v = stack.back().first;
			size_t& i = stack.back().second;
			if (i < out[v].size()) {
				int k = out[v][i++];
				int w;
				if (k < 0) {
					w = v + 1;
					if (colour[w] == 0) {
						colour[w] = 1;
						stack.push_back(std::make_pair(w, (size_t)0));
					}
					continue;
				}
				HTLayoutEdge& e = g->edges[k];
				w = node_of[e.target];
				if (colour[w] == 0) {
					colour[w] = 1;
					stack.push_back(std::make_pair(w, (size_t)0));
				} else if (colour[w] == 1) {
					htree_layout_reverse_edge(e);
				}
			} else {
				colour[v] = 2;
				if (v >= (int)n) {
					for (size_t m = members[v].size(); m > 0; m--) topo.push_back(members[v][m - 1]);
				} else {
					topo.push_back(v);
				}
				stack.pop_back();
			}
		}
	}
	std::reverse(topo.begin(), topo.end());
}

/* the update: the drawn vertices sorted along the flow fall into bands - a
   new band starts where the next extent does not overlap the band so far */
static void htree_layout_seed_bands(HTLayoutGraph* g)
{
	std::vector<std::pair<double, int> > order;
	double band_end = 0.0;
	int band = 0;
	for (size_t v = 1; v + 1 < g->vertices.size(); v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		if (vx.kind == htVertexReal && vx.seed) {
			order.push_back(std::make_pair(vx.seed_flow - vx.flow_size / 2.0, (int)v));
		}
	}
	std::stable_sort(order.begin(), order.end());
	for (size_t i = 0; i < order.size(); i++) {
		HTLayoutVertex& vx = g->vertices[order[i].second];
		double start = order[i].first, end = vx.seed_flow + vx.flow_size / 2.0;
		if (band == 0 || start > band_end - HTREE_COORD_EPS) {
			band++;
			band_end = end;
		} else if (end > band_end) {
			band_end = end;
		}
		vx.seed_layer = band;
	}
}

/* P3: the longest-path layering with the pinned roles; in the update the
   seed bands are the lower bounds and stay whole and ordered */
static void htree_layout_assign_layers(HTLayoutGraph* g, const std::vector<int>& topo)
{
	size_t n = g->vertices.size();
	std::vector<std::vector<int> > in(n);
	int has_source = 0, max_layer = 0, sink_layer, exit_layer, bands = 0;

	for (size_t k = 0; k < g->edges.size(); k++) {
		in[g->edges[k].target].push_back((int)k);
	}
	for (size_t v = 1; v < n; v++) {
		if (g->vertices[v].kind == htVertexReal && htree_layout_source_role(g->vertices[v].role) &&
			!(g->update && g->vertices[v].seed)) has_source = 1;
	}
	if (g->update) {
		/* the bands come from htree_layout_graph_prepare */
		for (size_t v = 1; v + 1 < n; v++) if (g->vertices[v].seed_layer > bands) bands = g->vertices[v].seed_layer;
	}
	std::vector<int> band_layer(bands + 1);
	for (int b = 0; b <= bands; b++) band_layer[b] = b;
	for (int round = 0; round <= bands; round++) {
		int changed = 0;
		for (size_t t = 0; t < topo.size(); t++) {
			int v = topo[t];
			HTLayoutVertex& vx = g->vertices[v];
			int layer;
			if (v == g->entry) {
				layer = 0;
			} else if (v == g->exit) {
				layer = 1;
			} else if (vx.seed) {
				layer = band_layer[vx.seed_layer];
			} else {
				layer = (has_source && !htree_layout_source_role(vx.role)) ? 2 : 1;
			}
			for (size_t k = 0; k < in[v].size(); k++) {
				const HTLayoutEdge& e = g->edges[in[v][k]];
				int need = g->vertices[e.source].layer + (e.has_label ? 2 : 1);
				if (need > layer) layer = need;
			}
			vx.layer = layer;
		}
		if (!g->update) break;
		/* a pushed band drags the later bands along, as whole bands */
		for (int b = 1, floor = 0; b <= bands; b++) {
			int top = 0;
			for (size_t v = 1; v + 1 < n; v++) {
				const HTLayoutVertex& vx = g->vertices[v];
				if (vx.seed_layer == b && vx.layer > top) top = vx.layer;
			}
			if (top < floor + 1) top = floor + 1;
			if (band_layer[b] != top) changed = 1;
			band_layer[b] = top;
			for (size_t v = 1; v + 1 < n; v++) {
				HTLayoutVertex& vx = g->vertices[v];
				if (vx.seed_layer == b) vx.layer = top;
			}
			floor = top;
		}
		if (!changed) break;
	}
	/* the sinks close the flow in a common last layer (the drawn ones stay) */
	sink_layer = 0;
	for (size_t v = 1; v + 1 < n; v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		if (htree_layout_sink_role(vx.role) && !(g->update && vx.seed)) {
			if (vx.layer > sink_layer) sink_layer = vx.layer;
		} else if (vx.layer > max_layer) {
			max_layer = vx.layer;
		}
	}
	if (sink_layer > 0 && sink_layer < max_layer + 1) sink_layer = max_layer + 1;
	for (size_t v = 1; v + 1 < n; v++) {
		HTLayoutVertex& vx = g->vertices[v];
		if (htree_layout_sink_role(vx.role) && !(g->update && vx.seed)) vx.layer = sink_layer;
	}
	g->last = sink_layer > max_layer ? sink_layer : max_layer;
	exit_layer = g->last + 1;
	for (size_t k = 0; k < in[g->exit].size(); k++) {
		const HTLayoutEdge& e = g->edges[in[g->exit][k]];
		int need = g->vertices[e.source].layer + (e.has_label ? 2 : 1);
		if (need > exit_layer) exit_layer = need;
	}
	g->vertices[g->exit].layer = exit_layer;
	g->last = exit_layer - 1;
}

static void htree_layout_reindex(HTLayoutGraph* g, int l);

static int htree_layout_add_dummy(HTLayoutGraph* g, HTLayoutVertexKind kind, int edge, int layer,
								  double flow_size, double before, double after)
{
	HTLayoutVertex v;
	v.node = NULL;
	v.role = htRoleNone;
	v.kind = kind;
	v.edge = edge;
	v.layer = layer;
	v.order = 0;
	v.flow_size = flow_size;
	v.cross_size = before + after;
	v.before = before;
	v.after = after;
	v.flow = v.cross = 0.0;
	v.seed = 0;
	v.seed_x = v.seed_y = v.seed_flow = v.seed_cross = 0.0;
	v.seed_layer = 0;
	g->vertices.push_back(v);
	return (int)g->vertices.size() - 1;
}

/* P4: the proper layering with the edge and label dummies */
static void htree_layout_insert_dummies(HTLayoutGraph* g, const HTLayoutOptions* o)
{
	for (size_t k = 0; k < g->edges.size(); k++) {
		HTLayoutEdge& e = g->edges[k];
		int ls = g->vertices[e.source].layer, lt = g->vertices[e.target].layer;
		int label_layer = -1;
		e.chain.clear();
		e.chain.push_back(e.source);
		if (e.has_label) {
			label_layer = e.reversed ? lt - 1 : ls + 1;
		}
		for (int l = ls + 1; l < lt; l++) {
			if (l == label_layer) {
				/* the slot: the line at the anchor, the label after it */
				double fs, cs;
				if (g->direction == htFlowDown) { fs = e.label_h; cs = e.label_w; }
				else { fs = e.label_w; cs = e.label_h; }
				e.chain.push_back(htree_layout_add_dummy(g, htVertexLabelDummy, (int)k, l, fs,
														 o->padding / 2.0, cs + o->padding / 2.0));
			} else {
				e.chain.push_back(htree_layout_add_dummy(g, htVertexEdgeDummy, (int)k, l, 0.0, 0.0, 0.0));
			}
		}
		e.chain.push_back(e.target);
	}
	g->layers.assign(g->last + 2, std::vector<int>());
	for (size_t v = 0; v < g->vertices.size(); v++) {
		std::vector<int>& L = g->layers[g->vertices[v].layer];
		g->vertices[v].order = (int)L.size();
		L.push_back((int)v);
	}
	if (g->update) {
		/* the drawn vertices by their drawn cross position, the others after */
		for (int l = 0; l <= g->last + 1; l++) {
			std::vector<int>& L = g->layers[l];
			std::vector<std::pair<double, int> > keyed;
			for (size_t i = 0; i < L.size(); i++) {
				const HTLayoutVertex& vx = g->vertices[L[i]];
				keyed.push_back(std::make_pair(vx.seed ? vx.seed_cross : 1e300, L[i]));
			}
			std::stable_sort(keyed.begin(), keyed.end());
			for (size_t i = 0; i < L.size(); i++) L[i] = keyed[i].second;
			htree_layout_reindex(g, l);
		}
	}
	g->up.assign(g->vertices.size(), std::vector<int>());
	g->down.assign(g->vertices.size(), std::vector<int>());
	for (size_t k = 0; k < g->edges.size(); k++) {
		const std::vector<int>& ch = g->edges[k].chain;
		for (size_t i = 0; i + 1 < ch.size(); i++) {
			g->down[ch[i]].push_back(ch[i + 1]);
			g->up[ch[i + 1]].push_back(ch[i]);
		}
	}
}

/* P5: the barycentre ordering */

static void htree_layout_reindex(HTLayoutGraph* g, int l)
{
	const std::vector<int>& L = g->layers[l];
	for (size_t i = 0; i < L.size(); i++) g->vertices[L[i]].order = (int)i;
}

/* the position of a fixed-layer vertex; a virtual vertex reports the centre
   of the layer being sorted */
static double htree_layout_position(const HTLayoutGraph* g, int v, size_t ref_size)
{
	if (g->vertices[v].kind == htVertexVirtual) return (ref_size - 1) / 2.0;
	return g->vertices[v].order;
}

static void htree_layout_barycentres(const HTLayoutGraph* g, int l, int fixed_up, std::vector<double>& b)
{
	const std::vector<int>& L = g->layers[l];
	b.assign(L.size(), 0.0);
	for (size_t i = 0; i < L.size(); i++) {
		const std::vector<int>& nb = fixed_up ? g->up[L[i]] : g->down[L[i]];
		if (nb.empty() || (g->update && g->vertices[L[i]].seed)) {
			b[i] = g->vertices[L[i]].order;
		} else {
			double sum = 0.0;
			for (size_t j = 0; j < nb.size(); j++) sum += htree_layout_position(g, nb[j], L.size());
			b[i] = sum / nb.size();
		}
	}
}

static int htree_layout_sort_layer(HTLayoutGraph* g, int l, int fixed_up)
{
	std::vector<int>& L = g->layers[l];
	std::vector<double> b;
	std::vector<std::pair<double, int> > keyed;
	int changed = 0;

	if (L.size() < 2) return 0;
	htree_layout_barycentres(g, l, fixed_up, b);
	for (size_t i = 0; i < L.size(); i++) keyed.push_back(std::make_pair(b[i], L[i]));
	std::stable_sort(keyed.begin(), keyed.end(),
					 [](const std::pair<double, int>& a, const std::pair<double, int>& c) {
						 return a.first < c.first;
					 });
	for (size_t i = 0; i < L.size(); i++) {
		if (L[i] != keyed[i].second) changed = 1;
		L[i] = keyed[i].second;
	}
	htree_layout_reindex(g, l);
	return changed;
}

static long htree_layout_count_crossings(const HTLayoutGraph* g)
{
	long total = 0;
	for (int l = 0; l <= g->last; l++) {
		std::vector<std::pair<int, int> > p;
		const std::vector<int>& L = g->layers[l];
		for (size_t i = 0; i < L.size(); i++) {
			const std::vector<int>& nb = g->down[L[i]];
			for (size_t j = 0; j < nb.size(); j++) {
				p.push_back(std::make_pair(g->vertices[L[i]].order, g->vertices[nb[j]].order));
			}
		}
		for (size_t a = 0; a < p.size(); a++) {
			for (size_t c = a + 1; c < p.size(); c++) {
				if ((p[a].first < p[c].first && p[a].second > p[c].second) ||
					(p[a].first > p[c].first && p[a].second < p[c].second)) total++;
			}
		}
	}
	return total;
}

typedef struct {
	std::vector<std::vector<int> > layers;
	long                            crossings;
} HTLayoutBest;

static void htree_layout_keep_best(HTLayoutGraph* g, HTLayoutBest* best)
{
	long k = htree_layout_count_crossings(g);
	if (k < best->crossings) {
		best->crossings = k;
		best->layers = g->layers;
	}
}

/* phase 1: the DOWN / UP sweeps, the best order kept aside */
static void htree_layout_order_phase1(HTLayoutGraph* g, int sweeps, HTLayoutBest* best)
{
	for (int s = 0; s < sweeps && best->crossings > 0; s++) {
		int changed = 0;
		for (int l = 1; l <= g->last + 1; l++) changed |= htree_layout_sort_layer(g, l, 1);
		for (int l = g->last; l >= 0; l--) changed |= htree_layout_sort_layer(g, l, 0);
		htree_layout_keep_best(g, best);
		if (!changed) break;
	}
}

/* phase 2: reverse the blocks of equal barycentres, then sweep again */
static int htree_layout_order_phase2(HTLayoutGraph* g)
{
	int any = 0;
	for (int l = 1; l <= g->last; l++) {
		std::vector<int>& L = g->layers[l];
		std::vector<double> b;
		if (L.size() < 2) continue;
		htree_layout_barycentres(g, l, 1, b);
		for (size_t i = 0; i < L.size();) {
			size_t j = i + 1;
			int drawn = g->update && g->vertices[L[i]].seed;
			while (j < L.size() && std::fabs(b[j] - b[i]) < HTREE_COORD_EPS) {
				if (g->update && g->vertices[L[j]].seed) drawn = 1;
				j++;
			}
			if (j - i > 1 && !drawn) {
				std::reverse(L.begin() + i, L.begin() + j);
				any = 1;
			}
			i = j;
		}
		htree_layout_reindex(g, l);
	}
	return any;
}

static void htree_layout_order(HTLayoutGraph* g, const HTLayoutOptions* o)
{
	HTLayoutBest best;
	best.layers = g->layers;
	best.crossings = htree_layout_count_crossings(g);
	htree_layout_order_phase1(g, o->sweeps, &best);
	for (int r = 0; r < o->sweeps && best.crossings > 0; r++) {
		if (!htree_layout_order_phase2(g)) break;
		htree_layout_keep_best(g, &best);
		htree_layout_order_phase1(g, o->sweeps, &best);
	}
	g->layers = best.layers;
	for (int l = 0; l <= g->last + 1; l++) htree_layout_reindex(g, l);
}

/* P6: the priority layout on the cross axis, the bands on the flow axis */

static double htree_layout_gap(const HTLayoutGraph* g, const HTLayoutOptions* o, int u, int v)
{
	if (g->vertices[u].kind == htVertexEdgeDummy && g->vertices[v].kind == htVertexEdgeDummy) {
		return o->padding;
	}
	if (g->update && g->vertices[u].seed && g->vertices[v].seed) {
		return 0.0;   /* the drawn neighbours keep their drawn gap */
	}
	return o->node_gap;
}

static void htree_layout_pack_layer(HTLayoutGraph* g, const HTLayoutOptions* o, int l)
{
	const std::vector<int>& L = g->layers[l];
	double pos = 0.0;
	for (size_t i = 0; i < L.size(); i++) {
		HTLayoutVertex& v = g->vertices[L[i]];
		if (i > 0) pos += htree_layout_gap(g, o, L[i - 1], L[i]) + v.before;
		else pos = v.before;
		if (g->update && v.seed && v.seed_cross > pos) pos = v.seed_cross;
		v.cross = pos;
		pos += v.after;
	}
}

/* the virtual vertices sit at the centre of the content */
static void htree_layout_centre_virtual(HTLayoutGraph* g)
{
	double lo = 0.0, hi = 0.0;
	int any = 0;
	for (size_t v = 0; v < g->vertices.size(); v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		if (vx.kind == htVertexVirtual) continue;
		double a = vx.cross - vx.before, b = vx.cross + vx.after;
		if (!any || a < lo) lo = a;
		if (!any || b > hi) hi = b;
		any = 1;
	}
	g->vertices[g->entry].cross = (lo + hi) / 2.0;
	g->vertices[g->exit].cross = (lo + hi) / 2.0;
}

static double htree_layout_block_mean(const std::vector<double>& sum, const std::vector<double>& w,
									  const std::vector<double>& dsum, const std::vector<size_t>& dn,
									  size_t k)
{
	return dn[k] > 0 ? dsum[k] / (double)dn[k] : sum[k] / w[k];
}

/* the priority pass of a layer as the weighted least-squares placement:
   every vertex aims at the mean position of its neighbours in the fixed
   layer with the weight of its priority (the dummies above every
   connectivity, the unconnected vertices barely weighted at their place);
   the layer order and the gaps are kept by pooling the adjacent violators */
static void htree_layout_priority_pass(HTLayoutGraph* g, const HTLayoutOptions* o, int l, int fixed_up)
{
	std::vector<int>& L = g->layers[l];
	size_t n = L.size();
	std::vector<double> off(n, 0.0), d(n, 0.0), w(n, 0.0);
	std::vector<double> bsum, bw, bdsum;
	std::vector<size_t> bstart, bdn;
	double top = (double)g->vertices.size() + 1.0;

	if (n == 0) return;
	htree_layout_centre_virtual(g);
	for (size_t i = 0; i < n; i++) {
		const HTLayoutVertex& v = g->vertices[L[i]];
		const std::vector<int>& nb = fixed_up ? g->up[L[i]] : g->down[L[i]];
		if (i > 0) {
			off[i] = off[i - 1] + g->vertices[L[i - 1]].after +
				htree_layout_gap(g, o, L[i - 1], L[i]) + v.before;
		}
		if (g->update && v.seed) {
			/* a drawn vertex aims at its drawn place above every pull */
			d[i] = v.seed_cross;
			w[i] = top * top;
		} else if (nb.empty()) {
			d[i] = v.cross;
			w[i] = HTREE_COORD_EPS;
		} else {
			double sum = 0.0;
			for (size_t k = 0; k < nb.size(); k++) sum += g->vertices[nb[k]].cross;
			d[i] = sum / nb.size();
			w[i] = v.kind == htVertexReal ? (double)nb.size() : top;
		}
		d[i] -= off[i];
	}
	/* a block holding drawn vertices takes their mean: the drawn vertices
	   stay pinned, the free ones yield (a weight alone still moves them) */
	for (size_t i = 0; i < n; i++) {
		int drawn = g->update && g->vertices[L[i]].seed;
		bsum.push_back(w[i] * d[i]);
		bw.push_back(w[i]);
		bdsum.push_back(drawn ? d[i] : 0.0);
		bdn.push_back(drawn ? 1 : 0);
		bstart.push_back(i);
		while (bsum.size() > 1 &&
			   htree_layout_block_mean(bsum, bw, bdsum, bdn, bsum.size() - 1) <
			   htree_layout_block_mean(bsum, bw, bdsum, bdn, bsum.size() - 2)) {
			bsum[bsum.size() - 2] += bsum.back();
			bw[bw.size() - 2] += bw.back();
			bdsum[bdsum.size() - 2] += bdsum.back();
			bdn[bdn.size() - 2] += bdn.back();
			bsum.pop_back();
			bw.pop_back();
			bdsum.pop_back();
			bdn.pop_back();
			bstart.pop_back();
		}
	}
	for (size_t k = 0; k < bstart.size(); k++) {
		size_t end = k + 1 < bstart.size() ? bstart[k + 1] : n;
		double m = htree_layout_block_mean(bsum, bw, bdsum, bdn, k);
		for (size_t i = bstart[k]; i < end; i++) g->vertices[L[i]].cross = m + off[i];
	}
}

static void htree_layout_assign_coordinates(HTLayoutGraph* g, const HTLayoutOptions* o)
{
	int t = (g->last + 1) / 2;
	double start = 0.0;

	for (int l = 1; l <= g->last; l++) htree_layout_pack_layer(g, o, l);
	for (int l = 1; l <= g->last; l++) htree_layout_priority_pass(g, o, l, 1);
	for (int l = g->last - 1; l >= 1; l--) htree_layout_priority_pass(g, o, l, 0);
	for (int l = t < 1 ? 1 : t; l <= g->last; l++) htree_layout_priority_pass(g, o, l, 1);

	if (g->update) {
		/* the drawn bands keep their drawn flow positions, shifted as a whole
		   only to make room for an inserted band; a new band takes the room
		   of its tallest vertex */
		double cursor = 0.0, shift = 0.0;
		int any = 0, after_new = 0;
		g->band_lo.assign(g->last + 2, 0.0);
		g->band_hi.assign(g->last + 2, 0.0);
		g->band_drawn.assign(g->last + 2, 0);
		for (int l = 1; l <= g->last; l++) {
			const std::vector<int>& L = g->layers[l];
			double lo = 0.0, hi = 0.0, tall = 0.0;
			int seeded = 0;
			for (size_t i = 0; i < L.size(); i++) {
				const HTLayoutVertex& vx = g->vertices[L[i]];
				if (vx.seed) {
					double a = vx.seed_flow - vx.flow_size / 2.0, b = vx.seed_flow + vx.flow_size / 2.0;
					if (!seeded || a < lo) lo = a;
					if (!seeded || b > hi) hi = b;
					seeded = 1;
				} else if (vx.flow_size > tall) {
					tall = vx.flow_size;
				}
			}
			if (seeded) {
				double need = any ? cursor + (after_new ? o->layer_gap : 0.0) : lo + shift;
				if (lo + shift < need) shift = need - lo;
				lo += shift;
				hi += shift;
				if (hi - lo < tall) hi = lo + tall;
				for (size_t i = 0; i < L.size(); i++) {
					HTLayoutVertex& vx = g->vertices[L[i]];
					vx.flow = vx.seed ? vx.seed_flow + shift : (lo + hi) / 2.0;
				}
				g->band_lo[l] = lo;
				g->band_hi[l] = hi;
				g->band_drawn[l] = 1;
				cursor = hi;
				after_new = 0;
			} else {
				double from = any ? cursor + o->layer_gap : 0.0;
				for (size_t i = 0; i < L.size(); i++) g->vertices[L[i]].flow = from + tall / 2.0;
				cursor = from + tall;
				after_new = 1;
			}
			any = 1;
		}
		g->vertices[g->entry].flow = 0.0;
		g->vertices[g->exit].flow = cursor;
		return;
	}
	for (int l = 1; l <= g->last; l++) {
		const std::vector<int>& L = g->layers[l];
		double band = 0.0;
		for (size_t i = 0; i < L.size(); i++) {
			if (g->vertices[L[i]].flow_size > band) band = g->vertices[L[i]].flow_size;
		}
		for (size_t i = 0; i < L.size(); i++) g->vertices[L[i]].flow = start + band / 2.0;
		start += band + o->layer_gap;
	}
	g->vertices[g->entry].flow = 0.0;
	g->vertices[g->exit].flow = start > o->layer_gap ? start - o->layer_gap : 0.0;
}

/* the content extent of the placed vertices in the frame, moved to the origin */
static void htree_layout_normalise(HTLayoutGraph* g, double* flow_extent, double* cross_extent)
{
	double f0 = 0.0, f1 = 0.0, c0 = 0.0, c1 = 0.0;
	int any = 0;
	for (size_t v = 0; v < g->vertices.size(); v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		if (vx.kind == htVertexVirtual) continue;
		double fa = vx.flow - vx.flow_size / 2.0, fb = vx.flow + vx.flow_size / 2.0;
		double ca = vx.cross - vx.before, cb = vx.cross + vx.after;
		if (!any || fa < f0) f0 = fa;
		if (!any || fb > f1) f1 = fb;
		if (!any || ca < c0) c0 = ca;
		if (!any || cb > c1) c1 = cb;
		any = 1;
	}
	for (size_t v = 0; v < g->vertices.size(); v++) {
		g->vertices[v].flow -= f0;
		g->vertices[v].cross -= c0;
	}
	for (size_t l = 0; l < g->band_drawn.size(); l++) {
		g->band_lo[l] -= f0;
		g->band_hi[l] -= f0;
	}
	*flow_extent = any ? f1 - f0 : 0.0;
	*cross_extent = any ? c1 - c0 : 0.0;
}

/* place the children and record the edge pieces in the content coordinates */
static void htree_layout_apply_graph(HTLayoutContext* ctx, int ci, HTLayoutGraph* g)
{
	HTLayoutContainer& c = ctx->containers[ci];
	const HTLayoutOptions* o = &ctx->opts;

	for (size_t v = 0; v < g->vertices.size(); v++) {
		HTLayoutVertex& vx = g->vertices[v];
		double x, y;
		if (vx.kind != htVertexReal) continue;
		htree_layout_to_doc(g->direction, vx.flow, vx.cross, &x, &y);
		if (vx.node->rect) {
			htree_shift_subtree(vx.node, x - vx.node->rect->width / 2.0 - vx.node->rect->x,
								y - vx.node->rect->height / 2.0 - vx.node->rect->y);
		} else {
			if (!vx.node->point) vx.node->point = htree_new_point();
			vx.node->point->x = x;
			vx.node->point->y = y;
		}
	}
	for (size_t k = 0; k < g->edges.size(); k++) {
		const HTLayoutEdge& e = g->edges[k];
		HTLayoutPiece piece;
		piece.edge = e.edge;
		piece.has_label = 0;
		htree_init_rect(&piece.label);
		for (size_t i = 1; i + 1 < e.chain.size(); i++) {
			const HTLayoutVertex& d = g->vertices[e.chain[i]];
			HTreePoint p;
			if (d.kind == htVertexLabelDummy) {
				/* the line runs through the anchor, the label fills the slot after it */
				piece.has_label = 1;
				if (g->direction == htFlowDown) {
					piece.label.x = d.cross + o->padding / 2.0;
					piece.label.y = d.flow - e.label_h / 2.0;
				} else {
					piece.label.x = d.flow - e.label_w / 2.0;
					piece.label.y = d.cross + o->padding / 2.0;
				}
				piece.label.width = e.label_w;
				piece.label.height = e.label_h;
			}
			if (d.layer < (int)g->band_drawn.size() && g->band_drawn[d.layer]) {
				/* the update: in a drawn band the line runs along the whole
				   band, so its ends clear the drawn neighbours */
				htree_layout_to_doc(g->direction, g->band_lo[d.layer], d.cross, &p.x, &p.y);
				piece.bends.push_back(p);
				htree_layout_to_doc(g->direction, g->band_hi[d.layer], d.cross, &p.x, &p.y);
				piece.bends.push_back(p);
				continue;
			}
			htree_layout_to_doc(g->direction, d.flow, d.cross, &p.x, &p.y);
			piece.bends.push_back(p);
		}
		if (e.reversed) std::reverse(piece.bends.begin(), piece.bends.end());
		c.pieces.push_back(piece);
	}
}

/* P2 - P5: the direction-independent phases */
static void htree_layout_graph_prepare(const HTLayoutContext* ctx, int ci, HTLayoutGraph* g)
{
	std::vector<int> topo;
	htree_layout_build_graph(ctx, ci, g);
	for (size_t v = 0; v < g->vertices.size(); v++) {
		HTLayoutVertex& vx = g->vertices[v];
		if (!vx.seed) continue;
		if (g->direction == htFlowDown) { vx.seed_flow = vx.seed_y; vx.seed_cross = vx.seed_x; }
		else { vx.seed_flow = vx.seed_x; vx.seed_cross = vx.seed_y; }
	}
	if (g->update) {
		/* the bands before the cycles: a new edge inside one band would
		   push a drawn vertex out of it, so it keeps a straight line */
		std::vector<HTLayoutEdge> kept;
		htree_layout_seed_bands(g);
		for (size_t k = 0; k < g->edges.size(); k++) {
			const HTLayoutEdge& e = g->edges[k];
			if (e.drawn_ends && g->vertices[e.source].seed_layer == g->vertices[e.target].seed_layer) continue;
			kept.push_back(e);
		}
		g->edges.swap(kept);
	}
	htree_layout_break_cycles(g, topo);
	htree_layout_assign_layers(g, topo);
	htree_layout_insert_dummies(g, &ctx->opts);
	htree_layout_order(g, &ctx->opts);
}

/* P6 in one frame: the vertex extents follow the direction, then the
   coordinates; the content extents come back in the document axes */
static void htree_layout_graph_frame(const HTLayoutContext* ctx, const HTLayoutGraph* base,
									 HTFlowDirection dir, HTLayoutGraph* g, double* cw, double* ch)
{
	const HTLayoutOptions* o = &ctx->opts;
	double flow_extent, cross_extent;

	*g = *base;
	g->direction = dir;
	for (size_t v = 0; v < g->vertices.size(); v++) {
		HTLayoutVertex& vx = g->vertices[v];
		if (vx.seed) {
			if (dir == htFlowDown) { vx.seed_flow = vx.seed_y; vx.seed_cross = vx.seed_x; }
			else { vx.seed_flow = vx.seed_x; vx.seed_cross = vx.seed_y; }
		}
		if (vx.kind == htVertexReal && vx.node && vx.node->rect) {
			htree_layout_size_to_frame(dir, vx.node->rect->width, vx.node->rect->height,
									   &vx.flow_size, &vx.cross_size);
			vx.before = vx.after = vx.cross_size / 2.0;
		} else if (vx.kind == htVertexLabelDummy && vx.edge >= 0) {
			const HTLayoutEdge& e = g->edges[vx.edge];
			double cs;
			if (dir == htFlowDown) { vx.flow_size = e.label_h; cs = e.label_w; }
			else { vx.flow_size = e.label_w; cs = e.label_h; }
			vx.before = o->padding / 2.0;
			vx.after = cs + o->padding / 2.0;
			vx.cross_size = vx.before + vx.after;
		}
	}
	htree_layout_assign_coordinates(g, o);
	htree_layout_normalise(g, &flow_extent, &cross_extent);
	if (dir == htFlowDown) { *cw = cross_extent; *ch = flow_extent; }
	else { *cw = flow_extent; *ch = cross_extent; }
}

/* the graph content laid out in the given frame, or in the frame chosen by
   the content in the adaptive mode (dir < 0) */
static int htree_layout_graph(HTLayoutContext* ctx, int ci, int dir, double* content_w, double* content_h)
{
	HTLayoutGraph base, g;
	HTFlowDirection chosen;

	htree_layout_graph_prepare(ctx, ci, &base);
	if (dir >= 0) {
		chosen = (HTFlowDirection)dir;
	} else if (ctx->opts.mode == htLayoutAdaptive && !ctx->update) {
		HTLayoutGraph gd, gr;
		double wd, hd, wr, hr;
		HTFlowDirection preferred = ctx->containers[ci].direction;
		htree_layout_graph_frame(ctx, &base, htFlowDown, &gd, &wd, &hd);
		htree_layout_graph_frame(ctx, &base, htFlowRight, &gr, &wr, &hr);
		chosen = htree_layout_choose(wd, hd, wr, hr, htree_layout_target(&ctx->opts, preferred), preferred);
	} else {
		chosen = ctx->containers[ci].direction;
	}
	htree_layout_graph_frame(ctx, &base, chosen, &g, content_w, content_h);
	ctx->containers[ci].direction = chosen;
	htree_layout_apply_graph(ctx, ci, &g);
	return HTREE_OK;
}

/* the extents of a graph container in a frame, no geometry written */
static void htree_layout_graph_extents(const HTLayoutContext* ctx, int ci, HTFlowDirection dir,
									   double* cw, double* ch)
{
	HTLayoutGraph base, g;
	htree_layout_graph_prepare(ctx, ci, &base);
	htree_layout_graph_frame(ctx, &base, dir, &g, cw, ch);
}

/* -----------------------------------------------------------------------------
 * The containers: P1, P7, P8 and the border points
 * ----------------------------------------------------------------------------- */

static void htree_layout_size_leaf(const HTLayoutContext* ctx, HTreeNode* node)
{
	if (node->type == htPoint || node->rect) return;
	node->rect = htree_new_rect();
	node->rect->x = node->rect->y = 0.0;
	node->rect->width = node->min_rect ? node->min_rect->width : ctx->opts.node_width;
	node->rect->height = node->min_rect ? node->min_rect->height : ctx->opts.node_height;
}

/* P8: the container rect around the content, the title block on top */
static void htree_layout_fit(const HTLayoutContext* ctx, const HTreeNode* node, double cw, double ch,
							 double* W, double* H, double* inset_x, double* inset_y)
{
	const HTLayoutOptions* o = &ctx->opts;
	double min_w = node && node->min_rect ? node->min_rect->width : 0.0;
	double min_h = node && node->min_rect ? node->min_rect->height : 0.0;
	double title = node && node->min_rect ? node->min_rect->height : o->padding;

	if (cw <= 0.0 && ch <= 0.0) {
		*W = min_w > o->node_width ? min_w : o->node_width;
		*H = min_h > o->node_height ? min_h : o->node_height;
		*inset_x = o->padding;
		*inset_y = title;
		return;
	}
	*W = cw + 2 * o->padding;
	if (min_w > *W) *W = min_w;
	*H = title + o->padding + ch + o->padding;
	*inset_x = o->padding + (*W - 2 * o->padding - cw) / 2.0;
	*inset_y = title + o->padding;
}

/* the fitted size of a region laid out in a frame, no geometry written */
static void htree_layout_region_size(const HTLayoutContext* ctx, int ri, HTFlowDirection dir,
									 double* W, double* H)
{
	double cw = 0.0, ch = 0.0, ix, iy;
	if (ctx->containers[ri].kind == htContainerGraph) {
		htree_layout_graph_extents(ctx, ri, dir, &cw, &ch);
	}
	htree_layout_fit(ctx, ctx->containers[ri].node, cw, ch, W, H, &ix, &iy);
}

/* the extents of the regions of a composite stacked across the flow axis */
static void htree_layout_stack_extents(const HTLayoutContext* ctx, int ci, HTFlowDirection dir,
									   double* cw, double* ch)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	double along = 0.0, across = 0.0;
	int n = 0;
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		std::map<const HTreeNode*, int>::const_iterator it;
		double W, H;
		if (r->type != htRegion) continue;
		it = ctx->index.find(r);
		if (it == ctx->index.end()) continue;
		htree_layout_region_size(ctx, it->second, dir, &W, &H);
		if (dir == htFlowDown) { along += W; if (H > across) across = H; }
		else { along += H; if (W > across) across = W; }
		n++;
	}
	if (n > 1) along += (n - 1) * ctx->opts.node_gap;
	if (dir == htFlowDown) { *cw = along; *ch = across; }
	else { *cw = across; *ch = along; }
}

static int htree_layout_container_forced(HTLayoutContext* ctx, int ci, HTFlowDirection dir);
static int htree_layout_container(HTLayoutContext* ctx, int ci);
static void htree_layout_content_bounds(const HTLayoutContext* ctx, int ci, HTreeRect* r);
static void htree_layout_node_centre(const HTreeNode* n, HTreePoint* p);
static void htree_layout_origin(const HTLayoutContext* ctx, int ci, double* ox, double* oy);

/* the update: a region of the composite is drawn - seeded itself or holding
   a seeded vertex */
static int htree_layout_region_drawn(const HTLayoutContext* ctx, const HTreeNode* r)
{
	if (htree_layout_seeded(ctx, r, NULL)) return 1;
	for (HTreeNode* v = r->children; v; v = v->next) {
		if (htree_layout_is_vertex(v) && htree_layout_seeded(ctx, v, NULL)) return 1;
	}
	return 0;
}

static int htree_layout_regions_drawn(const HTLayoutContext* ctx, int ci)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	if (!ctx->update || c.kind != htContainerRegions) return 0;
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		if (r->type == htRegion && htree_layout_region_drawn(ctx, r)) return 1;
	}
	return 0;
}

static void htree_layout_union(HTreeRect* u, const HTreeRect* r, int* any)
{
	double x2, y2;
	if (!*any) { *u = *r; *any = 1; return; }
	x2 = u->x + u->width > r->x + r->width ? u->x + u->width : r->x + r->width;
	y2 = u->y + u->height > r->y + r->height ? u->y + u->height : r->y + r->height;
	if (r->x < u->x) u->x = r->x;
	if (r->y < u->y) u->y = r->y;
	u->width = x2 - u->x;
	u->height = y2 - u->y;
}

/* P7: the regions laid out in the composite's frame, side by side across
   the flow axis, widened to the widest */
static int htree_layout_regions(HTLayoutContext* ctx, int ci, double* content_w, double* content_h)
{
	HTLayoutContainer& c = ctx->containers[ci];
	HTFlowDirection dir = c.direction;
	double max_flow = 0.0, cursor = 0.0, w = 0.0, h = 0.0;

	if (ctx->opts.mode == htLayoutAdaptive && !ctx->update) {
		double wd, hd, wr, hr;
		htree_layout_stack_extents(ctx, ci, htFlowDown, &wd, &hd);
		htree_layout_stack_extents(ctx, ci, htFlowRight, &wr, &hr);
		dir = htree_layout_choose(wd, hd, wr, hr, htree_layout_target(&ctx->opts, c.direction), c.direction);
		c.direction = dir;
	}
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		std::map<const HTreeNode*, int>::const_iterator it;
		int res;
		if (r->type != htRegion) continue;
		it = ctx->index.find(r);
		if (it == ctx->index.end()) return HTREE_BAD_PARAMETER;
		res = htree_layout_container_forced(ctx, it->second, dir);
		if (res != HTREE_OK) return res;
	}
	if (htree_layout_regions_drawn(ctx, ci)) {
		/* the drawn regions - drawn themselves or holding drawn vertices, an
		   implicit region of a drawn composite too - keep their places; a
		   new region goes after them */
		HTreeRect u;
		int any = 0;
		htree_init_rect(&u);
		for (HTreeNode* r = c.node->children; r; r = r->next) {
			if (r->type == htRegion && r->rect && htree_layout_region_drawn(ctx, r)) {
				htree_layout_union(&u, r->rect, &any);
			}
		}
		for (HTreeNode* r = c.node->children; r; r = r->next) {
			if (r->type != htRegion || !r->rect || htree_layout_region_drawn(ctx, r)) continue;
			if (dir == htFlowDown) htree_shift_subtree(r, u.x + u.width + ctx->opts.node_gap - r->rect->x, u.y - r->rect->y);
			else htree_shift_subtree(r, u.x - r->rect->x, u.y + u.height + ctx->opts.node_gap - r->rect->y);
			htree_layout_union(&u, r->rect, &any);
		}
		*content_w = u.width;
		*content_h = u.height;
		return HTREE_OK;
	}
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		if (r->type != htRegion || !r->rect) continue;
		double f = dir == htFlowDown ? r->rect->height : r->rect->width;
		if (f > max_flow) max_flow = f;
	}
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		if (r->type != htRegion || !r->rect) continue;
		if (dir == htFlowDown) {
			r->rect->height = max_flow;
			htree_shift_subtree(r, cursor - r->rect->x, -r->rect->y);
			cursor += r->rect->width + ctx->opts.node_gap;
			if (r->rect->x + r->rect->width > w) w = r->rect->x + r->rect->width;
			h = max_flow;
		} else {
			r->rect->width = max_flow;
			htree_shift_subtree(r, -r->rect->x, cursor - r->rect->y);
			cursor += r->rect->height + ctx->opts.node_gap;
			if (r->rect->y + r->rect->height > h) h = r->rect->y + r->rect->height;
			w = max_flow;
		}
	}
	*content_w = w;
	*content_h = h;
	return HTREE_OK;
}

/* the entry / exit points of a child container on its border, by the
   direction of the parent whose transitions cross it */
static void htree_layout_place_border_points(const HTLayoutContext* ctx, int parent, HTreeNode* node)
{
	HTFlowDirection dir = ctx->containers[parent].direction;
	int entries = 0, exits = 0, ke = 0, kx = 0;

	if (!node || !node->rect) return;
	for (HTreeNode* p = node->children; p; p = p->next) {
		if (!htree_layout_is_border_point(p)) continue;
		if (p->role == htRoleEntryPoint) entries++; else exits++;
	}
	for (HTreeNode* p = node->children; p; p = p->next) {
		const HTreeRect* r = node->rect;
		double f;
		if (!htree_layout_is_border_point(p)) continue;
		if (ctx->update && p->point) continue;
		if (!p->point) p->point = htree_new_point();
		if (p->role == htRoleEntryPoint) {
			f = (ke + 1.0) / (entries + 1.0);
			ke++;
			if (dir == htFlowDown) { p->point->x = r->x + r->width * f; p->point->y = r->y; }
			else { p->point->x = r->x; p->point->y = r->y + r->height * f; }
		} else {
			f = (kx + 1.0) / (exits + 1.0);
			kx++;
			if (dir == htFlowDown) { p->point->x = r->x + r->width * f; p->point->y = r->y + r->height; }
			else { p->point->x = r->x + r->width; p->point->y = r->y + r->height * f; }
		}
	}
}

/* P1: the children first - the nested containers laid out, the leaves sized;
   the regions of a composite wait for the composite's frame */
static int htree_layout_children(HTLayoutContext* ctx, int ci)
{
	for (HTreeNode* child = htree_layout_first_child(ctx, ci); child; child = child->next) {
		if (htree_node_is_comment(child) || htree_layout_is_border_point(child)) continue;
		if (htree_layout_is_container(child)) {
			std::map<const HTreeNode*, int>::const_iterator it = ctx->index.find(child);
			int res;
			if (it == ctx->index.end()) return HTREE_BAD_PARAMETER;
			res = child->type == htRegion ? htree_layout_children(ctx, it->second)
				: htree_layout_container(ctx, it->second);
			if (res != HTREE_OK) return res;
		} else {
			htree_layout_size_leaf(ctx, child);
		}
	}
	return HTREE_OK;
}

/* the content in the container's frame (dir < 0: chosen by the content),
   then the fit, the border points of the children and the rect */
static int htree_layout_finish(HTLayoutContext* ctx, int ci, int dir)
{
	double cw = 0.0, ch = 0.0, W, H, ix, iy;
	int res, is_root = ci == 0;
	HTreeNode* node;

	switch (ctx->containers[ci].kind) {
	case htContainerGraph:
		res = htree_layout_graph(ctx, ci, dir, &cw, &ch);
		break;
	case htContainerRegions:
		if (dir >= 0) ctx->containers[ci].direction = (HTFlowDirection)dir;
		res = htree_layout_regions(ctx, ci, &cw, &ch);
		break;
	default:
		if (dir >= 0) ctx->containers[ci].direction = (HTFlowDirection)dir;
		res = HTREE_OK;
		break;
	}
	if (res != HTREE_OK) return res;

	node = ctx->containers[ci].node;
	htree_layout_fit(ctx, node, cw, ch, &W, &H, &ix, &iy);
	if (ctx->update) {
		/* the content goes back to the drawing: a drawn child anchors it,
		   else it starts inside the drawn rect; the rect only grows */
		const HTLayoutContainer& c = ctx->containers[ci];
		HTreeRect bounds;
		double fit_iy = iy;
		int anchored = 0;
		if (htree_layout_regions_drawn(ctx, ci)) {
			anchored = 1;
			ix = iy = 0.0;
		}
		for (size_t k = 0; k < c.children.size() && !anchored; k++) {
			HTreePoint seed;
			if (!htree_layout_seeded(ctx, c.children[k], &seed)) continue;
			ix = seed.x - (c.children[k]->rect ? c.children[k]->rect->x + c.children[k]->rect->width / 2.0 :
							c.children[k]->point ? c.children[k]->point->x : 0.0);
			iy = seed.y - (c.children[k]->rect ? c.children[k]->rect->y + c.children[k]->rect->height / 2.0 :
							c.children[k]->point ? c.children[k]->point->y : 0.0);
			anchored = 1;
		}
		if (!anchored && node && node->rect) {
			ix += node->rect->x;
			iy += node->rect->y;
		}
		for (HTreeNode* child = htree_layout_first_child(ctx, ci); child; child = child->next) {
			if (htree_node_is_comment(child) || htree_layout_is_border_point(child)) continue;
			if (htree_layout_is_container(child)) htree_layout_place_border_points(ctx, ci, child);
			htree_shift_subtree(child, ix, iy);
		}
		if (node && (!is_root || ctx->reconstruct_sm || ctx->sm_had_rect || node->rect)) {
			htree_layout_content_bounds(ctx, ci, &bounds);
			if (node->rect) {
				/* grow-only around the content with the padding */
				double x1 = bounds.x - ctx->opts.padding, y1 = bounds.y - ctx->opts.padding;
				double x2 = bounds.x + bounds.width + ctx->opts.padding;
				double y2 = bounds.y + bounds.height + ctx->opts.padding;
				double rx2 = node->rect->x + node->rect->width, ry2 = node->rect->y + node->rect->height;
				if (bounds.width > 0.0 || bounds.height > 0.0) {
					if (x1 < node->rect->x) node->rect->x = x1;
					if (y1 < node->rect->y) node->rect->y = y1;
					if (x2 > rx2) rx2 = x2;
					if (y2 > ry2) ry2 = y2;
					node->rect->width = rx2 - node->rect->x;
					node->rect->height = ry2 - node->rect->y;
				}
			} else if (!anchored) {
				/* nothing drawn inside: the rect of the full layout, the
				   content already sits at its insets */
				node->rect = htree_new_rect();
				node->rect->x = node->rect->y = 0.0;
				node->rect->width = W;
				node->rect->height = H;
			} else {
				/* a new rect around the drawn content keeps the title band */
				node->rect = htree_new_rect();
				node->rect->x = bounds.x - ctx->opts.padding;
				node->rect->y = bounds.y - fit_iy;
				node->rect->width = bounds.width + 2 * ctx->opts.padding;
				node->rect->height = bounds.height + fit_iy + ctx->opts.padding;
				if (node->rect->width < W) node->rect->width = W;
				if (node->rect->height < H) node->rect->height = H;
			}
		}
		{
			/* the pieces stay in the container frame like in the full layout:
			   P9 adds the final origin, so they follow a later move */
			std::vector<HTLayoutPiece>& pieces = ctx->containers[ci].pieces;
			double ox, oy;
			htree_layout_origin(ctx, ci, &ox, &oy);
			for (size_t k = 0; k < pieces.size(); k++) {
				for (size_t i = 0; i < pieces[k].bends.size(); i++) {
					pieces[k].bends[i].x += ix - ox;
					pieces[k].bends[i].y += iy - oy;
				}
				pieces[k].label.x += ix - ox;
				pieces[k].label.y += iy - oy;
			}
		}
		if (node && node->rect && ctx->seeds.find(node) != ctx->seeds.end()) {
			/* a grown container keeps its drawn corner: its parent anchors
			   the new centre */
			HTreePoint centre;
			htree_layout_node_centre(node, &centre);
			ctx->seeds[node] = centre;
		}
		return HTREE_OK;
	}
	for (HTreeNode* child = htree_layout_first_child(ctx, ci); child; child = child->next) {
		if (htree_node_is_comment(child) || htree_layout_is_border_point(child)) continue;
		if (htree_layout_is_container(child)) htree_layout_place_border_points(ctx, ci, child);
		htree_shift_subtree(child, ix, iy);
	}
	{
		std::vector<HTLayoutPiece>& pieces = ctx->containers[ci].pieces;
		for (size_t k = 0; k < pieces.size(); k++) {
			for (size_t i = 0; i < pieces[k].bends.size(); i++) {
				pieces[k].bends[i].x += ix;
				pieces[k].bends[i].y += iy;
			}
			pieces[k].label.x += ix;
			pieces[k].label.y += iy;
		}
	}
	if (node && (!is_root || ctx->reconstruct_sm || ctx->sm_had_rect)) {
		if (!node->rect) node->rect = htree_new_rect();
		node->rect->x = node->rect->y = 0.0;
		node->rect->width = W;
		node->rect->height = H;
	}
	return HTREE_OK;
}

static int htree_layout_container(HTLayoutContext* ctx, int ci)
{
	int res = htree_layout_children(ctx, ci);
	if (res != HTREE_OK) return res;
	return htree_layout_finish(ctx, ci, -1);
}

/* a region: its children are already laid out by the composite */
static int htree_layout_container_forced(HTLayoutContext* ctx, int ci, HTFlowDirection dir)
{
	return htree_layout_finish(ctx, ci, (int)dir);
}

/* -----------------------------------------------------------------------------
 * P9: the edge routing in the document coordinates
 * ----------------------------------------------------------------------------- */

static void htree_layout_origin(const HTLayoutContext* ctx, int ci, double* ox, double* oy)
{
	const HTreeNode* node = ctx->containers[ci].node;
	*ox = *oy = 0.0;
	if (node && node->rect) { *ox = node->rect->x; *oy = node->rect->y; }
}

static const HTLayoutPiece* htree_layout_find_piece(const HTLayoutContext* ctx, int ci, const HTreeEdge* e)
{
	const std::vector<HTLayoutPiece>& pieces = ctx->containers[ci].pieces;
	for (size_t k = 0; k < pieces.size(); k++) {
		if (pieces[k].edge == e) return &pieces[k];
	}
	return NULL;
}

/* the attachment point on the border of a box toward a point */
static int htree_layout_border_crossing(const HTreeNode* box, double px, double py, HTreePoint* out)
{
	HTreeNode a, b;
	HTreeEdge e;
	HTreePoint p;
	int res;

	memset(&a, 0, sizeof(a));
	memset(&b, 0, sizeof(b));
	memset(&e, 0, sizeof(e));
	a.type = htSimpleNode;
	a.rect = box->rect;
	a.point = box->point;
	b.type = htPoint;
	p.x = px;
	p.y = py;
	b.point = &p;
	e.source = &a;
	e.target = &b;
	if (!htree_attach_edge_minimal(&e)) {
		res = htree_project_edge_to_borders(&e);
		if (res != HTREE_OK || !e.source_point) {
			if (e.source_point) htree_destroy_point(e.source_point);
			if (e.target_point) htree_destroy_point(e.target_point);
			return 0;
		}
	}
	*out = *e.source_point;
	htree_destroy_point(e.source_point);
	if (e.target_point) htree_destroy_point(e.target_point);
	return 1;
}

typedef struct {
	int         marker;     /* 1: a border of the box still to resolve */
	HTreeNode*  box;
	HTreePoint  p;
	int         leaving;    /* the marker is crossed outward */
} HTLayoutWaypoint;

static void htree_layout_append_piece(const HTLayoutContext* ctx, int ci, const HTreeEdge* e,
									  std::vector<HTLayoutWaypoint>& wps)
{
	const HTLayoutPiece* piece = htree_layout_find_piece(ctx, ci, e);
	double ox, oy;
	if (!piece) return;
	htree_layout_origin(ctx, ci, &ox, &oy);
	for (size_t i = 0; i < piece->bends.size(); i++) {
		HTLayoutWaypoint w;
		w.marker = 0;
		w.box = NULL;
		w.leaving = 0;
		w.p.x = piece->bends[i].x + ox;
		w.p.y = piece->bends[i].y + oy;
		wps.push_back(w);
	}
}

static void htree_layout_node_centre(const HTreeNode* n, HTreePoint* p)
{
	if (n->rect) { p->x = n->rect->x + n->rect->width / 2.0; p->y = n->rect->y + n->rect->height / 2.0; }
	else if (n->point) { *p = *n->point; }
	else { p->x = p->y = 0.0; }
}

static void htree_layout_route_loop(HTLayoutContext* ctx, HTreeEdge* e)
{
	int ci = htree_layout_container_of(ctx, e->source);
	int bottom = ci >= 0 && ctx->containers[ci].direction == htFlowRight;
	const HTreeRect* r = e->source->rect;
	if (!r) return;
	htree_reconstruct_edge_loop(e, bottom);
	if (e->label_rect) {
		double w = e->label_rect->width > 0.0 ? e->label_rect->width : ctx->opts.label_width;
		double h = e->label_rect->height > 0.0 ? e->label_rect->height : ctx->opts.label_height;
		e->label_rect->width = w;
		e->label_rect->height = h;
		if (bottom) {
			e->label_rect->x = r->x + r->width / 2.0 - w / 2.0;
			e->label_rect->y = r->y + r->height + 2 * ctx->opts.padding + ctx->opts.padding / 2.0;
		} else {
			e->label_rect->x = r->x + r->width + 2 * ctx->opts.padding + ctx->opts.padding / 2.0;
			e->label_rect->y = r->y + r->height / 2.0 - h / 2.0;
		}
	}
}

static int htree_layout_route_edge(HTLayoutContext* ctx, HTreeEdge* e)
{
	std::vector<HTLayoutWaypoint> wps;
	std::vector<int> chain_s, chain_t;
	int lca = -1, tail;
	HTreePoint sc, tc;

	for (int c = htree_layout_container_of(ctx, e->source); c >= 0;
		 c = htree_layout_container_of(ctx, ctx->containers[c].node)) chain_s.push_back(c);
	for (int c = htree_layout_container_of(ctx, e->target); c >= 0;
		 c = htree_layout_container_of(ctx, ctx->containers[c].node)) chain_t.push_back(c);
	for (size_t i = 0; i < chain_t.size() && lca < 0; i++) {
		if (std::find(chain_s.begin(), chain_s.end(), chain_t[i]) != chain_s.end()) lca = chain_t[i];
	}
	if (lca < 0) return HTREE_OK;

	/* the source side: the pieces and the borders left, innermost first */
	for (size_t i = 0; i < chain_s.size() && chain_s[i] != lca; i++) {
		HTLayoutContainer& c = ctx->containers[chain_s[i]];
		htree_layout_append_piece(ctx, chain_s[i], e, wps);
		if (htree_layout_is_composite(c.node) && c.node != e->target && c.node->rect &&
			!(htree_layout_is_border_point(e->source) && e->source->parent == c.node)) {
			HTLayoutWaypoint w;
			w.marker = 1;
			w.box = c.node;
			w.leaving = 1;
			w.p.x = w.p.y = 0.0;
			wps.push_back(w);
		}
	}
	htree_layout_append_piece(ctx, lca, e, wps);
	/* the target side: the borders entered, outermost first */
	{
		std::vector<int> below;
		for (size_t i = 0; i < chain_t.size() && chain_t[i] != lca; i++) below.push_back(chain_t[i]);
		for (size_t i = below.size(); i > 0; i--) {
			HTLayoutContainer& c = ctx->containers[below[i - 1]];
			if (htree_layout_is_composite(c.node) && c.node != e->source && c.node->rect &&
				!(htree_layout_is_border_point(e->target) && e->target->parent == c.node)) {
				HTLayoutWaypoint w;
				w.marker = 1;
				w.box = c.node;
				w.leaving = 0;
				w.p.x = w.p.y = 0.0;
				wps.push_back(w);
			}
			htree_layout_append_piece(ctx, below[i - 1], e, wps);
		}
	}

	/* resolve the markers: a leaving border toward the next point, an
	   entering border from the previous one */
	htree_layout_node_centre(e->source, &sc);
	htree_layout_node_centre(e->target, &tc);
	for (size_t i = 0; i < wps.size(); i++) {
		if (!wps[i].marker || !wps[i].leaving) continue;
		HTreePoint next = tc;
		for (size_t j = i + 1; j < wps.size(); j++) if (!wps[j].marker) { next = wps[j].p; break; }
		if (htree_layout_border_crossing(wps[i].box, next.x, next.y, &wps[i].p)) wps[i].marker = 0;
	}
	for (size_t i = wps.size(); i > 0; i--) {
		if (!wps[i - 1].marker || wps[i - 1].leaving) continue;
		HTreePoint prev = sc;
		for (size_t j = i - 1; j > 0; j--) if (!wps[j - 1].marker) { prev = wps[j - 1].p; break; }
		if (htree_layout_border_crossing(wps[i - 1].box, prev.x, prev.y, &wps[i - 1].p)) wps[i - 1].marker = 0;
	}
	{
		std::vector<HTLayoutWaypoint> kept;
		for (size_t i = 0; i < wps.size(); i++) if (!wps[i].marker) kept.push_back(wps[i]);
		wps.swap(kept);
	}

	/* the ends */
	if (wps.empty()) {
		if (!htree_attach_edge_minimal(e)) {
			int res = htree_project_edge_to_borders(e);
			if (res != HTREE_OK) return res;
		}
	} else {
		HTreePoint p;
		if (!htree_layout_border_crossing(e->source, wps.front().p.x, wps.front().p.y, &p)) p = sc;
		e->source_point = htree_new_point();
		*e->source_point = p;
		if (!htree_layout_border_crossing(e->target, wps.back().p.x, wps.back().p.y, &p)) p = tc;
		e->target_point = htree_new_point();
		*e->target_point = p;
		e->polyline = htree_new_polyline();
		e->polyline->point = wps[0].p;
		HTreePolyline* pl = e->polyline;
		for (size_t i = 1; i < wps.size(); i++) {
			pl->next = htree_new_polyline();
			pl = pl->next;
			pl->point = wps[i].p;
		}
	}

	/* the label: the slot of the tail piece, or beside the first segment;
	   only the rect is set - a label carries either the point or the rect */
	if (e->label_rect) {
		tail = htree_layout_container_of(ctx, e->source);
		const HTLayoutPiece* piece = tail >= 0 ? htree_layout_find_piece(ctx, tail, e) : NULL;
		double w = e->label_rect->width > 0.0 ? e->label_rect->width : ctx->opts.label_width;
		double h = e->label_rect->height > 0.0 ? e->label_rect->height : ctx->opts.label_height;
		e->label_rect->width = w;
		e->label_rect->height = h;
		if (piece && piece->has_label) {
			double ox, oy;
			htree_layout_origin(ctx, tail, &ox, &oy);
			e->label_rect->x = piece->label.x + ox;
			e->label_rect->y = piece->label.y + oy;
		} else {
			HTreePoint a = *e->source_point, b = wps.empty() ? *e->target_point : wps.front().p;
			e->label_rect->x = (a.x + b.x) / 2.0 + ctx->opts.padding;
			e->label_rect->y = (a.y + b.y) / 2.0 - h / 2.0;
		}
	}
	return HTREE_OK;
}

static int htree_layout_same_pair(const HTreeEdge* a, const HTreeEdge* b)
{
	return (a->source == b->source && a->target == b->target) ||
		(a->source == b->target && a->target == b->source);
}

static void htree_layout_shift_edge(HTreeEdge* e, double dx, double dy)
{
	if (e->source_point) { e->source_point->x += dx; e->source_point->y += dy; }
	if (e->target_point) { e->target_point->x += dx; e->target_point->y += dy; }
	if (e->label_point) { e->label_point->x += dx; e->label_point->y += dy; }
	if (e->label_rect) { e->label_rect->x += dx; e->label_rect->y += dy; }
}

/* the straight transitions between one pair of states (in either direction)
   would coincide: spread them across the flow axis by two paddings */
static void htree_layout_spread_pairs(HTLayoutContext* ctx)
{
	for (HTreeEdge* e = ctx->tree->edges; e; e = e->next) {
		std::vector<HTreeEdge*> group;
		int ci, across_x;
		double step = 2 * ctx->opts.padding, k = 0.0;
		if (!e->source || !e->target || e->source == e->target || e->polyline ||
			!e->source_point || !e->target_point) continue;
		if (ctx->update && ctx->routed.find(e) == ctx->routed.end()) continue;
		for (HTreeEdge* f = e; f; f = f->next) {
			if (f->source && f->target && f != e && f->source != f->target && !f->polyline &&
				f->source_point && f->target_point && htree_layout_same_pair(e, f)) {
				if (f < e) break;
			}
		}
		for (HTreeEdge* f = ctx->tree->edges; f && f != e; f = f->next) {
			if (f->source && f->target && !f->polyline && f->source_point && f->target_point &&
				htree_layout_same_pair(e, f)) group.clear();
		}
		for (HTreeEdge* f = ctx->tree->edges; f; f = f->next) {
			if (f->source && f->target && f->source != f->target && !f->polyline &&
				f->source_point && f->target_point && htree_layout_same_pair(e, f) &&
				(!ctx->update || ctx->routed.find(f) != ctx->routed.end())) group.push_back(f);
		}
		if (group.size() < 2 || group[0] != e) continue;
		ci = htree_layout_container_of(ctx, e->source);
		across_x = ci < 0 || ctx->containers[ci].direction == htFlowDown;
		for (size_t i = 0; i < group.size(); i++, k += 1.0) {
			double d = (k - (group.size() - 1) / 2.0) * step;
			htree_layout_shift_edge(group[i], across_x ? d : 0.0, across_x ? 0.0 : d);
		}
	}
}

/* the update: a drawn transition keeps its geometry when neither end moved;
   when both moved by one offset (a drawn group put into a new container)
   it moves with them */
static int htree_layout_edge_kept(const HTLayoutContext* ctx, HTreeEdge* e)
{
	HTreePoint s0, s1, t0, t1;
	double dx, dy;
	if (!ctx->update || ctx->edge_seeds.find(e) == ctx->edge_seeds.end()) return 0;
	if (!htree_layout_seeded(ctx, e->source, &s0) || !htree_layout_seeded(ctx, e->target, &t0)) return 0;
	htree_layout_node_centre(e->source, &s1);
	htree_layout_node_centre(e->target, &t1);
	dx = s1.x - s0.x;
	dy = s1.y - s0.y;
	if (std::fabs(t1.x - t0.x - dx) > HTREE_COORD_EPS || std::fabs(t1.y - t0.y - dy) > HTREE_COORD_EPS) return 0;
	if (std::fabs(dx) > HTREE_COORD_EPS || std::fabs(dy) > HTREE_COORD_EPS) {
		htree_layout_shift_edge(e, dx, dy);
		for (HTreePolyline* pl = e->polyline; pl; pl = pl->next) {
			pl->point.x += dx;
			pl->point.y += dy;
		}
	}
	return 1;
}

static void htree_layout_drop_edge_geometry(HTreeEdge* e)
{
	if (e->source_point) { htree_destroy_point(e->source_point); e->source_point = NULL; }
	if (e->target_point) { htree_destroy_point(e->target_point); e->target_point = NULL; }
	if (e->label_point) { htree_destroy_point(e->label_point); e->label_point = NULL; }
	if (e->polyline) { htree_destroy_polyline(e->polyline); e->polyline = NULL; }
	if (e->label_rect) { e->label_rect->x = e->label_rect->y = 0.0; }   /* the size stays the preset */
}

static int htree_layout_route_edges(HTLayoutContext* ctx)
{
	for (HTreeEdge* e = ctx->tree->edges; e; e = e->next) {
		int res;
		if (!e->source || !e->target ||
			htree_node_is_comment(e->source) || htree_node_is_comment(e->target)) continue;
		if (!(e->source->rect || e->source->point) || !(e->target->rect || e->target->point)) continue;
		if (ctx->update) {
			if (htree_layout_edge_kept(ctx, e)) continue;
			htree_layout_drop_edge_geometry(e);
			ctx->routed.insert(e);
		}
		if (e->source == e->target) {
			htree_layout_route_loop(ctx, e);
			continue;
		}
		res = htree_layout_route_edge(ctx, e);
		if (res != HTREE_OK) return res;
	}
	htree_layout_spread_pairs(ctx);
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * P10: the comments
 * ----------------------------------------------------------------------------- */

static int htree_layout_rects_overlap(const HTreeRect* a, const HTreeRect* b)
{
	return a->x < b->x + b->width && b->x < a->x + a->width &&
		a->y < b->y + b->height && b->y < a->y + a->height;
}

static void htree_layout_node_bounds(const HTreeNode* n, HTreeRect* r)
{
	if (n->rect) *r = *n->rect;
	else if (n->point) { r->x = n->point->x; r->y = n->point->y; r->width = r->height = 0.0; }
	else htree_init_rect(r);
}

/* the rect is free of the container's other children */
static int htree_layout_rect_free(const HTLayoutContext* ctx, int ci, const HTreeNode* self, const HTreeRect* r)
{
	for (HTreeNode* n = htree_layout_first_child(ctx, ci); n; n = n->next) {
		HTreeRect b;
		if (n == self || (!n->rect && !n->point)) continue;
		htree_layout_node_bounds(n, &b);
		if (htree_layout_rects_overlap(r, &b)) return 0;
	}
	return 1;
}

static void htree_layout_content_bounds(const HTLayoutContext* ctx, int ci, HTreeRect* r)
{
	int any = 0;
	htree_init_rect(r);
	for (HTreeNode* n = htree_layout_first_child(ctx, ci); n; n = n->next) {
		HTreeRect b;
		if (htree_node_is_comment(n) || (!n->rect && !n->point)) continue;
		htree_layout_node_bounds(n, &b);
		if (!any) { *r = b; any = 1; continue; }
		double x2 = r->x + r->width > b.x + b.width ? r->x + r->width : b.x + b.width;
		double y2 = r->y + r->height > b.y + b.height ? r->y + r->height : b.y + b.height;
		if (b.x < r->x) r->x = b.x;
		if (b.y < r->y) r->y = b.y;
		r->width = x2 - r->x;
		r->height = y2 - r->y;
	}
}

static void htree_layout_place_comment(HTLayoutContext* ctx, HTreeNode* comment)
{
	const HTLayoutOptions* o = &ctx->opts;
	HTreeNode* element = NULL;
	HTreeRect r, e, content;
	int ci;

	if (comment->rect) return;
	r.width = comment->min_rect ? comment->min_rect->width : o->node_width;
	r.height = comment->min_rect ? comment->min_rect->height : o->node_height;
	for (HTreeEdge* edge = ctx->tree->edges; edge && !element; edge = edge->next) {
		if (edge->source == comment && edge->target && edge->target != comment) element = edge->target;
	}
	if (element && (element->rect || element->point)) {
		ci = htree_layout_container_of(ctx, element);
		HTFlowDirection dir = ci >= 0 ? ctx->containers[ci].direction : o->direction;
		double gap = o->node_gap;
		htree_layout_node_bounds(element, &e);
		for (int cand = 0; cand < 3 && ci >= 0; cand++) {
			if (dir == htFlowDown) {
				if (cand == 0) { r.x = e.x + e.width + gap; r.y = e.y; }
				else if (cand == 1) { r.x = e.x - gap - r.width; r.y = e.y; }
				else { r.x = e.x; r.y = e.y + e.height + gap; }
			} else {
				if (cand == 0) { r.x = e.x; r.y = e.y + e.height + gap; }
				else if (cand == 1) { r.x = e.x; r.y = e.y - gap - r.height; }
				else { r.x = e.x + e.width + gap; r.y = e.y; }
			}
			if (htree_layout_rect_free(ctx, ci, comment, &r)) {
				comment->rect = htree_new_rect();
				*comment->rect = r;
				return;
			}
		}
	}
	/* the shelf after the content of the comment's own container */
	ci = htree_layout_container_of(ctx, comment);
	if (ci < 0) ci = 0;
	htree_layout_content_bounds(ctx, ci, &content);
	if (ctx->containers[ci].direction == htFlowDown) {
		r.x = content.x + ctx->containers[ci].shelf;
		r.y = content.y + content.height + o->node_gap;
		ctx->containers[ci].shelf += r.width + o->node_gap;
	} else {
		r.x = content.x + content.width + o->node_gap;
		r.y = content.y + ctx->containers[ci].shelf;
		ctx->containers[ci].shelf += r.height + o->node_gap;
	}
	comment->rect = htree_new_rect();
	*comment->rect = r;
}

static void htree_layout_place_comments(HTLayoutContext* ctx, HTreeNode* nodes)
{
	for (HTreeNode* n = nodes; n; n = n->next) {
		if (htree_node_is_comment(n)) htree_layout_place_comment(ctx, n);
		if (n->children) htree_layout_place_comments(ctx, n->children);
	}
}

/* -----------------------------------------------------------------------------
 * The tree layout
 * ----------------------------------------------------------------------------- */

/* the final shape of the machine: the root rect, else the top-level content */
static void htree_layout_final_shape(const HTLayoutContext* ctx, double* w, double* h)
{
	const HTreeNode* root = ctx->containers[0].node;
	HTreeRect r;
	if (root && root->rect) {
		*w = root->rect->width;
		*h = root->rect->height;
		return;
	}
	htree_layout_content_bounds(ctx, 0, &r);
	*w = r.width;
	*h = r.height;
}

/* the update: the centres of the drawn nodes and the drawn edges are the seeds */
static void htree_layout_collect_seeds(HTLayoutContext* ctx, HTreeNode* nodes)
{
	for (HTreeNode* n = nodes; n; n = n->next) {
		if (n->rect || n->point) {
			HTreePoint c;
			htree_layout_node_centre(n, &c);
			ctx->seeds[n] = c;
		}
		if (n->children) htree_layout_collect_seeds(ctx, n->children);
	}
}

static int htree_layout_tree_pass(HTree* tree, int reconstruct_sm, const HTLayoutOptions* opts,
								  HTFlowDirection start, int update, double* w, double* h)
{
	HTLayoutContext ctx;
	HTreeNode* root;
	int res;

	ctx.tree = tree;
	ctx.opts = *opts;
	ctx.start = start;
	ctx.reconstruct_sm = reconstruct_sm;
	ctx.update = update;
	root = (tree->nodes && !tree->nodes->next && tree->nodes->type == htTree) ? tree->nodes : NULL;
	ctx.sm_had_rect = root && root->rect;

	if (update) {
		if (tree->nodes) htree_layout_collect_seeds(&ctx, tree->nodes);
		for (HTreeEdge* e = tree->edges; e; e = e->next) {
			if (e->source_point || e->target_point || e->polyline || e->label_point) ctx.edge_seeds.insert(e);
		}
	} else {
		if (tree->nodes) htree_clean_tree_geometry(tree->nodes);
		for (HTreeEdge* e = tree->edges; e; e = e->next) {
			if (e->source_point) { htree_destroy_point(e->source_point); e->source_point = NULL; }
			if (e->target_point) { htree_destroy_point(e->target_point); e->target_point = NULL; }
			if (e->label_point) { htree_destroy_point(e->label_point); e->label_point = NULL; }
			if (e->polyline) { htree_destroy_polyline(e->polyline); e->polyline = NULL; }
		}
	}

	htree_layout_collect(&ctx, root, 0);
	htree_layout_seed_edges(&ctx);
	if (update) htree_layout_infer_directions(&ctx);
	res = htree_layout_container(&ctx, 0);
	if (res != HTREE_OK) return res;
	res = htree_layout_route_edges(&ctx);
	if (res != HTREE_OK) return res;
	htree_layout_place_comments(&ctx, tree->nodes);
	res = htree_grow_sm_border(tree);
	if (res != HTREE_OK) return res;
	htree_layout_final_shape(&ctx, w, h);
	return HTREE_OK;
}

/* the adaptive mode tries both starts and keeps the layout whose machine
   shape is better (wide rather than tall, then closer to the target); the
   geometry of the winner stays */
int htree_layout_tree(HTree* tree, int reconstruct_sm, const HTLayoutOptions* opts, int update)
{
	double wr, hr, wd, hd;
	int res, better;

	if (!tree || !opts) {
		return HTREE_BAD_PARAMETER;
	}
	if (update || opts->mode != htLayoutAdaptive) {
		return htree_layout_tree_pass(tree, reconstruct_sm, opts, opts->direction, update, &wr, &hr);
	}
	res = htree_layout_tree_pass(tree, reconstruct_sm, opts, htFlowRight, 0, &wr, &hr);
	if (res != HTREE_OK) return res;
	res = htree_layout_tree_pass(tree, reconstruct_sm, opts, htFlowDown, 0, &wd, &hd);
	if (res != HTREE_OK) return res;
	better = htree_layout_shape_better(wr, hr, wd, hd, opts->aspect);
	if (better == 0 || (better < 0 && opts->direction == htFlowRight)) {
		return htree_layout_tree_pass(tree, reconstruct_sm, opts, htFlowRight, 0, &wr, &hr);
	}
	return HTREE_OK;
}

/* -----------------------------------------------------------------------------
 * Several machines: every tree laid out on its own, then placed in a row
 * ----------------------------------------------------------------------------- */

static void htree_shift_tree(HTree* tree, double dx, double dy)
{
	for (HTreeNode* n = tree->nodes; n; n = n->next) {
		htree_shift_subtree(n, dx, dy);
	}
	for (HTreeEdge* e = tree->edges; e; e = e->next) {
		htree_layout_shift_edge(e, dx, dy);
		for (HTreePolyline* pl = e->polyline; pl; pl = pl->next) {
			pl->point.x += dx;
			pl->point.y += dy;
		}
	}
}

/* the machines side by side (wide rather than tall) in the document order,
   two node gaps apart, their tops aligned; the comments travel with their
   machine */
int htree_layout_place_trees(HTDocument* doc, const HTLayoutOptions* opts, const int* drawn)
{
	double cursor = 0.0;
	int k = 0;

	if (!doc || !opts) {
		return HTREE_BAD_PARAMETER;
	}
	/* the update: the drawn machines stay, the row starts after their extent */
	for (HTree* tree = doc->trees; tree; tree = tree->next, k++) {
		HTreeRect* bounds = NULL;
		if (!drawn || !drawn[k]) continue;
		if (htree_tree_bounding_rect(tree, 0, &bounds) != HTREE_OK || !bounds) continue;
		if (bounds->x + bounds->width + 2 * opts->node_gap > cursor) cursor = bounds->x + bounds->width + 2 * opts->node_gap;
		htree_destroy_rect(bounds);
	}
	k = 0;
	for (HTree* tree = doc->trees; tree; tree = tree->next, k++) {
		HTreeRect* bounds = NULL;
		int res;
		if (drawn && drawn[k]) continue;
		res = htree_tree_bounding_rect(tree, 0, &bounds);
		if (res != HTREE_OK) {
			return res;
		}
		if (!bounds) {
			continue;
		}
		htree_shift_tree(tree, cursor - bounds->x, -bounds->y);
		cursor += bounds->width + 2 * opts->node_gap;
		htree_destroy_rect(bounds);
	}
	return HTREE_OK;
}
