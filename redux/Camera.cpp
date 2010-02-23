#include "stdafx.h"
#include "Camera.hpp"

namespace {
  const float kPi = static_cast<float>(D3DX_PI);
  const D3DXVECTOR3 kRight(1,0,0);
  const D3DXVECTOR3 kUp(0,1,0);
  const D3DXVECTOR3 kForward(0,0,1);
}

Camera::Camera()
  : view_matrix_(kMtxId)
  , projection_matrix_(kMtxId)
  , eye_pos_(kVec3Zero)
  , forward_vector_(kForward)
  , right_vector_(kRight)
  , up_vector_(kUp)
  , horizontal_fov_(0)
  , vertical_fov_(0)
  , aspect_ratio_(0)
  , near_plane_(0.1f)
  , far_plane_(1000)
{
}

void Camera::calc_view_matrix()
{
  D3DXVec3Normalize(&forward_vector_, &forward_vector_);
  D3DXVec3Normalize(&up_vector_, &up_vector_);
  const D3DXVECTOR3 look_at(eye_pos_ + forward_vector_);
  D3DXMatrixLookAtLH(&view_matrix_, &eye_pos_, &look_at, &up_vector_);
}

void Camera::calc_projection_matrix()
{
  D3DXMatrixPerspectiveFovLH(&projection_matrix_, vertical_fov_, aspect_ratio_, near_plane_, far_plane_);
}

void Camera::update()
{
  calc_view_matrix();
  calc_projection_matrix();
}

void Camera::set_eye_pos(const D3DXVECTOR3& pos)
{
  eye_pos_ = pos;
  calc_view_matrix();
}

void Camera::set_look_at(const D3DXVECTOR3& lookat)
{
  forward_vector_ = lookat - eye_pos_;
  D3DXVec3Normalize(&forward_vector_, &forward_vector_);
  calc_view_matrix();
}

void Camera::set_pos_up_dir(const D3DXVECTOR3& pos, const D3DXVECTOR3& up, const D3DXVECTOR3& dir)
{
  eye_pos_ = pos;
  up_vector_ = up;
  forward_vector_ = dir;
  calc_view_matrix();
}
