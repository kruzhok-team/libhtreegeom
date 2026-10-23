# libhtgeom project changelog

## Version 1.3.0 (unreleased)

Added:
- the comment links: the edge kind `HTEdgeType` and the commented edge of a link
  to a transition (`htree_edge_set_link`, `htree_find_edge_by_id`); the
  reconstruction attaches the links to the borders, a link to a transition on its
  path off its arrow and its label, a link to a fragment without a target point
  (PNST 1044 9.1.1); the update
  keeps the drawn links and fills only the missing ends.

## Version 1.2.0

Added:
- the geometry update `htree_update_document_geometry`: the layered layout
  seeded by the existing drawing, which is kept.

Fixed:
- multiple reconstruction algorithm updates;
- correct the entry and exit points geometry.

## Version 1.1.1

Added:
- a document with several machines: every machine gets its border and the
  machines are placed in a row.

Fixed:
- the label rect of a transition from a point is centre-based in the
  local-centre format like the others.

## Version 1.1.0

Added:
- the layered layout of the full geometry reconstruction
- the node roles (`HTNodeRole`) and the preset minimum sizes (`min_rect`,
  `htree_node_set_min_size`);
- the layout options (`HTLayoutOptions`, `htree_default_layout_options`);
- the adaptive flow direction (`HTLayoutMode`, the target aspect ratio): the
  machine level prefers left-to-right, the content chooses the frame;
- `htree_reconstruct_document_geometry` takes the layout options instead
  of the ordering flag (`NULL` keeps the preserving fill-in);

## Version 1.0.6

Fixed:
- bounding rect calculation was updated;
- the geometry transformation errors are not propagated to the caller;
- the edge geometry reconstruction implemented;
- `HTREE_GEOMETRY_TRANSFORM_ERROR` spelling was corrected.

Added:
- added the `htree_check_geometry` geometry validator;
- the border to center edge conversion (the yEd export);
- the preserving geometry reconstruction;
- `htree_compare_rects` in the public interface;
- the CMake package configuration (`lib/cmake/htgeom`) beside the finder.

## Version 1.0

Stable version of the library.

Known issues:
- the yEd export was limited;
- the edge reconstruction was broken.
