# libhtgeom project changelog

## Version 1.2.0 (unreleased)

Added:
- the geometry update `htree_update_document_geometry`: the layered layout
  seeded by the existing drawing, which is kept.

Fixed:
- the update: the bends and the labels of the new transitions inside a
  drawn container are no longer shifted twice by its origin.
- the update: a new container and a new region get the title band and the
  insets of the full layout.
- the update: the drawn vertices stay pinned when new vertices or bends
  share their layer, and a new line through a drawn layer runs along the
  whole band instead of cutting the corner of a drawn neighbour.
- the update: a new state that closes a cycle through the drawing is placed
  after it, the new edge back is the reversed one; a new transition between
  drawn states of different bands is routed around the states between them.
- the entry and exit points of a machine (not of a nested state) are laid
  out as vertices of the machine; they had no geometry.

## Version 1.1.1 (unreleased)

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
