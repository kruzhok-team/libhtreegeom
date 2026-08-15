# The libhtreegeom Interface Specification

The public interface is the C API of `htgeom.h`. The functions return
`HTREE_OK` (0) on success, `HTREE_BAD_PARAMETER` (1) for invalid
arguments or formats, `HTREE_NOT_FOUND` (2), `HTREE_GEOMETRY_TRANSFORM_ERROR`
(3, with the deprecated misspelled alias `HTREE_GEOMETRY_TRANFORM_ERROR`)
and `HTREE_GEOMETRY_INVALID` (4, the content escapes the explicit SM
border). The allocating functions return NULL on failure.

## Points Module

* `htree_new_point()` - allocate a zero point
* `htree_new_point_coord(x, y)` - allocate a point with the coordinates
* `htree_copy_point(src)` - allocate a copy of the point
* `htree_set_point(dst, src)` - copy the coordinates into an existing point
* `htree_round_point(p, signs)` - round the coordinates to the decimal signs
* `htree_destroy_point(p)` - free the point
* `htree_print_point(p)` - print the point to stdout
* `htree_rect_center_point(r, format)` - allocate the rect center
  (the raw origin for `coordLocalCenter`)

## Rects Module

* `htree_new_rect()` / `htree_new_rect_coord(x, y, w, h)` - allocate a rect
* `htree_copy_rect(src)` - allocate a copy of the rect
* `htree_init_rect(rect)` - zero the rect fields
* `htree_set_rect(dst, src)` - copy the fields into an existing rect
* `htree_round_rect(r, signs)` - round the fields to the decimal signs
* `htree_empty_rect(r)` - non-zero when all the fields are zero
* `htree_compare_rects(a, b)` - 0 for equal rects, 1 for different,
  -1 when either rect is NULL
* `htree_destroy_rect(r)` / `htree_print_rect(r)` - free / print the rect

## Polylines Module

* `htree_new_polyline()` / `htree_new_polyline_coord(x, y)` - allocate a
  polyline (with the first vertex)
* `htree_polyline_add_point(pl, x, y)` - append a vertex
* `htree_copy_polyline(src)` / `htree_set_polyline(dst, src)` - copy
* `htree_destroy_polyline(polyline)` - free the whole chain

## Nodes Module

* `htree_new_node(type, id)` - allocate a node of the given `HTNodeType`
* `htree_node_set_rect(node, x, y, w, h)` / `htree_node_set_point(node, x, y)` -
  set the node geometry
* `htree_add_sibling_node(node, new_node)` - append to the sibling list
* `htree_add_child_node(node, new_node)` - append a child; a simple node
  becomes composite
* `htree_copy_node(src)` - deep copy of the node subtree
* `htree_find_node_by_id(root, id)` - search the siblings and the children
* `htree_node_has_geometry(node)` - the node or a descendant carries geometry
* `htree_node_has_toplevel_geometry(node)` - exactly one top-level node
  carries geometry
* `htree_destroy_node(node)` - free the node subtree

## Edges Module

* `htree_new_edge(id, source_id, target_id)` - allocate an edge; the caller
  resolves the `source`/`target` node pointers
* `htree_edge_set_points(edge, sx, sy, tx, ty)` - set the end points
* `htree_copy_edge(src)` / `htree_destroy_edge(edge)` - copy / free

## Trees Module

* `htree_new_tree()` / `htree_copy_tree(src)` / `htree_destroy_tree(tree)` -
  lifecycle of one state machine tree
* `htree_add_node(tree, n)` / `htree_add_edge(tree, e)` - append to the tree
* `htree_tree_has_geometry(tree)` - any node or edge carries geometry

## Documents Module

* `htree_new_document(node_fmt, edge_fmt, edge_pl_fmt, edge_format)` -
  allocate a document with the given coordinate formats
* `htree_add_tree(doc, tree)` - append a state machine tree
* `htree_copy_document(src)` / `htree_destroy_document(doc)` /
  `htree_print_document(doc)` - lifecycle and printing
* `htree_build_bounding_rect(doc, result)` - the document bounding rect:
  the union of the node and edge geometry; a tree with the explicit SM
  border (a single `htTree` root with a rect) contributes exactly the border
* `htree_check_geometry(doc)` - `HTREE_GEOMETRY_INVALID` when the content
  escapes an explicit SM border (any coordinate format accepted)
* `htree_convert_document_geometry(doc, node_fmt, edge_fmt, edge_pl_fmt,
  edge_format)` - convert between the coordinate formats through the
  canonical form (absolute coordinates, border end points); `edgeCenter`
  targets get the center-projected end points
* `htree_reconstruct_document_geometry(doc, reconstruct_sm)` - the
  preserving fill-in: generate the missing geometry (shelf-placed nodes,
  straight border-projected edges, side loops), grow the authored parents
  and the SM border when the content overflows; `reconstruct_sm` allows
  creating the missing SM border rect
