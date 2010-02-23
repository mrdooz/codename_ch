#include "stdafx.h"
#include "FreeFlyCamera.hpp"
#include "Input.hpp"
#include "SmoothDriver.hpp"

namespace 
{
  const float kPi = static_cast<float>(D3DX_PI);
  const D3DXVECTOR3 kRight(1,0,0);
  const D3DXVECTOR3 kUp(0,1,0);
  const D3DXVECTOR3 kForward(0,0,1);

  const float kNearPlane = 0.1f;
  const float kFarPlane = 1000.0f;
  const float kFov = static_cast<float>(D3DXToRadian(45));
}

FreeFlyCamera::FreeFlyCamera() 
{
  Serializer::instance().add_instance(this);
  reset();
}

FreeFlyCamera::~FreeFlyCamera()
{
  Serializer::instance().remove_instance(this);
}

void FreeFlyCamera::reset()
{
  near_plane_ = kNearPlane;
  far_plane_ = kFarPlane;
  fov_ = kFov;
  D3DXMatrixPerspectiveFovLH(&proj_matrix_, fov_, 4/3.0f, near_plane_, far_plane_);
  eye_pos_ = D3DXVECTOR3(0,0,-100);
  eye_vel_ = D3DXVECTOR3(0,0,0);
  forward_vector_ = D3DXVECTOR3(0,0,1);
  right_vector_ = D3DXVECTOR3(1, 0, 0);
  up_vector_ = D3DXVECTOR3(0, 1, 0);
  theta_ = kPi / 2.0f;
  phi_ = kPi / 2.0f;
  movement_speed_ = 100.0f;
  rotation_speed_ = 1.0f;
  auto_ = false;
  D3DXMatrixIdentity(&view_matrix_);

}

void FreeFlyCamera::calc_view_matrix() 
{
  D3DXVECTOR3 at(eye_pos_ + forward_vector_);
  D3DXMatrixLookAtLH(&view_matrix_, &eye_pos_, &at, &up_vector_);
}

void FreeFlyCamera::update_from_input(const float delta_time, const Input& input)
{
  if (input.key_status[VK_SHIFT]) {
    if (input.left_button_down) {
      // rotate around x
      theta_ += delta_time * rotation_speed_ * static_cast<float>(input.y_pos - input.last_y_pos);
      // rotate around y
      phi_ += delta_time * rotation_speed_ * static_cast<float>(input.x_pos - input.last_x_pos);
    }
  }

  // create matrix to rotate the x,y,z coord system
  D3DXMATRIX rot_around_y;
  D3DXMatrixIdentity(&rot_around_y);
  D3DXMatrixRotationY(&rot_around_y, phi_ - kPi / 2.0f);

  D3DXMATRIX rot_around_x;
  D3DXMatrixIdentity(&rot_around_x);
  D3DXMatrixRotationX(&rot_around_x, theta_ - kPi / 2.0f);

  D3DXMATRIX total = rot_around_x * rot_around_y;
  D3DXVec3TransformNormal(&right_vector_, &kRight, &total);
  D3DXVec3TransformNormal(&up_vector_, &kUp, &total);
  D3DXVec3TransformNormal(&forward_vector_, &kForward, &total);

  // up/down
  if (input.key_status['Q']) {
    eye_pos_ += delta_time * movement_speed_ * up_vector_;
  } else if (input.key_status['E']) {
    eye_pos_ -= delta_time * movement_speed_ * up_vector_;
  }

  // left/right
  if (input.key_status['A']) {
    eye_pos_ -= delta_time * movement_speed_ * right_vector_;
  } else if (input.key_status['D']) {
    eye_pos_ += delta_time * movement_speed_ * right_vector_;
  }

  // forward/back
  if (input.key_status['W']) {
    eye_pos_ += delta_time * movement_speed_ * forward_vector_;
  } else if (input.key_status['S']) {
    eye_pos_ -= delta_time * movement_speed_ * forward_vector_;
  }

  calc_view_matrix();
}

void FreeFlyCamera::tick(const float dt)
{
  if (!auto_ || dt == 0) {
    return;
  }

  D3DXVECTOR3 to_pos(0,0,-50);
  D3DXVECTOR3 to_vel(0,0,0);
  D3DXVECTOR3 vel(0,0,0);
  //D3DXVECTOR3 pos(0,0,0);

  // max_accel should be somewhere around (typical_distance/coverge_time^2)

  const D3DXVECTOR3 dist = to_pos - eye_pos_;
  const float coverage_time = 1;
  const float max_accel = D3DXVec3Length(&dist) / coverage_time * coverage_time;

  SmoothDriver::DriveCubic((SmoothDriver::Vec3*)&eye_pos_.x, (SmoothDriver::Vec3*)&eye_vel_.x, 
    (const SmoothDriver::Vec3*)&to_pos.x, (const SmoothDriver::Vec3*)&to_vel.x, max_accel, 1.0f / 60.0f);

  calc_view_matrix();
}
