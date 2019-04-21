#include "student_code.h"
#include "mutablePriorityQueue.h"

using namespace std;

namespace CGL
{
  void BezierCurve::evaluateStep()
  {
    // TODO Part 1.
    // Perform one step of the Bezier curve's evaluation at t using de Casteljau's algorithm for subdivision.
    // Store all of the intermediate control points into the 2D vector evaluatedLevels.
      int num_levels = evaluatedLevels.size();
      int num_points = evaluatedLevels[num_levels-1].size();
      if (num_points == 1) {  // make sure that the size of the last level is greater than 1
          return;
      }
      std::vector<Vector2D> prev_level = evaluatedLevels[num_levels-1];
      std::vector<Vector2D> new_level;
      for(int i = 0; i < num_points-1; i+=1) {
          new_level.push_back((1-t)*prev_level[i] + (t)*prev_level[i+1]);
      }
      evaluatedLevels.push_back(new_level);
  }


  Vector3D BezierPatch::evaluate(double u, double v) const
  {
    // TODO Part 2.
    // Evaluate the Bezier surface at parameters (u, v) through 2D de Casteljau subdivision.
    // (i.e. Unlike Part 1 where we performed one subdivision level per call to evaluateStep, this function
    // should apply de Casteljau's algorithm until it computes the final, evaluated point on the surface)
      std::vector<Vector3D> lines;
      int size = controlPoints.size();
      for (int i = 0; i < size; i+=1) {
          lines.push_back(BezierPatch::evaluate1D(controlPoints[i], u));
      }
      return BezierPatch::evaluate1D(lines, v);
  }

  Vector3D BezierPatch::evaluate1D(std::vector<Vector3D> points, double t) const
  {
    // TODO Part 2.
    // Optional helper function that you might find useful to implement as an abstraction when implementing BezierPatch::evaluate.
    // Given an array of 4 points that lie on a single curve, evaluates the Bezier curve at parameter t using 1D de Casteljau subdivision.
      std::vector< std::vector<Vector3D> > evaluatedLevels;
      evaluatedLevels.push_back(points);
      int num_levels = evaluatedLevels.size();
      int num_points = evaluatedLevels[num_levels-1].size();
      while (num_points != 1) {
          std::vector<Vector3D> prev_level = evaluatedLevels[num_levels-1];
          std::vector<Vector3D> new_level;
          for(int i = 0; i < num_points-1; i+=1) {
              new_level.push_back((1-t)*prev_level[i] + (t)*prev_level[i+1]);
          }
          evaluatedLevels.push_back(new_level);
          
          num_points -= 1;
          num_levels += 1;
      }
      return evaluatedLevels[num_levels-1][0]; // there should only be one element at this point
 }



  Vector3D Vertex::normal( void ) const
  {
    // TODO Part 3.
    // TODO Returns an approximate unit normal at this vertex, computed by
    // TODO taking the area-weighted average of the normals of neighboring
    // TODO triangles, then normalizing.
      
      Vector3D n(0,0,0); // initialize a vector to store your normal sum
      HalfedgeCIter h = halfedge(); // Since we're in a Vertex, this returns a halfedge
      // pointing _away_ from that vertex
      h = h->twin(); // Bump over to the halfedge pointing _toward_ the vertex.
      // Now h->next() will be another edge on the same face,
      // sharing the central vertex.
      HalfedgeCIter h_orig = h;
      do {
          Vector3D a = h->vertex()->position - h->twin()->vertex()->position;
          Vector3D b = h->next()->vertex()->position - h->next()->twin()->vertex()->position;
          n += cross(a, b);
          h = h->next()->twin();
      } while (h != h_orig);
      
      return n.unit();
  }

