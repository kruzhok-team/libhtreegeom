# The Reconstruction Layout

The document specifies the layout algorithm behind
`htree_reconstruct_document_geometry`: the full rebuild of the geometry
of a hierarchical state machine diagram from its structure alone. It
replaces the reading-order shelf placement of 1.0.6 and drops the
closeness-to-original target with it: an original layout may be
generated, broken or absent, so the only signal the algorithm trusts is
the structure - the nesting of the states and the flow of the
transitions. The preserving fill-in (the open-time repair of a partial
geometry) stays as it is and is out of scope here.

```
     structure only                                    geometry
    +---------------------+                        +-------------------+
    |  tree of nodes      |   layered layout,      |  rects, points,   |
    |  (roles, min sizes) | --- bottom-up per ---> |  edge end points, |
    |  directed edges     |   region, HV frames    |  polylines,       |
    |  (labels)           |                        |  label rects      |
    +---------------------+                        +-------------------+
```

## The Sources

Four sources were analysed; the table names what each one contributes.

```
    +-----------------------------+------------------------------------------+
    | source                      | taken into the design                    |
    +-----------------------------+------------------------------------------+
    | Sugiyama, Tagawa, Toda 1981 | the four steps; dummy vertices for long  |
    | "Methods for Visual Under-  | edges; the barycentre sweeps (phase 1    |
    | standing of Hierarchical    | DOWN/UP, phase 2 block reversal); the    |
    | System Structures", IEEE    | crossing count; the priority layout      |
    | SMC-11(2)                   | method for the coordinates               |
    +-----------------------------+------------------------------------------+
    | Sugiyama, Misue 1991        | the compound digraph model; conventions  |
    | "Visualization of Struc-    | C1-C4 and rules R1-R5 with priority; the |
    | tural Information: Auto-    | local hierarchies (one per container,    |
    | matic Drawing of Compound   | children contiguous); the splitting      |
    | Digraphs", IEEE SMC-21(4)   | method; the recursive metrical layout    |
    |                             | with the container widths fed upward     |
    +-----------------------------+------------------------------------------+
    | Feng 1997 "Algorithms for   | the drawing conventions of nested        |
    | Drawing Clustered Graphs",  | regions: containment, one border crossing|
    | PhD thesis, Newcastle       | per edge per cluster; bundling of the    |
    |                             | in/out edges of a cluster at its two     |
    |                             | flow sides; rectangles instead of hulls  |
    +-----------------------------+------------------------------------------+
    | Eclipse Layout Kernel       | the five-phase pipeline with pluggable   |
    | (ELK Layered) and the       | strategies; bottom-up layout per region  |
    | SCCharts practice           | with region packing; FIRST/LAST layer    |
    |                             | constraints for the initial and final    |
    |                             | states; model order as the tie-breaker;  |
    |                             | label dummies; self-loops taken out of   |
    |                             | the ordering; direction alternating by   |
    |                             | depth                                    |
    +-----------------------------+------------------------------------------+
```

### Sugiyama, Tagawa, Toda 1981

The hierarchical drawing of an n-level digraph in four steps: (I) cycle
removal and the splitting of the long-span edges with dummy vertices,
(II) reduction of crossings by permuting each level, (III) horizontal
coordinates keeping the order, (IV) drawing. The readability elements
are A hierarchical placement, B few crossings, C straight edges
(one-span and long-span), D connected vertices close, E balanced edges.

The barycentre of a vertex in level i+1 over the fixed level i, with
m_kl the adjacency matrix and l the positions in the fixed level:

```
    B(k) = sum_l (l * m_kl) / sum_l m_kl
```

The crossing count of two adjacent levels for the rows j before k:

```
    K = sum_{j<k} sum_{a<b} m_jb * m_ka
```

The BC method sorts a level by the barycentres (a stable sort, equal
values keep their order). Phase 1 sweeps DOWN (levels 2..n, the level
above fixed) then UP (levels n-1..1), keeps a permutation only when the
crossing count decreases, and stops when the order recurs or the
iteration limit is reached. Phase 2 reverses each block of vertices
with equal barycentres and re-runs phase 1 when that changes the
neighbour level's barycentre order. The priority layout method assigns
the coordinates: the dummies get the highest priority, the other
vertices their connectivity; in the DOWN/UP order each vertex moves
toward its barycentre keeping the level order, integer distinct
positions, and pushing only the lower-priority neighbours by the least
amount. The cycle removal by condensation (strongly connected components
collapsed to a vertex) is not usable for state machines, whose cycles
are the content.

