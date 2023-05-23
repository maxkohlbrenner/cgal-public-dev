/*!
\ingroup PkgMesh3SecondaryConcepts
\cgalConcept

The concept `TriangleAccessor_3` represents an accessor to a triangulated polyhedral
surface, intersection free and without boundaries.

\cgalHasModel `CGAL::Triangle_accessor_3<Polyhedron,,K>`

\sa `CGAL::make_mesh_3()`

*/

class TriangleAccessor_3 {
public:

/// \name Types
/// @{

/*!
Triangle type. Must be a model of `DefaultConstructible` and
`CopyConstructible`.
*/
typedef unspecified_type Triangle_3;

/*!
Triangle iterator type. Must be a model of `InputIterator`.
*/
typedef unspecified_type Triangle_iterator;

/*!
%Handle to a `Triangle_3`. Must be
constructible from `Triangle_iterator`. It may be `Triangle_iterator` itself.
*/
typedef unspecified_type Triangle_handle;

/*!
Polyhedron type which must be a model of `FaceGraph`.
*/
typedef unspecified_type Polyhedron;

/// @}

/// \name Operations
/// @{

/*!
Returns a `Triangle_iterator` to visit the triangles of polyhedron `p`.
*/
Triangle_iterator triangles_begin(Polyhedron p);

/*!
Returns the past-the-end iterator for the above iterator.
*/
Triangle_iterator triangles_end(Polyhedron p);

/*!
Returns a `Triangle_3`
object from handle `h`.
*/
Triangle_3 triangle(Triangle_handle h);

/// @}

}; /* end TriangleAccessor_3 */
