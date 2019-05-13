#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
  this->width = width;
  this->height = height;
  this->num_width_points = num_width_points;
  this->num_height_points = num_height_points;
  this->thickness = thickness;

  buildGrid();
  buildClothMesh();
}

Cloth::~Cloth() {
  point_masses.clear();
  springs.clear();

  if (clothMesh) {
    delete clothMesh;
  }
}

bool is_pinned(int row, int col, Cloth *cloth) {
    for (int i = 0; i < cloth->pinned.size(); i++) {
        if (row == cloth->pinned[i][0] && col == cloth->pinned[i][1]) {
            return true;
        }
    }
    return false;
}

void Cloth::buildGrid() {
    // TODO (Part 1): Build a grid of masses and springs.
    // Add point masses
    for (int row = 0; row < num_height_points; row++) {
        for (int col = 0; col < num_width_points; col++) {
            if (orientation == HORIZONTAL) {
                double x = row * (height/((double) num_height_points));
                double y = 1;
                double z = col * (width/((double) num_width_points));
                point_masses.push_back(PointMass(Vector3D(x, y, z), is_pinned(row, col, this)));
            } else { // orientation == VERTICAL
                double x = row * (height/((double) num_height_points));
                double y = col * (width/((double) num_width_points));
                double z = ((((1.0f * rand())/(1.0f*RAND_MAX)) * 2) - 1) / 1000.0;
                point_masses.push_back(PointMass(Vector3D(x, y, z), is_pinned(row, col, this)));
            }
        }
    }
    // Add springs
    int nw = num_width_points;
    for (int row = 0; row < num_height_points; row++) {
        for (int col = 0; col < num_width_points; col++) {
            if (row > 0) {
                springs.push_back(Spring(&point_masses[(row-1)*nw+col], &point_masses[row*nw+col], STRUCTURAL));
            }
            if (col > 0) {
                springs.push_back(Spring(&point_masses[row*nw+(col-1)], &point_masses[row*nw+col], STRUCTURAL));
            }
            if (row > 0 && col > 0) {
                springs.push_back(Spring(&point_masses[(row-1)*nw+(col-1)], &point_masses[row*nw+col], SHEARING));
            }
            if (row > 0 && col < num_width_points-1) {
                springs.push_back(Spring(&point_masses[(row-1)*nw+(col+1)], &point_masses[row*nw+col], SHEARING));
            }
            if (row > 1) {
                springs.push_back(Spring(&point_masses[(row-2)*nw+col], &point_masses[row*nw+col], BENDING));
            }
            if (col > 1) {
                springs.push_back(Spring(&point_masses[row*nw+(col-2)], &point_masses[row*nw+col], BENDING));
            }
        }
    }
}

void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
  double mass = width * height * cp->density / num_width_points / num_height_points;
  double delta_t = 1.0f / frames_per_sec / simulation_steps;

  // TODO (Part 2): Compute total force acting on each point mass.
    Vector3D external_force = Vector3D(0, 0, 0);
    for (int i = 0; i < external_accelerations.size(); i++) {
        external_force += external_accelerations[i] * mass;
    }
    for (int i = 0; i < point_masses.size(); i++) {
        point_masses[i].forces = external_force;
    }
    for (int i = 0; i < springs.size(); i++) {
        Spring spring = springs[i];
        if ((spring.spring_type == STRUCTURAL && cp->enable_structural_constraints) ||
            (spring.spring_type == SHEARING && cp->enable_shearing_constraints) ||
            (spring.spring_type == BENDING && cp->enable_bending_constraints)) {
            double mod = 1;
            if (spring.spring_type == BENDING) {mod = 0.2;}
            double Fs = cp->ks * mod * ((spring.pm_a->position - spring.pm_b->position).norm() - spring.rest_length);
            spring.pm_a->forces += Fs * (spring.pm_b->position - spring.pm_a->position).unit();
            spring.pm_b->forces += Fs * (spring.pm_a->position - spring.pm_b->position).unit();
        }
    }


  // TODO (Part 2): Use Verlet integration to compute new point mass positions
    
    for (int i = 0; i < point_masses.size(); i++) {
        Vector3D new_pos = point_masses[i].position + (1 - (cp->damping / 100.0)) * (point_masses[i].position - point_masses[i].last_position) + (point_masses[i].forces / mass) * delta_t * delta_t;
        if (!point_masses[i].pinned) {
            point_masses[i].last_position = point_masses[i].position;
            point_masses[i].position = new_pos;
        }
    }
    
    // Correct for rest_length
    for (int i = 0; i < springs.size(); i++) {
        Spring spring = springs[i];
        double length = (spring.pm_a->position - spring.pm_b->position).norm();
        if (length <= (1.1 * spring.rest_length))
            continue;
        if (spring.pm_a->pinned && spring.pm_b->pinned)
            continue;
        Vector3D correction = (spring.pm_a->position - spring.pm_b->position).unit() * (length - (spring.rest_length*1.1));
        if (spring.pm_a->pinned) {
            spring.pm_b->position += correction;
        } else if (spring.pm_b->pinned) {
            spring.pm_a->position -= correction;
        } else {
            spring.pm_b->position += correction *  (1/2.0);
            spring.pm_a->position -= correction *  (1/2.0);
        }
    }

  // TODO (Part 4): Handle self-collisions.
