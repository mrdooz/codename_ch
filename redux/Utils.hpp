#ifndef UTILS_HPP
#define UTILS_HPP

template<typename T>
T clamp(T value, T min_value, T max_value)
{
  return min(max_value, max(min_value, value));
}

inline float dist(float x0, float y0, float z0, float x1, float y1, float z1)
{
  const float dx = x0 - x1;
  const float dy = y0 - y1;
  const float dz = z0 - z1;
  return sqrtf(dx*dx + dy * dy + dz * dz);
}

inline void add_vector(std::vector<float>& v, const D3DXVECTOR2& vtx)
{
  v.push_back(vtx[0]);
  v.push_back(vtx[1]);
}

inline void add_vector(std::vector<float>& v, const D3DXVECTOR3& vtx)
{
  v.push_back(vtx[0]);
  v.push_back(vtx[1]);
  v.push_back(vtx[2]);
}

inline void add_vector(std::vector<float>& v, const D3DXCOLOR& color)
{
  v.push_back(color[0]);
  v.push_back(color[1]);
  v.push_back(color[2]);
  v.push_back(color[3]);
}

// Find a vector orthogonal to r, according to rtr (eq. 4.24, pg 71)
inline D3DXVECTOR3 find_orthogonal(const D3DXVECTOR3& r)
{
  const float abs_x = fabs(r.x);
  const float abs_y = fabs(r.y);
  const float abs_z = fabs(r.z);

  if (abs_x < abs_y && abs_x < abs_z) {
    return D3DXVECTOR3(0, -r.z, r.y);
  } else if (abs_y < abs_x && abs_y < abs_z) {
    return D3DXVECTOR3(-r.z, 0, r.x);
  } else {
    return D3DXVECTOR3(-r.y, r.x, 0);
  }
}

inline D3DXMATRIX matrix_from_vectors(const D3DXVECTOR3& v1, const D3DXVECTOR3& v2, const D3DXVECTOR3& v3, const D3DXVECTOR3& v4)
{
  return D3DXMATRIX(
    v1.x, v1.y, v1.z, 0,
    v2.x, v2.y, v2.z, 0,
    v3.x, v3.y, v3.z, 0,
    v4.x, v4.y, v4.z, 1);
}

inline void calc_catmull_coeffs(D3DXVECTOR3& a, D3DXVECTOR3& b, D3DXVECTOR3& c, D3DXVECTOR3& d,
                                const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
{
  a = 0.5f * (-p0 + 3 * p1 - 3 * p2 + p3);
  b = 0.5f * (2 * p0 - 5 * p1 + 4 * p2 - p3);
  c = 0.5f * (-p0 + p2);
  d = 0.5f * (2 * p1);
}

inline D3DXVECTOR3 catmull(const float t, const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
{
  const float t2 = t * t;
  const float t3 = t * t2;
  D3DXVECTOR3 a, b, c, d;
  calc_catmull_coeffs(a, b, c, d, p0, p1, p2, p3);
  return t3 * a + t2 * b + t * c + d;
}

inline D3DXVECTOR3 catmull_vel(const float t, const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
{
  const float t2 = t * t;
  D3DXVECTOR3 a, b, c, d;
  calc_catmull_coeffs(a, b, c, d, p0, p1, p2, p3);
  return 3 * t2 * a + 2 * t * b + c;
}

inline D3DXVECTOR3 catmull_acc(const float t, const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
{
  D3DXVECTOR3 a, b, c, d;
  calc_catmull_coeffs(a, b, c, d, p0, p1, p2, p3);
  return 6 * t * a + 2 * b;
}

inline void catmul_pos_vel(D3DXVECTOR3& pos, D3DXVECTOR3& vel, const float t, 
                           const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, const D3DXVECTOR3& p3)
{
  const float t2 = t * t;
  const float t3 = t * t2;
  D3DXVECTOR3 a, b, c, d;
  calc_catmull_coeffs(a, b, c, d, p0, p1, p2, p3);
  pos = t3 * a + t2 * b + t * c + d;
  // vel = pos'
  vel = 3 * t2 * a + 2 * t * b + c;
}


// Compute tangent for a Kochanek-Bartels curve (using tension and bias)
inline D3DXVECTOR3 calc_tangent(const float a, const float b, const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, const D3DXVECTOR3& p2)
{
  return 0.5f * (1-a)*(1+b) * (p1 - p0) + 0.5f * (1-a)*(1-b) * (p2 - p1);
}

inline D3DXVECTOR3 hermite_interpolate(const float t, const D3DXVECTOR3& p0, const D3DXVECTOR3& m0, const D3DXVECTOR3& p1, const D3DXVECTOR3& m1)
{
  const float t2 = t * t;
  const float t3 = t * t2;
  return (2*t3-3*t2+1) * p0 + (t3-2*t2+t) * m0 + (t3-t2) * m1 + (-2*t3+3*t2) * p1;
}


ID3D10Buffer* create_dynamic_buffer(ID3D10DevicePtr device, const uint32_t bind_flag, const uint32_t buffer_size);

#endif
