# The libhtreegeom Architecture

The library implements the geometry model of hierarchical graphs used by
the CyberiadaML standard: trees of rectangular nodes connected by edges
with polylines and labels. It converts the geometry between the coordinate
formats of the CyberiadaML-GraphML serialization, computes bounding
rectangles and reconstructs missing geometry. The interface is pure C
(`htgeom.h`); the implementation is C++17 and depends only on the vendored
header-only homog2d library.

```
     libcyberiadaml/geometry.c
    +------------------------+
    |  model <-> htree       |
    |  bridge                |
    +------------------------+
                |
                |  C calls
                v
     htgeom.h (extern "C")
    +------------------------------------+
    |  public C API                      |
    +------------------------------------+
         |                          |
         | lifecycle                | transforms
         v                          v
     htgeom_types.cpp           htgeom.cpp                 homog2d.hpp
    +------------------------+ +------------------------+ +------------------------+
    |  type layer:           | |  transform layer:      | |  vendored geometry     |
    |  alloc/copy/set/       | |  format pipelines,     | |  engine: getBB,        |
    |  destroy/round/print   | |  bounding rect,        | |  segment x rect        |
    |                        | |  reconstruction        | |  intersection          |
    +------------------------+ +------------------------+ +------------------------+
                ^                   |            |                   ^
                |                   |            |                   |
                +---- alloc/free ---+            +---- geometry -----+
```

## The Public Interface

`htgeom.h` is the single public header, wrapped in `extern "C"`. It
declares the data types and four function groups:

* lifecycle - `htree_new_*` / `htree_copy_*` / `htree_set_*` /
  `htree_destroy_*` for points, rects, polylines, nodes, edges, trees and
  documents, plus `htree_round_*` and `htree_print_*` helpers;
* tree building - `htree_add_sibling_node`, `htree_add_child_node`,
  `htree_add_node`, `htree_add_edge`, `htree_add_tree`,
  `htree_find_node_by_id`;
* predicates - `htree_node_has_geometry`,
  `htree_node_has_toplevel_geometry`, `htree_tree_has_geometry`,
  `htree_empty_rect`;
* transforms - `htree_build_bounding_rect`,
  `htree_convert_document_geometry`,
  `htree_reconstruct_document_geometry`, `htree_check_geometry`.

The functions return `HTREE_OK`, `HTREE_BAD_PARAMETER`, `HTREE_NOT_FOUND`
or `HTREE_GEOMETRY_TRANFORM_ERROR`.

## The Data Model

```
    HTDocument
      |  node_coord_format, edge_coord_format,
      |  edge_pl_coord_format, edge_format, bounding_rect
      |
      +-- trees --> HTree --next--> HTree --> ...
                     |
                     +-- nodes --> HTreeNode --next--> ...
                     |              |  id, type, point | rect
                     |              +-- children --> HTreeNode ...
                     |
                     +-- edges --> HTreeEdge --next--> ...
                                    id, source/target ids and nodes,
                                    source/target points, polyline,
                                    label point, label rect
```

A node carries either a point (`htPoint`, e.g. an initial pseudostate) or
a rect (`htSimpleNode`, `htCompositeNode`, `htRegion`); `htTree` is the
root of one state machine. Edges reference their endpoints by id; the
consumer resolves `source`/`target` pointers with `htree_find_node_by_id`
before transforming.

## The Geometry Formats