//    build_spatial_map();
//    for (int i = 0; i < point_masses.size(); i++) {
//        self_collide(point_masses[i], simulation_steps);
//    }

  // TODO (Part 3): Handle collisions with other primitives.
    for (int i = 0; i < collision_objects->size(); i++) {
        for (int j = 0; j < point_masses.size(); j++) {
            (*collision_objects)[i]->collide(point_masses[j]);
        }
    }

  // TODO (Part 2): Constrain the changes to be such that the spring does not change
  // in length more than 10% per timestep [Provot 1995].

}

void Cloth::build_spatial_map() {
  for (const auto &entry : map) {
    delete(entry.second);
  }
  map.clear();

  // TODO (Part 4): Build a spatial map out of all of the point masses.
    for (int i = 0; i < point_masses.size(); i++) {
        float hash_num = hash_position(point_masses[i].position);
        if (map[hash_num] == NULL) {
            vector<PointMass *> *vpm = new vector<PointMass *>();
            vpm->push_back(&point_masses[i]);
            map[hash_num] = vpm;
        } else {
            map[hash_num]->push_back(&point_masses[i]);
        }
    }
}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
  // TODO (Part 4): Handle self-collision for a given point mass.
    float hash_num = hash_position(pm.position);
    if (map[hash_num] == NULL) {
        return;
    }
    vector<PointMass *> vpm = *map[hash_num];
    Vector3D correction_final = Vector3D(0, 0, 0);
    int num_corrections = 0;
//    cout << vpm.size() << endl;r
    for (int i = 0; i < vpm.size(); i++) {
        PointMass candidate = *vpm[i];
        if (candidate.position == pm.position) {
            // Same exact address means same exact object
            continue;
        }
        if ((pm.position - candidate.position).norm() < (2 * thickness)) {
            num_corrections++;
            correction_final += (pm.position - candidate.position).unit() * (2 * thickness - (pm.position - candidate.position).norm());
        }
    }
    if (num_corrections == 0) {
        return;
    }
    correction_final /= (double) num_corrections;
    correction_final /= simulation_steps;

    pm.position = pm.position + correction_final;
}

float Cloth::hash_position(Vector3D pos) {
  // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.
    float w = 3 * width / ((float) num_width_points);
    float h = 3 * height / ((float) num_height_points);
    float t = max(w, h);
    float pos_w = pos.x - fmod(pos.x, w);
    float pos_y = pos.y - fmod(pos.y, h);
    float pos_z = pos.z - fmod(pos.z, t);
    
    return (31*31*pos_w)+(31*pos_y)+pos_z;
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
  PointMass *pm = &point_masses[0];
  for (int i = 0; i < point_masses.size(); i++) {
    pm->position = pm->start_position;
    pm->last_position = pm->start_position;
    pm++;
  }
}

