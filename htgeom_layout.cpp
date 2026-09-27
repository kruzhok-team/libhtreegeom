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
	opts->direction = htFlowDown;
	opts->alternate = 1;
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
	*out = *in;
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
} HTLayoutVertex;

typedef struct {
	int                 source, target;   /* in the layout direction */
	HTreeEdge*          edge;
	int                 reversed;
	int                 has_label;
	double              label_w, label_h;
	std::vector<int>    chain;            /* source, dummies, target by layer */
} HTLayoutEdge;

typedef struct {
	HTFlowDirection                 direction;
	std::vector<HTLayoutVertex>     vertices;   /* ENTRY, the children, EXIT, the dummies */
	std::vector<HTLayoutEdge>       edges;
	std::vector<std::vector<int> >  layers;     /* 0 = ENTRY .. last + 1 = EXIT */
	std::vector<std::vector<int> >  up, down;   /* the neighbours in the adjacent layers */
	int                             entry, exit, last;
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
	int                             reconstruct_sm;
	int                             sm_had_rect;
	std::vector<HTLayoutContainer>  containers;   /* pre-order, 0 = the root */
	std::map<const HTreeNode*, int> index;        /* lookup only */
} HTLayoutContext;

static int htree_layout_source_role(HTNodeRole r)
{
	return r == htRoleInitial || r == htRoleEntryPoint ||
		r == htRoleShallowHistory || r == htRoleDeepHistory;
}

static int htree_layout_sink_role(HTNodeRole r)
{
	return r == htRoleFinal || r == htRoleTerminate || r == htRoleExitPoint;
}