### Sugiyama, Misue 1991

The compound digraph D = (V, E, F): an inclusion tree (V, E) and an
adjacency digraph (V, F), where an adjacency edge never joins a vertex
with its own ancestor or descendant. The drawing conventions: C1 a
vertex is a rectangle, C2 inclusion is geometric inclusion, disjoint
rectangles otherwise, C3 the vertices lie on parallel nested horizontal
bands - the compound levels, C4 an adjacency edge runs downward from the
bottom side of the source to the top side of the target. The rules and
their priority:

```
    R1 closeness of connected vertices  >  R2 few line crossings
       >  R3 few line-rectangle crossings  >  R4 straight lines
       >  R5 balanced edges at a vertex
```

R1 is the adjacency of *connected* vertices - it has nothing in common
with the closeness-to-original metric this design drops. The four steps:

* hierarchization - a *compound level* (a sequence of integers along the
  inclusion path, ordered lexicographically) is assigned to every vertex
  from a *derived graph* where each adjacency edge is replaced by
  constraints between the ancestors of its ends below their common
  ancestor; the strongly connected components of the derived graph are
  resolved by reversing feedback edges (a heuristic, the exact problem is
  NP-complete) and the reversed edges are drawn upward at the end;
* normalization - every non-proper adjacency edge (its ends not on
  adjacent levels of the same container) is replaced by a linear chain of
  dummy vertices and dummy inclusion edges, so that every edge joins
  adjacent levels of one *local hierarchy*;
* vertex ordering - each container is a local hierarchy H(v) with its
  levels, its inner edges F^d (between levels), its same-level edges F^s
  and, for every child w, the counts lambda(w) / rho(w) of the edges
  leaving w toward the outside on the left / right; the problem is solved
  container by container from the root to the leaves (VOrderGlobal ->
  VOrderLocal): the *splitting method* first pins the children with
  outside connections to the left or right end by the sign of
  lambda - rho, then the barycentre DOWN/UP sweeps (with the insertion of
  the dummies for the same-level edges) reorder the rest; the children of
  one container stay contiguous by construction;
