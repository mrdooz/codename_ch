#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Node.hpp"

/**
 * Base class for cameras
 */
class Camera : public Node
{
public:
  Camera();
  const D3DXMATRIX&   view_matrix() const { return view_matrix_; }
  const D3DXMATRIX&   projection_matrix() const { return projection_matrix_; }
  const D3DXVECTOR3&  eye_pos() const { return eye_pos_; }
  const D3DXVECTOR3& up() const { return up_vector_; }
  const D3DXVECTOR3& forward() const { return forward_vector_; }
  
  void set_pos_up_dir(const D3DXVECTOR3& pos, const D3DXVECTOR3& up, const D3DXVECTOR3& dir);
  void set_eye_pos(const D3DXVECTOR3& pos);
  void set_look_at(const D3DXVECTOR3& lookat);
  void update();
private:
  friend class ReduxLoader;
  void  calc_view_matrix();
  void  calc_projection_matrix();
  D3DXMATRIX  view_matrix_;
  D3DXMATRIX  projection_matrix_;
  D3DXVECTOR3 eye_pos_;
  D3DXVECTOR3 forward_vector_;
  D3DXVECTOR3 right_vector_;
  D3DXVECTOR3 up_vector_;

  float horizontal_fov_;
  float vertical_fov_;
  float aspect_ratio_;
  float near_plane_;
  float far_plane_;
};


#endif