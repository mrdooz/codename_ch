#ifndef FREE_FLY_CAMERA_HPP
#define FREE_FLY_CAMERA_HPP

#include "Serializer.hpp"
struct Input;

struct Sphere
{
  Sphere() {}
  Sphere(const D3DXVECTOR3& center, const float radius) : center(center), radius(radius) {}
  D3DXVECTOR3 center;
  float radius;
};

class FreeFlyCamera 
{
public:
  FreeFlyCamera();
  ~FreeFlyCamera();
  const D3DXMATRIX&    view_matrix() const { return view_matrix_; }
  const D3DXMATRIX&   proj_matrix() const;
  const D3DXVECTOR3&  eye_pos() const { return eye_pos_; }
  float near_plane() const;
  float far_plane() const;
  
  void  update_from_input(const float delta_time, const Input& input);
  void  reset();
  void  tick(const float dt);

  void  set_bounding_sphere(const D3DXVECTOR3& center, const float radius);

private:
  void  calc_view_matrix();
  Sphere  bounding_sphere_;
  D3DXMATRIX  view_matrix_;
  D3DXMATRIX proj_matrix_;
  D3DXVECTOR3 eye_pos_;
  D3DXVECTOR3 eye_vel_;
  D3DXVECTOR3 forward_vector_;
  D3DXVECTOR3 right_vector_;
  D3DXVECTOR3 up_vector_;
  float near_plane_;
  float far_plane_;
  float fov_;
  float theta_;   // rot around x
  float phi_;     // rot around y
  float movement_speed_;
  float rotation_speed_;
  bool auto_;

  SERIALIZE( FreeFlyCamera, MEMBER(eye_pos_) MEMBER(forward_vector_) MEMBER(right_vector_) MEMBER(up_vector_) \
    MEMBER(theta_) MEMBER(phi_) MEMBER(movement_speed_) MEMBER(rotation_speed_) MEMBER(auto_) );
};

inline const D3DXMATRIX& FreeFlyCamera::proj_matrix() const
{
  return proj_matrix_;
}

inline float FreeFlyCamera::near_plane() const
{
  return near_plane_;
}

inline float FreeFlyCamera::far_plane() const
{
  return far_plane_;
}

#endif