* metrical layout - the horizontal problem is solved recursively
  (LayoutLocal: the non-leaf children first, then the container by the
  priority method with the children's final widths), the initial position
  of the k-th rectangle in a level is
  `delta_1/2 + (d_2 + delta_2) + ... + (d_2 + delta_k/2)` (the widths
  packed with the sibling gap d_2), the improvement passes run DOWN
  (levels 2..n), UP (n-1..1), DOWN (t..n) with the priorities of the 1981
  method, then the container width becomes the content span plus 2 d_1;
  the vertical problem is independent and easy; the edge routing follows
  from the positions: a proper edge is a straight line, a long edge bends
  at its zero-width dummies.

The reported runtime is close to linear in the sum over the levels of
the squared level sizes; a map of 96 vertices took under 5 s in 1991.

### Feng 1997

A clustered graph C = (G, T) with an undirected graph G and an inclusion
tree T whose leaves are the vertices. The drawing conventions: every
cluster is a simple closed region containing exactly its sub-clusters,
an edge inside a cluster stays inside, an edge leaving a cluster crosses
its boundary exactly once; a drawing is c-planar when there are neither
edge crossings nor edge-region crossings. The layered construction uses
a c-st numbering - every cluster occupies a contiguous band of layers -
and the orthogonal construction adds four side dummies per cluster,
bundling the outgoing edges at its top dummy and the incoming edges at
its bottom dummy. The thesis warns that straight-line convex drawings
need exponential area and that one cluster per band leaves no vertical
compaction. The planarity assumption, the undirected model and the edges
that cannot end on a cluster do not fit state machines.

### ELK Layered and the SCCharts practice

ELK Layered runs five phases - cycle breaking, layer assignment, crossing
minimization, node placement, edge routing - joined by intermediate
processors: the long-edge splitter, the label dummy inserter, the
self-loop pre/post processors, the layer-constraint processors (FIRST /
LAST), the model-order sorter. The SCCharts synthesis lays every region
out with Layered on its own, packs the regions of a state with the
rectangle packer, lays the tree out bottom-up (children first, a
composite becomes a sized box in its parent) and alternates the direction
with the depth. Its options that carry over: the initial state pinned to
the first layer, the model order (the order of the states and
transitions in the source) as the tie-breaker of the cycle breaking and
of the ordering, the transition label in the tail layer of its edge, a
self-loop spacing of 18. In that mode ELK drops the edges that cross
nesting levels; its global mode (`INCLUDE_CHILDREN`, the 1991 method
brought up to date) has known weaknesses with per-level directions and
layer constraints. A user study on 265 statecharts (Domrös et al. 2023)
rated the model-order cycle breaker best and asked for the initial state
in the first layer and the final states in the last.

## The Readability Criteria

The layout target is the set of structural rules below, in priority
order, adapted from R1-R5 to state machines; each rule has a measure a
test can check on the output.

```
    +----+------------------------------------------+--------------------------+
    |    | rule                                     | measure                  |
    +----+------------------------------------------+--------------------------+
    | H1 | containment: a child inside its parent   | escapes = 0, overlaps = 0|
    |    | plus padding, siblings disjoint          |                          |
    | H2 | flow: the initial in the first layer,    | forward-edge fraction    |
    |    | the finals in the last, transitions run  | (non-loop transitions    |
    |    | forward along the flow axis              | with source layer <      |
    |    |                                          | target layer)            |
    | H3 | few crossings between transitions        | crossing count           |
    | H4 | no transition through an unrelated state | line-rect crossings = 0  |
    |    | (one border crossing per transition per  |                          |
    |    | state on the nesting path)               |                          |
    | H5 | straight one-layer transitions, long     | bends per transition     |
    |    | ones with few bends                      |                          |
    | H6 | connected states adjacent (R1)           | mean transition length   |
    | H7 | balanced attachment (R5)                 | -                        |
    | H8 | compact: content area, aspect ratio      | area, width / height     |
    +----+------------------------------------------+--------------------------+
```

The original coordinates of the document take no part in any rule.

## The Layout Model

### The frame

The algorithm works in an abstract frame with a *flow* axis (the layers
follow each other along it) and a *cross* axis (the order within a
layer). A frame is mapped to the document coordinates by the direction
of its container: `DOWN` maps flow to y and cross to x, `RIGHT` maps flow
to x and cross to y. The *preferred* frame alternates with the nesting
depth of the container: the state machine - level 0, its border never
counts as a level - prefers `RIGHT` (a wide box, the screen is wider
than tall), a composite state at depth 1 prefers `DOWN` (it sits in a
column of the machine), a composite at depth 2 `RIGHT` again. A region
is transparent: its content takes the frame of the composite that owns
it, and the depth counts composites only.

The frame actually used is chosen by the content (the adaptive mode, the
default): the layout is bottom-up, so a container cannot see its future
slot, but it can be laid out in both frames and compared with the target
shape of its depth - `aspect` (1.6) for a wide preference, `1/aspect` for
a tall one. The frame whose rect ratio is closer to the target on the log
scale wins, a tie takes the preferred frame. A long chain therefore
becomes a row at the machine level and a column inside a composite, while
a wide diamond inside a composite keeps the frame that makes it squarer.
The composite chooses one frame for all its regions from their stacked
extents. On top of that the start is decided by the result: the whole
tree is laid out with both starts (wide, tall, wide ... and tall, wide,
...) and the layout whose final machine shape is closer to the wide
target stays. The caller may instead fix one direction everywhere or
keep the plain alternation from a chosen start (`HTLayoutMode`).

```
    SM (DOWN)                                   depth 0: flow = y
    +-------------------------------------------+
    | (o)                                       |
    |  |                                        |
    |  v                                        |
    | +----+      +---------------------------+ |
    | | A  | ---> | B (RIGHT)                 | |  depth 1: flow = x
    | +----+      |                +--------+ | |
    |             | (o)->[b1]----> | b3     | | |  depth 2: flow = y
    |             |                | (DOWN) | | |
    |             |                |  [c1]  | | |
    |             |                |   |    | | |
    |             |                |   v    | | |
    |             |                |  [c2]  | | |
    |             |                +--------+ | |
    |             +---------------------------+ |
    |                      |                    |
    |                      v                    |
    |                     (x)                   |
    +-------------------------------------------+
```

### The layout graph of a container

Every container - the state machine, a region, a composite state without
regions - gets its own layout graph, the local hierarchy of the 1991
method:

* the vertices are the direct children that take part in the layout:
  states, submachine states, pseudostates (by their role, below); the
  comments are not vertices (they are placed after the layout);
* an edge is a transition between two vertices; a transition whose ends
  lie deeper in the tree is *lifted*: with R the container, u' and v' the
  children of R that contain the source u and the target v, the edge
  u' -> v' enters R's graph (u' or v' may be u or v itself); a lifted
  edge with u' = v' and a transition u -> u are self-loops - they leave
  the graph and return at the routing;
* a transition between a child of R and R itself (or an ancestor of R)
  leaves R's graph through one of two virtual vertices: `ENTRY` before
  the first layer collects the transitions coming from outside, `EXIT`
  after the last layer collects the transitions going outside (Feng's
  bundling at the two flow sides); the virtual vertices have no size and
  take part in the layering and the ordering only;
* the roles pin the vertices: the *source* roles (initial, entry point,
  shallow and deep history) go to the first layer, the *sink* roles
  (final, terminate, exit point) to the last layer; the choice, fork and
  join are ordinary small vertices; a submachine state is an ordinary
  vertex with its preset size.

```
    R                                          layout graph of R
    +---------------------------------+
    | (o)   +---------+   +--------+  |        (o) --> A --> B --> (x)
    |  |    | A       |   | B      |  |         |            ^ lifted a2 -> b1
    |  +--> | [a1]    |   | [b1]   |  |         +--------> B
    |       |  |      |   |  ^     |  |
    |       |  v      |   |  |     |  |        layout graph of A
    |       | [a2] ---+---+--+     |  |
    |       +---------+   +--------+  |        ENTRY --> a1 --> a2 --> EXIT
    |                        |        |
    |                        v        |        layout graph of B
    |                       (x)       |
    +---------------------------------+        ENTRY --> b1 --> EXIT
```

The lifting keeps every container's ordering aware of the transitions
that cross its border, which is what the per-region ELK mode loses;
the per-container recursion keeps the algorithm simple and every
container contiguous, which is what the global 1991 mode pays for.

### The regions of a composite

A composite with regions has no layout graph of its own: each region is
a container with its own graph and frame (the direction of the
composite), and the regions are stacked along the cross axis of that
frame, each a band across the full content width, in the document order
of the regions.

```
    composite (content RIGHT)          cross axis
    +------------------------------+     |
    | +--------------------------+ |     |   region 1: ENTRY..layers..EXIT
    | | (o) -> [ ] -> [ ] -> (x) | |     v
    | +--------------------------+ |
    | +--------------------------+ |
    | | (o) -> [ ] -> [ ]        | |         region 2
    | +--------------------------+ |
    +------------------------------+ ---> flow axis
```

### The dummies

A proper layered graph is built before the ordering, as in both
Sugiyama methods and in ELK:

* an edge spanning more than one layer is split, one *edge dummy* per
  crossed layer; the dummies have no cross-axis size but keep a slot and
  become the bends of the polyline;
* a transition with a preset label rect gets one *label dummy* on the
  layer after its source (the tail layer, the SCCharts choice); the
  dummy has the label's size, so the label gets its own room in the
  layer band; a one-layer transition with a label spans two layers after
  the insertion, so its target is layered accordingly.

## The Algorithm

```
    htree_reconstruct_document_geometry(doc, reconstruct_sm, layout)
      |
      v
    clean the node and edge geometry (the preset sizes kept)
      |
      v  per tree, per container, children first (P1)
    +----------------------------------------------------------------+
    |  P2 cycle breaking      DFS from the sources, back edges reversed|
    |        |                                                         |
    |  P3 layering            longest path, sources FIRST, sinks LAST  |
    |        |                                                         |
    |  P4 proper layering     edge dummies, label dummies              |
    |        |                                                         |
    |  P5 ordering            barycentre sweeps, phase 1 + phase 2     |
    |        |                                                         |
    |  P6 coordinates         priority layout on the cross axis,       |
    |        |                bands on the flow axis                   |
    |  P7 regions             bands along the cross axis               |
    |        |                                                         |
    |  P8 container fit       content + padding + title block          |
    +----------------------------------------------------------------+
      |
      v  per tree
    P9 edge routing, P10 comments, the SM border, the bounding rect
      |
      v
    canonical form (absolute, border ends), to-format (the saved formats)
```

### P0 the sizes

Every vertex has a size before its container is laid out: a leaf state
takes its preset minimum rect (the size of its name and actions block,
given by the caller) or `NODE_WIDTH` x `NODE_HEIGHT`; a pseudostate
takes the size of its glyph (`POINT_SIZE`, a constant of the library);
a composite takes the size computed by its own layout (P8). The preset
sizes are minimums: the layout never shrinks them.

### P1 the recursion

The containers are processed children first (LayoutLocal of the 1991
method): the containers nested in a composite's regions, then the regions
in the composite's frame, then the composite, then its parent container.
A container is laid out at the origin of its own frame; the finished
subtree is shifted into place when the parent assigns its slot
(`htree_shift_subtree`). The frame of a container is chosen from its
content against the target of its depth (the frame section); P2-P5 are
frame independent and run once, P6 runs per candidate frame. The entry
and exit points of a container are placed on its border by its parent,
whose frame the crossing transitions follow.

### P2 the cycle breaking

A depth-first search over the layout graph in model order: it starts
from the source-role vertices in the child list order, then from the
remaining vertices with in-degree 0, then from any vertex not yet
visited, all in the child list order; the outgoing edges are followed in
the edge list order. An edge to a vertex on the search stack is a back
edge and is reversed for the layering and the ordering; the reversal is
undone at the routing, so the drawn arrow keeps its direction (the
"reversed edge" of the 1991 method drawn upward). The strategy is ELK's
depth-first breaker with the model-order tie-break, the second-best
strategy in the SCCharts study and the one that needs no priorities from
the caller. Linear in the graph size.

### P3 the layering

The longest-path layering: a vertex without predecessors is in layer 1,
every other vertex one layer after its furthest predecessor. The
constraints follow: when the graph has source-role vertices, only they
stay in layer 1 (a virtual edge from each source-role vertex to every
other layer-1 vertex pushes the rest to layer 2); the sink-role vertices
move to a common last layer after the longest path is computed. `ENTRY`
is layer 0 and `EXIT` the layer after the last one. The longest path is
the compact choice (few layers, the flow axis short); the network
simplex layering of ELK is the recorded upgrade when the layouts turn out
too wide. Linear.

### P4 the proper layering

The edge and label dummies of the model section are inserted; the model
order of a dummy is the order of its edge, so the sweeps start from a
deterministic state. Linear in the number of dummies (bounded by the
edges times the layers).

### P5 the ordering

The initial order in every layer is the model order (the child list
order for the vertices, the edge list order for the dummies). Then the
1981 BC method: phase 1 sweeps DOWN and UP with the barycentre over the
fixed neighbour layer, a stable sort, a permutation kept only when the
total crossing count decreases, stop on a recurring order or after
`LAYOUT_SWEEPS` sweeps; phase 2 reverses the equal-barycentre blocks and
re-runs phase 1 when that changes the neighbour's barycentre order. The
splitting method of the 1991 paper (the vertices with outside
connections pinned to the layer ends) is not needed: the virtual
vertices `ENTRY` and `EXIT` sit at the cross-axis centre of their
layers, take part in the barycentres like any vertex and pull their
partners toward the centre, which shortens the border crossings (R1).
`O(sweeps * |E| log |V|)`.

### P6 the coordinates

The cross-axis positions come from the priority layout method with the
1991 initialisation: a layer is packed from its start with the gap
`NODE_GAP` between the neighbours, each vertex centred in its slot; the
improvement passes run DOWN (layers 1..n), UP (n-1..1), DOWN (t..n) with
t the middle layer; in a pass every vertex aims at the metrical
barycentre of its neighbours in the fixed layer (the mean of their
centres) with the weight of its priority: the dummies above every
connectivity (so the long edges get straight), the vertices by their
connectivity, an unconnected vertex barely weighted at its place. The
pass is the weighted least-squares form of the priority rule: the layer
order and the gaps are kept by pooling the adjacent violators, so the
neighbours of a pulled vertex move by the least amount and two siblings
pulled to one point settle symmetrically around it. The flow-axis
positions are bands: the band of a layer has the size of its largest
vertex, the bands are separated by `LAYER_GAP`, and the label dummies
enlarge the band of their layer like any vertex. A vertex is centred
in its band on the flow axis. Quadratic in a layer in the worst case.

### P7 the regions

The regions of a composite are laid out by P2-P6 and P8 each, then
stacked along the cross axis of the composite's frame with the gap
`NODE_GAP`; every region band is widened to the widest one on the flow
axis, so the region borders align.

### P8 the container fit

The content rect of a container is the union of its children's rects
and of its label dummies. The container rect is the content plus
`PADDING` on every side plus the title block at the top of the rect
(the name and actions block is drawn there whatever the flow direction):
the preset minimum rect of a composite gives the size of that block, and
the content starts below it; without a preset the title block is
`PADDING` high. The content is finally shifted so that
the container's origin is (0, 0) in its frame (the rescale of the 1991
method). A composite whose min rect is wider than the content is widened
to it, the content centred. The explicit SM border, when present or
requested, follows the same rule through `htree_grow_sm_border`.