  EdgeIter HalfedgeMesh::flipEdge( EdgeIter e0 )
  {
      // TODO Part 4.
      // TODO This method should flip the given edge and return an iterator to the flipped edge.
      if (e0->isBoundary()) {
          return e0; // we don't flip edges here
      }
      // Collect Elements
      HalfedgeIter h0 = e0->halfedge();
      HalfedgeIter h1 = h0->next();
      HalfedgeIter h2 = h1->next();
      HalfedgeIter h3 = h0->twin();
      HalfedgeIter h4 = h3->next();
      HalfedgeIter h5 = h4->next();
      HalfedgeIter h6 = h1->twin();
      HalfedgeIter h7 = h2->twin();
      HalfedgeIter h8 = h4->twin();
      HalfedgeIter h9 = h5->twin();
      
      VertexIter v0 = h0->vertex();
      VertexIter v1 = h3->vertex();
      VertexIter v2 = h2->vertex();
      VertexIter v3 = h5->vertex();
      
      EdgeIter e1 = h1->edge();
      EdgeIter e2 = h2->edge();
      EdgeIter e3 = h4->edge();
      EdgeIter e4 = h5->edge();
      
      FaceIter f0 = h0->face();
      FaceIter f1 = h3->face();
      
      //HALFEDGES
      h0->setNeighbors(h1, h3, v3, e0, f0);
      h1->setNeighbors(h2, h7, v2, e2, f0);
      h2->setNeighbors(h0, h8, v0, e3, f0);
      h3->setNeighbors(h4, h0, v2, e0, f1);
      h4->setNeighbors(h5, h9, v3, e4, f1);
      h5->setNeighbors(h3, h6, v1, e1, f1);
      h6->setNeighbors(h6->next(), h5, v2, e1, h6->face());
      h7->setNeighbors(h7->next(), h1, v0, e2, h7->face());
      h8->setNeighbors(h8->next(), h2, v3, e3, h8->face());
      h9->setNeighbors(h9->next(), h4, v1, e4, h9->face());
      //VERTICES
      v0->halfedge() = h2;
      v1->halfedge() = h5;
      v2->halfedge() = h3;
      v3->halfedge() = h0;
      //EDGES
      e0->halfedge() = h0;
      e1->halfedge() = h5;
      e2->halfedge() = h1;
      e3->halfedge() = h2;
      e4->halfedge() = h4;
      //FACES
      f0->halfedge() = h0;
      f1->halfedge() = h3;
      
      return e0;
  }

  VertexIter HalfedgeMesh::splitEdge( EdgeIter e0 )
  {
    // TODO Part 5.
    // TODO This method should split the given edge and return an iterator to the newly inserted vertex.
    // TODO The halfedge of this vertex should point along the edge that was split, rather than the new edges.
      if (e0->isBoundary()) {
          return e0->halfedge()->vertex(); // we don't flip edges here
      }
      // Collect Elements
      HalfedgeIter h0 = e0->halfedge();
      HalfedgeIter h1 = h0->next();
      HalfedgeIter h2 = h1->next();
      HalfedgeIter h3 = h0->twin();
      HalfedgeIter h4 = h3->next();
      HalfedgeIter h5 = h4->next();
      HalfedgeIter h6 = h1->twin();
      HalfedgeIter h7 = h2->twin();
      HalfedgeIter h8 = h4->twin();
      HalfedgeIter h9 = h5->twin();
      
      VertexIter v0 = h0->vertex();
      VertexIter v1 = h3->vertex();
      VertexIter v2 = h2->vertex();
      VertexIter v3 = h5->vertex();
      
      EdgeIter e1 = h1->edge();
      EdgeIter e2 = h2->edge();
      EdgeIter e3 = h4->edge();
      EdgeIter e4 = h5->edge();
      
      FaceIter f0 = h0->face();
      FaceIter f1 = h3->face();
      // Allocate new Elements
      HalfedgeIter h10 = HalfedgeMesh::newHalfedge();
      HalfedgeIter h11 = HalfedgeMesh::newHalfedge();
      HalfedgeIter h12 = HalfedgeMesh::newHalfedge();
      HalfedgeIter h13 = HalfedgeMesh::newHalfedge();
      HalfedgeIter h14 = HalfedgeMesh::newHalfedge();
      HalfedgeIter h15 = HalfedgeMesh::newHalfedge();
      
      VertexIter v4 = HalfedgeMesh::newVertex();
      v4->position = (v0->position+v1->position) / 2.0f;
      
      EdgeIter e5 = HalfedgeMesh::newEdge();
      EdgeIter e6 = HalfedgeMesh::newEdge();
      e6->isNew = true;
      EdgeIter e7 = HalfedgeMesh::newEdge();
      e7->isNew = true;
      
      FaceIter f2 = HalfedgeMesh::newFace();
      FaceIter f3 = HalfedgeMesh::newFace();
      
      // Reassign HALFEDGES
      h0->setNeighbors(h1, h3, v4, e0, f0);
      h1->setNeighbors(h12, h6, v1, e1, f0);
      h2->setNeighbors(h15, h7, v2, e2, f3);
      h3->setNeighbors(h10, h0, v1, e0, f1);
      h4->setNeighbors(h11, h8, v0, e3, f2);
      h5->setNeighbors(h3, h9, v3, e4, f1);
      // Set neighbors for edges not in either of the two traingles do not change

      h10->setNeighbors(h5, h11, v4, e6, f1); // New
      h11->setNeighbors(h14, h10, v3, e6, f2); // New
      h12->setNeighbors(h0, h13, v2, e7, f0); // New
      h13->setNeighbors(h2, h12, v4, e7, f3); // New
      h14->setNeighbors(h4, h15, v4, e5, f2); // New
      h15->setNeighbors(h13, h14, v0, e5, f3); // New

      // Reassign VERTICES
      v0->halfedge() = h7;
      v1->halfedge() = h9;
      v2->halfedge() = h6;
      v3->halfedge() = h8;
      v4->halfedge() = h0; // New (need to return halfedge that was split)
      // Reassign EDGES
      e0->halfedge() = h0;
      e1->halfedge() = h1;
      e2->halfedge() = h2;
      e3->halfedge() = h4;
      e4->halfedge() = h5;
      e5->halfedge() = h15; // New
      e6->halfedge() = h11; // New
      e7->halfedge() = h12; // New
      // Reassing FACES
      f0->halfedge() = h0;
      f1->halfedge() = h3;
      f2->halfedge() = h4; // New
      f3->halfedge() = h2; // New
      
      return v4;
  }