static int htree_layout_is_border_point(const HTreeNode* n)
{
	return n && n->type == htPoint && (n->role == htRoleEntryPoint || n->role == htRoleExitPoint);
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

static HTFlowDirection htree_layout_direction(const HTLayoutOptions* o, int level)
{
	if (o->alternate && (level % 2) == 1) {
		return o->direction == htFlowDown ? htFlowRight : htFlowDown;
	}
	return o->direction;
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
	ctx->containers[ci].direction = htree_layout_direction(&ctx->opts, level);
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

/* -----------------------------------------------------------------------------
 * The layered layout of one graph container: P2 - P6
 * ----------------------------------------------------------------------------- */

static void htree_layout_build_graph(const HTLayoutContext* ctx, int ci, HTLayoutGraph* g)
{
	const HTLayoutContainer& c = ctx->containers[ci];
	const HTLayoutOptions* o = &ctx->opts;
	size_t n = c.children.size();

	g->direction = c.direction;
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
		if (k >= 1 && k <= n) {
			HTreeNode* node = c.children[k - 1];
			v.node = node;
			v.role = node->role;
			v.kind = htVertexReal;
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
		if (s.self_loop) continue;
		e.source = s.source_child < 0 ? g->entry : s.source_child + 1;
		e.target = s.target_child < 0 ? g->exit : s.target_child + 1;
		e.edge = s.edge;
		e.reversed = 0;
		e.has_label = s.has_label;
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
   the back edges are reversed. Returns the topological order */
static void htree_layout_break_cycles(HTLayoutGraph* g, std::vector<int>& topo)
{
	size_t n = g->vertices.size();
	std::vector<std::vector<int> > out(n);
	std::vector<int> colour(n, 0), indeg(n, 0), roots;
	std::vector<std::pair<int, size_t> > stack;

	for (size_t k = 0; k < g->edges.size(); k++) {
		HTLayoutEdge& e = g->edges[k];
		if ((htree_layout_source_role(g->vertices[e.target].role) && e.source != g->entry) ||
			(htree_layout_sink_role(g->vertices[e.source].role) && e.target != g->exit)) {
			htree_layout_reverse_edge(e);
		}
	}
	for (size_t k = 0; k < g->edges.size(); k++) {
		out[g->edges[k].source].push_back((int)k);
		indeg[g->edges[k].target]++;
	}
	for (size_t v = 0; v < n; v++) if (htree_layout_source_role(g->vertices[v].role)) roots.push_back((int)v);
	for (size_t v = 0; v < n; v++) if (indeg[v] == 0) roots.push_back((int)v);
	for (size_t v = 0; v < n; v++) roots.push_back((int)v);

	topo.clear();
	for (size_t r = 0; r < roots.size(); r++) {
		if (colour[roots[r]]) continue;
		stack.push_back(std::make_pair(roots[r], (size_t)0));
		colour[roots[r]] = 1;
		while (!stack.empty()) {
			int v = stack.back().first;
			size_t& i = stack.back().second;
			if (i < out[v].size()) {
				HTLayoutEdge& e = g->edges[out[v][i++]];
				int w = e.target;
				if (colour[w] == 0) {
					colour[w] = 1;
					stack.push_back(std::make_pair(w, (size_t)0));
				} else if (colour[w] == 1) {
					htree_layout_reverse_edge(e);
				}
			} else {
				colour[v] = 2;
				topo.push_back(v);
				stack.pop_back();
			}
		}
	}
	std::reverse(topo.begin(), topo.end());
}

/* P3: the longest-path layering with the pinned roles */
static void htree_layout_assign_layers(HTLayoutGraph* g, const std::vector<int>& topo)
{
	size_t n = g->vertices.size();
	std::vector<std::vector<int> > in(n);
	int has_source = 0, max_layer = 0, sink_layer, exit_layer;

	for (size_t k = 0; k < g->edges.size(); k++) {
		in[g->edges[k].target].push_back((int)k);
	}
	for (size_t v = 1; v < n; v++) {
		if (g->vertices[v].kind == htVertexReal && htree_layout_source_role(g->vertices[v].role)) has_source = 1;
	}
	for (size_t t = 0; t < topo.size(); t++) {
		int v = topo[t];
		HTLayoutVertex& vx = g->vertices[v];
		int layer;
		if (v == g->entry) {
			layer = 0;
		} else if (v == g->exit) {
			layer = 1;
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
	/* the sinks close the flow in a common last layer */
	sink_layer = 0;
	for (size_t v = 1; v + 1 < n; v++) {
		const HTLayoutVertex& vx = g->vertices[v];
		if (htree_layout_sink_role(vx.role)) {
			if (vx.layer > sink_layer) sink_layer = vx.layer;
		} else if (vx.layer > max_layer) {
			max_layer = vx.layer;
		}
	}
	if (sink_layer > 0 && sink_layer < max_layer + 1) sink_layer = max_layer + 1;
	for (size_t v = 1; v + 1 < n; v++) {
		HTLayoutVertex& vx = g->vertices[v];
		if (htree_layout_sink_role(vx.role)) vx.layer = sink_layer;
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
		if (nb.empty()) {
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
			while (j < L.size() && std::fabs(b[j] - b[i]) < HTREE_COORD_EPS) j++;
			if (j - i > 1) {
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
	std::vector<double> bsum, bw;
	std::vector<size_t> bstart;
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
		if (nb.empty()) {
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
	for (size_t i = 0; i < n; i++) {
		bsum.push_back(w[i] * d[i]);
		bw.push_back(w[i]);
		bstart.push_back(i);
		while (bsum.size() > 1 &&
			   bsum.back() / bw.back() < bsum[bsum.size() - 2] / bw[bw.size() - 2]) {
			bsum[bsum.size() - 2] += bsum.back();
			bw[bw.size() - 2] += bw.back();
			bsum.pop_back();
			bw.pop_back();
			bstart.pop_back();
		}
	}
	for (size_t k = 0; k < bstart.size(); k++) {
		size_t end = k + 1 < bstart.size() ? bstart[k + 1] : n;
		double m = bsum[k] / bw[k];
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
			htree_layout_to_doc(g->direction, d.flow, d.cross, &p.x, &p.y);
			piece.bends.push_back(p);
		}
		if (e.reversed) std::reverse(piece.bends.begin(), piece.bends.end());
		c.pieces.push_back(piece);
	}
}

static int htree_layout_graph(HTLayoutContext* ctx, int ci, double* content_w, double* content_h)
{
	HTLayoutGraph g;
	std::vector<int> topo;
	double flow_extent, cross_extent;

	htree_layout_build_graph(ctx, ci, &g);
	htree_layout_break_cycles(&g, topo);
	htree_layout_assign_layers(&g, topo);
	htree_layout_insert_dummies(&g, &ctx->opts);
	htree_layout_order(&g, &ctx->opts);
	htree_layout_assign_coordinates(&g, &ctx->opts);
	htree_layout_normalise(&g, &flow_extent, &cross_extent);
	htree_layout_apply_graph(ctx, ci, &g);
	if (g.direction == htFlowDown) { *content_w = cross_extent; *content_h = flow_extent; }
	else { *content_w = flow_extent; *content_h = cross_extent; }
	return HTREE_OK;
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

/* P7: the regions side by side across the flow axis, widened to the widest */
static int htree_layout_regions(HTLayoutContext* ctx, int ci, double* content_w, double* content_h)
{
	HTLayoutContainer& c = ctx->containers[ci];
	double max_flow = 0.0, cursor = 0.0, w = 0.0, h = 0.0;

	for (HTreeNode* r = c.node->children; r; r = r->next) {
		if (r->type != htRegion || !r->rect) continue;
		double f = c.direction == htFlowDown ? r->rect->height : r->rect->width;
		if (f > max_flow) max_flow = f;
	}
	for (HTreeNode* r = c.node->children; r; r = r->next) {
		if (r->type != htRegion || !r->rect) continue;
		if (c.direction == htFlowDown) {
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

static void htree_layout_place_border_points(HTLayoutContext* ctx, int ci)
{
	HTLayoutContainer& c = ctx->containers[ci];
	int parent = c.node ? htree_layout_container_of(ctx, c.node) : -1;
	HTFlowDirection dir = parent >= 0 ? ctx->containers[parent].direction : ctx->opts.direction;
	int entries = 0, exits = 0, ke = 0, kx = 0;

	if (!c.node || !c.node->rect) return;
	for (HTreeNode* p = c.node->children; p; p = p->next) {
		if (!htree_layout_is_border_point(p)) continue;
		if (p->role == htRoleEntryPoint) entries++; else exits++;
	}
	for (HTreeNode* p = c.node->children; p; p = p->next) {
		const HTreeRect* r = c.node->rect;
		double f;
		if (!htree_layout_is_border_point(p)) continue;
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

static int htree_layout_container(HTLayoutContext* ctx, int ci)
{
	double cw = 0.0, ch = 0.0, W, H, ix, iy;
	int res, is_root = ci == 0;
	HTreeNode* node;

	for (HTreeNode* child = htree_layout_first_child(ctx, ci); child; child = child->next) {
		if (htree_node_is_comment(child) || htree_layout_is_border_point(child)) continue;
		if (htree_layout_is_container(child)) {
			std::map<const HTreeNode*, int>::const_iterator it = ctx->index.find(child);
			if (it == ctx->index.end()) return HTREE_BAD_PARAMETER;
			res = htree_layout_container(ctx, it->second);
			if (res != HTREE_OK) return res;
		} else {
			htree_layout_size_leaf(ctx, child);
		}
	}
	switch (ctx->containers[ci].kind) {
	case htContainerGraph:
		res = htree_layout_graph(ctx, ci, &cw, &ch);
		break;
	case htContainerRegions:
		res = htree_layout_regions(ctx, ci, &cw, &ch);
		break;
	default:
		res = HTREE_OK;
		break;
	}
	if (res != HTREE_OK) return res;

	node = ctx->containers[ci].node;
	htree_layout_fit(ctx, node, cw, ch, &W, &H, &ix, &iy);
	for (HTreeNode* child = htree_layout_first_child(ctx, ci); child; child = child->next) {
		if (htree_node_is_comment(child) || htree_layout_is_border_point(child)) continue;
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
	htree_layout_place_border_points(ctx, ci);
	return HTREE_OK;
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
				f->source_point && f->target_point && htree_layout_same_pair(e, f)) group.push_back(f);
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

static int htree_layout_route_edges(HTLayoutContext* ctx)
{
	for (HTreeEdge* e = ctx->tree->edges; e; e = e->next) {
		int res;
		if (!e->source || !e->target ||
			htree_node_is_comment(e->source) || htree_node_is_comment(e->target)) continue;
		if (!(e->source->rect || e->source->point) || !(e->target->rect || e->target->point)) continue;
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

int htree_layout_tree(HTree* tree, int reconstruct_sm, const HTLayoutOptions* opts)
{
	HTLayoutContext ctx;
	HTreeNode* root;
	int res;

	if (!tree || !opts) {
		return HTREE_BAD_PARAMETER;
	}
	ctx.tree = tree;
	ctx.opts = *opts;
	ctx.reconstruct_sm = reconstruct_sm;
	root = (tree->nodes && !tree->nodes->next && tree->nodes->type == htTree) ? tree->nodes : NULL;
	ctx.sm_had_rect = root && root->rect;

	if (tree->nodes) htree_clean_tree_geometry(tree->nodes);
	for (HTreeEdge* e = tree->edges; e; e = e->next) {
		if (e->source_point) { htree_destroy_point(e->source_point); e->source_point = NULL; }
		if (e->target_point) { htree_destroy_point(e->target_point); e->target_point = NULL; }
		if (e->label_point) { htree_destroy_point(e->label_point); e->label_point = NULL; }
		if (e->polyline) { htree_destroy_polyline(e->polyline); e->polyline = NULL; }
	}

	htree_layout_collect(&ctx, root, 0);
	htree_layout_seed_edges(&ctx);
	res = htree_layout_container(&ctx, 0);
	if (res != HTREE_OK) return res;
	res = htree_layout_route_edges(&ctx);
	if (res != HTREE_OK) return res;
	htree_layout_place_comments(&ctx, tree->nodes);
	return htree_grow_sm_border(tree);
}