### P9 the edge routing

The routing works in the document coordinates after every container is
placed:

* a transition between adjacent layers is a straight segment attached to
  the borders by `htree_attach_edge_minimal` (the band-perpendicular /
  45-degree attachment of 1.0.6), with `htree_project_edge_to_borders`
  as the fallback;
* a transition with edge dummies is a polyline through the centres of
  its dummies, the ends attached to the borders as above from the first
  and the last bend; at a label dummy the line runs along the slot's
  cross-axis start and the label rect fills the rest of the slot;
* a self-loop is the side loop of 1.0.6 on the side facing away from the
  flow (the cross-axis end of the state, so it does not enter the
  routing lanes between the layers);
* a transition that crosses a state border - a lifted edge in one or more
  containers - is routed level by level: from the source to the border
  of the lifted source vertex (the crossing point is the attachment
  toward the next hop), then in the container's frame as a lifted edge,
  then from the border of the lifted target vertex to the target, one
  crossing per border on the nesting path (H4); the border crossing of
  an entry or exit point is the point itself, which is placed on the
  parent's border at the `ENTRY` / `EXIT` end of the parent's frame;
* a reversed edge is routed on its reversed geometry with the end
  points swapped back;
* the straight transitions between one pair of states (in either
  direction) would coincide: they are spread across the flow axis by two
  paddings each;
