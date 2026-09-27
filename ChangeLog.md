# libhtgeom project changelog

## Version 1.1.0 (unreleased)

Added:
- the layered layout of the full geometry reconstruction
  (`docs/reconstruction.md`, `htgeom_layout.cpp`);
- the node roles (`HTNodeRole`) and the preset minimum sizes (`min_rect`,
  `htree_node_set_min_size`);
- the layout options (`HTLayoutOptions`, `htree_default_layout_options`);
- the layout tests 25-35.

Changed:
- `htree_reconstruct_document_geometry` takes the layout options instead
  of the ordering flag (`NULL` keeps the preserving fill-in);
- the reading-order placement and `layout_rank` are removed.

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
