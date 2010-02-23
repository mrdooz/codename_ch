#ifndef MARCHING_CUBES_UTILS_HPP
#define MARCHING_CUBES_UTILS_HPP

// code stolen from http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/
struct Triangle
{
  D3DXVECTOR3 p[3];
};

struct GridCell
{
  D3DXVECTOR3 p[8];
  float val[8];
};

int Polygonise(const GridCell& grid, float isolevel, Triangle *triangles);

#endif