* the label rect is the label dummy's rect at the dummy's slot; no label
  point is produced (a label carries either the point or the rect); a
  transition without a preset label rect gets no label geometry.

### P10 the comments

A comment linked to an element is placed beside that element on its
free side (the cross-axis end with no neighbour, else after the last
layer), with the `NODE_GAP`; an unlinked comment is shelf-placed after
the content of its container. Comments enlarge neither the containers
nor the SM border (the 1.0.6 rule) and their links are not routed.

## The Interface

The public header changes; the preserving fill-in keeps its behaviour
under a `NULL` layout:

```
    typedef enum {
        htRoleNone = 0,
        htRoleInitial, htRoleFinal, htRoleTerminate,
        htRoleEntryPoint, htRoleExitPoint,
        htRoleShallowHistory, htRoleDeepHistory,
        htRoleChoice, htRoleFork, htRoleJoin,
        htRoleSubmachine
    } HTNodeRole;

    typedef struct _HTreeNode {
        ...
        HTNodeRole      role;        /* the layout role, htRoleNone default */
        HTreeRect*      min_rect;    /* the preset minimum size (x, y ignored) */
    } HTreeNode;                     /* layout_rank is removed */

    typedef enum { htFlowDown = 0, htFlowRight } HTFlowDirection;

    typedef enum { htLayoutFixed = 0, htLayoutAlternate, htLayoutAdaptive } HTLayoutMode;

    typedef struct {
        HTFlowDirection direction;   /* the root frame, the preferred start */
        HTLayoutMode    mode;        /* adaptive by default */
        double          aspect;      /* the target width / height of a wide box */
        double          node_gap;    /* NODE_GAP */
        double          layer_gap;   /* LAYER_GAP */
        double          padding;     /* PADDING */
        int             sweeps;      /* LAYOUT_SWEEPS */
        double          node_width;  /* the size defaults, so a caller can */
        double          node_height; /* combine its own estimates with them */
        double          point_size;
        double          label_width;
        double          label_height;
    } HTLayoutOptions;

    void htree_default_layout_options(HTLayoutOptions* opts);
    void htree_node_set_min_size(HTreeNode* node, double w, double h);
    int  htree_reconstruct_document_geometry(HTDocument* doc, int reconstruct_sm,
                                             const HTLayoutOptions* layout);
```

