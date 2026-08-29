# libhtgeom project changelog

## Version 1.0.6

Fixed:
- the edge geometry (the polylines and the labels) was excluded from the
  document bounding rect;
- a loop edge with the coinciding ends aborted the process while the
  bounding rect was built;
- a degenerate (a single point or a collinear) geometry produced an empty
  or a wrong bounding rect;
- the geometry transformation errors were not propagated to the caller:
  the conversion always reported the success;
- the conversion to the yEd edge format left the edge points on the state
  borders, so the exported diagrams were broken;
- the edge geometry reconstruction did nothing (the 1.0 known issue);
- `htree_print_point` was declared and defined with the different types.

Added:
- `htree_check_geometry` and the `HTREE_GEOMETRY_INVALID` code: the diagram
  content must fit the explicit state machine border;
- the explicit state machine border defines the document bounding rect;
- the border to center edge conversion (the yEd export);
- the preserving geometry reconstruction: the existing geometry is kept, the
  missing nodes are placed inside their parents, the missing edges get the
  border-projected attachments and the loops get the side polylines;
- `htree_compare_rects` in the public interface and the correctly spelled
  `HTREE_GEOMETRY_TRANSFORM_ERROR` (the misspelled name is kept as an alias).

## Version 1.0

Stable version of the library.

Known issues:
- the edge reconstruction was broken.