void Cloth::buildClothMesh() {
  if (point_masses.size() == 0) return;

  ClothMesh *clothMesh = new ClothMesh();
  vector<Triangle *> triangles;

  // Create vector of triangles
  for (int y = 0; y < num_height_points - 1; y++) {
    for (int x = 0; x < num_width_points - 1; x++) {
      PointMass *pm = &point_masses[y * num_width_points + x];
      // Get neighboring point masses:
      /*                      *
       * pm_A -------- pm_B   *
       *             /        *
       *  |         /   |     *
       *  |        /    |     *
       *  |       /     |     *
       *  |      /      |     *
       *  |     /       |     *
       *  |    /        |     *
       *      /               *
       * pm_C -------- pm_D   *
       *                      *
       */
      
      float u_min = x;
      u_min /= num_width_points - 1;
      float u_max = x + 1;
      u_max /= num_width_points - 1;
      float v_min = y;
      v_min /= num_height_points - 1;
      float v_max = y + 1;
      v_max /= num_height_points - 1;
      
      PointMass *pm_A = pm                       ;
      PointMass *pm_B = pm                    + 1;
      PointMass *pm_C = pm + num_width_points    ;
      PointMass *pm_D = pm + num_width_points + 1;
      
      Vector3D uv_A = Vector3D(u_min, v_min, 0);
      Vector3D uv_B = Vector3D(u_max, v_min, 0);
      Vector3D uv_C = Vector3D(u_min, v_max, 0);
      Vector3D uv_D = Vector3D(u_max, v_max, 0);
      
      
      // Both triangles defined by vertices in counter-clockwise orientation
        // Flip normals Gefen
            triangles.push_back(new Triangle(pm_A, pm_B, pm_C, uv_A, uv_B, uv_C));
            triangles.push_back(new Triangle(pm_B, pm_D, pm_C, uv_B, uv_D, uv_C));
        // Old code
//          triangles.push_back(new Triangle(pm_A, pm_C, pm_B,
//                                           uv_A, uv_C, uv_B));
//          triangles.push_back(new Triangle(pm_B, pm_C, pm_D,
//                                           uv_B, uv_C, uv_D));
    }
  }

  // For each triangle in row-order, create 3 edges and 3 internal halfedges
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    // Allocate new halfedges on heap
    Halfedge *h1 = new Halfedge();
    Halfedge *h2 = new Halfedge();
    Halfedge *h3 = new Halfedge();

    // Allocate new edges on heap
    Edge *e1 = new Edge();
    Edge *e2 = new Edge();
    Edge *e3 = new Edge();

    // Assign a halfedge pointer to the triangle
    t->halfedge = h1;

    // Assign halfedge pointers to point masses
    t->pm1->halfedge = h1;
    t->pm2->halfedge = h2;
    t->pm3->halfedge = h3;

    // Update all halfedge pointers
    h1->edge = e1;
    h1->next = h2;
    h1->pm = t->pm1;
    h1->triangle = t;

    h2->edge = e2;
    h2->next = h3;
    h2->pm = t->pm2;
    h2->triangle = t;

    h3->edge = e3;
    h3->next = h1;
    h3->pm = t->pm3;
    h3->triangle = t;
  }

  // Go back through the cloth mesh and link triangles together using halfedge
  // twin pointers

  // Convenient variables for math
  int num_height_tris = (num_height_points - 1) * 2;
  int num_width_tris = (num_width_points - 1) * 2;

  bool topLeft = true;
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    if (topLeft) {
      // Get left triangle, if it exists
      if (i % num_width_tris != 0) { // Not a left-most triangle
        Triangle *temp = triangles[i - 1];
        t->pm1->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm1->halfedge->twin = nullptr;
      }

      // Get triangle above, if it exists
      if (i >= num_width_tris) { // Not a top-most triangle
        Triangle *temp = triangles[i - num_width_tris + 1];
        t->pm3->halfedge->twin = temp->pm2->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle to bottom right; guaranteed to exist
      Triangle *temp = triangles[i + 1];
      t->pm2->halfedge->twin = temp->pm1->halfedge;
    } else {
      // Get right triangle, if it exists
      if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
        Triangle *temp = triangles[i + 1];
        t->pm3->halfedge->twin = temp->pm1->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle below, if it exists
      if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
        Triangle *temp = triangles[i + num_width_tris - 1];
        t->pm2->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm2->halfedge->twin = nullptr;
      }

      // Get triangle to top left; guaranteed to exist
      Triangle *temp = triangles[i - 1];
      t->pm1->halfedge->twin = temp->pm2->halfedge;
    }

    topLeft = !topLeft;
  }

  clothMesh->triangles = triangles;
  this->clothMesh = clothMesh;
}
