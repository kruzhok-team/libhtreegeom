# libhtgeom project changelog

## Version 1.3.0 (unreleased)

Added:
- the choice glyph: a choice is a square of `HTLayoutOptions::choice_size`
  (40, the editor's default) instead of a state box, the larger side of its
  preset or seed; a transition end on a choice is the tip of its rhombus
  facing the other end (tests 59, 60);
- the arrange (`htree_arrange_document_geometry`): the layered layout anew,
  seeded by the existing geometry - the bands read in the seed's own
  extents, the order in a layer and the flow directions kept, the seed rects
  as the minimum sizes; the places, the routes with their label dummies and
  the comments of the full layout, nothing pinned (tests 54-58); the pull
  (`HTLayoutOptions::seed_pull`): the seeded vertices at their seed places
  scaled to the least distances, the rows drawn askew in line (test 61);
- the orthogonal routing (`HTLayoutOptions::routing`, `htRoutingOrthogonal`;
  the polylines stay the default): the ends spread on the sides facing the
  flow, the turns in the channels of the gaps, the joins and the ends turned
  square clear of the nodes (tests 62-65); the arrange keeps the drawn sides
  of the ends along the flow (test 66);
- the comment links: the edge kind `HTEdgeType` and the commented edge of a link
  to a transition (`htree_edge_set_link`, `htree_find_edge_by_id`); the
  reconstruction attaches the links to the borders, a link to a transition on its
  path off its arrow and its label, a link to a fragment without a target point
  (PNST 1044 9.1.1); the update
  keeps the drawn links and fills only the missing ends; a link to a choice
  ends anywhere on the border of its rhombus.

Changed:
- the comments are content of the layered layout: a linked comment is the
  margin of its anchor (the vertex of its own container holding the subject,
  stacked along the flow), the others form the shelf band after the content;
  the containers and the machine border grow around them (no comment outside
  its container, no overlap); the update places a new comment on the first
  spot beside its anchor clear of every drawn node, route and label, else on
  the shelf after the drawn content, and grows the containers over it.

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