The document stores one coordinate format for nodes, one for edge points
and one for edge polylines - `coordAbsolute`, `coordLeftTop` (relative to
the parent's left-top corner) or `coordLocalCenter` (relative to the
parent's center); `coordNone` means no geometry. Edge endpoints are bound
either to node centers (`edgeCenter`) or to node borders (`edgeBorder`).

The canonical internal form is absolute coordinates with `edgeBorder`
endpoints: every transform first normalizes the document to it, then
derives the requested formats from it. When a document has no single
top-level node with geometry, the document bounding rect serves as the
implicit parent for `coordLocalCenter` conversion
(`htree_has_toplevel_rect`).

## The Transform Layer

```
    htree_convert_document_geometry     htree_reconstruct_document_geometry
    -------------------------------     -----------------------------------
      to-absolute                         to-absolute
           |                                   |
           v                                   v
      rebuild bounding rect               reconstruct nodes
           |                              (edges: stub)
           v                                   |
      to-format (new formats)                  v
                                          rebuild bounding rect
                                               |
                                               v
                                          to-format (saved formats)
```

The to-absolute pass converts nodes top-down (a parent's converted rect
becomes the children's reference), then edge source/target points, then
projects `edgeCenter` endpoints onto the node borders (segment x rect
intersection via homog2d), then edge labels. The to-format pass mirrors
it: labels, then edge points, then nodes children-first, then the
bounding rect itself. Polyline points are always relative to the source
node. A special case ("the yEd hack") binds edge labels to the edge
source point instead of the source node for the
absolute/local-center/absolute/`edgeCenter` format combination.

Reconstruction gives every geometry-less node a default rect
(`NODE_WIDTH` x `NODE_HEIGHT` at parent offset `PADDING`) or point, and
rebuilds composite parents' rects from their children's bounding box.

## The homog2d Integration

`htgeom.cpp` wraps homog2d behind small adapters -
`htree_point_to_homog`, `htree_rect_to_homog`, `htree_polyline_to_homog`
and the reverse `homog_*_to_htree` functions. The engine is used for two
things only: bounding boxes (`h2d::getBB` over collected points, rects
and polylines) and edge border projection (`h2d::Segment::intersects`
with a rect). The document bounding rect unites the node rects and
points with the edge geometry of resolved edges: polylines (including
their source/target endpoints) and label points and rects; straight-edge
source/target points alone do not extend it. A tree whose single root is
an `htTree` node with a rect (the explicit SM border on the diagram)
contributes exactly that rect - the border wins over the content. A
well-formed diagram never places content outside its SM border;
`htree_check_geometry` reports such documents as
`HTREE_GEOMETRY_INVALID`, while the bounding rect stays tolerant. `homog2d.hpp` is header-only, so the shared library links
nothing.

## The Consumer Contract

`libcyberiadaml/geometry.c` is the only direct consumer; `cyberiadaml.h`
aliases the geometry types (`CyberiadaPoint` = `HTreePoint` etc.), so all
CyberiadaML clients use them transitively. On import, export or explicit
conversion the consumer builds an `HTDocument` copy of the state-machine
geometry, resolves edge endpoints, calls
`htree_convert_document_geometry` (optionally preceded by
`htree_reconstruct_document_geometry`), copies the resulting geometry
back into the model and destroys the document.

## Building and Testing

CMake builds the shared `htgeom` library from the two `.cpp` files and
one `<NN>-<name>.test` executable per `tests/*.cpp` file, registered
with CTest. Each test prints its document through the `operator<<`
overloads of `htgeom_types.h`; `cmake/RunDiffTest.cmake` runs the
binary and byte-compares its stdout with `tests/<NN>-output.txt`. The
suite is run with `ctest --output-on-failure` (or `run-tests.sh`, which
rebuilds in Debug mode first). Test 01 covers the empty document, test
02 a full tree with edges, tests 03-09 the bounding rect cases (empty
and degenerate documents, node unions, edge polylines, loop edges,
labels, the explicit SM border), test 10 the geometry validity check.
No test exercises format conversion or reconstruction.

## Known Gaps

* `htree_reconstruct_edges_geometry` (htgeom.cpp:1389) is an empty stub -
  reconstruction never creates edge points, polylines or labels.
* The border-to-center edge conversion is disabled:
  `htree_convert_edges_geometry_to_format_center` is a commented-out,
  non-compiling draft (htgeom.cpp:1111-1209) and its call site is
  commented out too (htgeom.cpp:1286-1293), so converting to `edgeCenter`
  updates the `edge_format` field while the points stay on the borders.
* `htree_print_point` is declared for `const HTreePoint*` (htgeom.h:135)
  but defined for `const HTreeRect*` (htgeom_types.cpp:148); the declared
  C symbol is never defined and the body prints the pointer value.
* `htree_polyline_to_homog` skips the vertex of a single-point polyline
  (htgeom.cpp:107) — only the source and target points enter the
  bounding-rect collection.
* `htree_compare_rects` (htgeom_types.cpp:209) is implemented but not
  declared in `htgeom.h`.
* The public entry points ignore the return codes of the internal
  conversion passes and always return `HTREE_OK`.
* `htree_build_bounding_rect` reads `*result` before assigning it
  (htgeom.cpp:490, 507), so the caller must pass a NULL-initialized
  pointer; the library does not guard against garbage input.