  void MeshResampler::upsample( HalfedgeMesh& mesh )
  {
    // TODO Part 6.
    // This routine should increase the number of triangles in the mesh using Loop subdivision.
    // Each vertex and edge of the original surface can be associated with a vertex in the new (subdivided) surface.
    // Therefore, our strategy for computing the subdivided vertex locations is to *first* compute the new positions
    // using the connectity of the original (coarse) mesh; navigating this mesh will be much easier than navigating
    // the new subdivided (fine) mesh, which has more elements to traverse. We will then assign vertex positions in
    // the new mesh based on the values we computed for the original mesh.


      // TODO Compute new positions for all the vertices in the input mesh, using the Loop subdivision rule,
      // TODO and store them in Vertex::newPosition. At this point, we also want to mark each vertex as being a vertex of the original mesh.
      for (VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
          Vector3D avg;
          HalfedgeIter start = v->halfedge()->next();
          HalfedgeIter curr = start;
          float n = start->vertex()->degree();
          float u;
          if (n == 3) {u = 3.0f/16.0f;}
          else {u = 3.0f/(8.0f*n);}
          int i = 0;
          do {
              avg += (u * curr->vertex()->position); // make sure it is old position
              curr = curr->next()->twin()->next();
              i++;
          } while (i != n);
          v->newPosition = avg + (v->position * (1-n*u));
          v->isNew = false;
      }
      // TODO Next, compute the updated vertex positions associated with edges, and store it in Edge::newPosition.
      vector<EdgeIter> iter;
      for (EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
          HalfedgeIter h = e->halfedge();
          HalfedgeIter ht = h->twin();
          
          VertexIter v1 = h->vertex();
          VertexIter v2 = ht->vertex();
          VertexIter v3 = h->next()->next()->vertex();
          VertexIter v4 = ht->next()->next()->vertex();;
          
          e->newPosition = (v1->position * (3/8.0f)) + (v2->position * (3/8.0f)) + (v3->position * (1/8.0f)) + (v4->position * (1/8.0f));
          e->isNew = false;
          
          iter.push_back(e);
      }

      // TODO Next, we're going to split every edge in the mesh, in any order.  For future
      // TODO reference, we're also going to store some information about which subdivided
      // TODO edges come from splitting an edge in the original mesh, and which edges are new,
      // TODO by setting the flat Edge::isNew.  Note that in this loop, we only want to iterate
      // TODO over edges of the original mesh---otherwise, we'll end up splitting edges that we
      // TODO just split (and the loop will never end!)
      for (EdgeIter e : iter) {
          if (e->isNew == false) {
              VertexIter v = mesh.splitEdge(e);
              v->isNew = true;
              v->newPosition = e->newPosition;
          }
      }

      // TODO Now flip any new edge that connects an old and new vertex.
      for (EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
          VertexIter v = e->halfedge()->vertex();
          VertexIter vt = e->halfedge()->twin()->vertex();
          if (((v->isNew && !vt->isNew) || (!v->isNew && vt->isNew)) && (e->isNew)){
              mesh.flipEdge(e);
          }
      }

      // TODO Finally, copy the new vertex positions into final Vertex::position.
      for (VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
          v->position = v->newPosition;
          v->isNew = false;
      }
  }
}