A value of zero or less in the options selects the library default.
`HTreeEdge` keeps `label_rect`: when the caller presets it before the
layout, its width and height size the label dummy (a zero size means the
default label size); without a preset the transition gets no label
geometry. The regions are passed as `htRegion` nodes. The caller knows
the texts, so it presets the sizes: the libcyberiadaml bridge estimates
the state text blocks and the transition labels from the titles and the
actions with fixed character metrics.

## The Implementation

```
    htgeom.h                    the roles, the options, min_rect
       |
    htgeom_internal.h           the helpers shared by the two sources
       |                 |
    htgeom.cpp           htgeom_layout.cpp
    the fill-in,         the containers and the seeds, P2-P8 per
    the attachment,      container, the routing, the comments,
    the loop, the        htree_layout_tree
    SM border
```

`htree_reconstruct_document_geometry` dispatches on the options pointer:
`NULL` runs the preserving fill-in of `htgeom.cpp`, an options struct
runs `htree_layout_tree` of `htgeom_layout.cpp` for every tree. The
tests 25-36 (`tests/layout-check.h` holds the builders and the
readability checks H1-H4) cover the cases of the testing section.

## Determinism and Complexity

The algorithm has no random choice: every tie is broken by the model
order, the sweeps start from it and their count is a constant. The same
document yields the same geometry on every platform (the coordinates are
doubles rounded by the consumer as today). The phases are linear in the
layout graph except the ordering, `O(sweeps * |E| log |V|)` per
container, and the priority layout, quadratic in the size of a layer;
the memory is the layout graph with its dummies, freed per container.

## Testing

The library tests follow the current scheme (one executable per test,
its printed document compared with the recorded output), one per
structural case, each asserting the measures of the criteria section
on the result before printing it:

* 25 a chain (initial -> A -> B -> final): the flow order, straight
  edges, forward fraction 1;
* 26 a cycle (A -> B -> C -> A): one backward edge, no crossing;
* 27 a diamond (A -> B, A -> C, B -> D, C -> D): no crossing, D centred;
* 28 a long edge (A -> D over three layers) with a label: two bends,
  the label rect in the tail layer beside the states;
* 29 a composite with two regions: the region bands aligned, stacked
  with the gap below the title block;
* 30 a cross-level transition: one crossing per composite border, no
  line-rect crossing;
* 31 a self-loop: the loop on the cross-axis end, DOWN and RIGHT;
* 32 the alternation: a composite inside the SM laid out `RIGHT`, its
  child composite `DOWN`, the alternation off, the `RIGHT` root;
* 33 the preset sizes: a min rect never shrunk, a wide title block
  widening its composite, the content centred;
* 34 comments: beside the linked element, the link unrouted, the
  unlinked comment on the shelf;
* 35 the degenerate documents: empty, points only, a single state, loops
  only, cycles without roles, parallel edges, no root rect, a submachine
  with its border points, a top-level list;
* 36 the adaptive direction: the machine-level chain runs right with and
  without the border and as a top-level list, a chain inside a composite
  makes the two starts compete, a single state takes the preferred frame,
  a wide diamond takes the squarer one, the fixed mode, the option checks.

The editor's polygon corpus is re-run in the `reconstruct` mode with the
render-soundness checks and the readability measures; the closeness
rate is removed from its report.

## Open Questions

* the network simplex layering when the longest path gives too wide
  layers;
* a slot-aware direction: a second pass could re-lay a composite out for
  the slot its parent finally gave it;
* the Brandes-Koepf placement in place of the priority method for
  straighter long edges;
* the orthogonal routing in channels for the dense diagrams;
* separate connected components placed side by side instead of one
  layering;
* the submachine state contents (not laid out - a sized box);
* the glyph shapes of the choice, fork and join (a small rect today).
