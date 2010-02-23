#include "stdafx.h"
#include "SpringTest.hpp"
#include "DebugRenderer.hpp"
#include "../system/SystemInterface.hpp"

extern ID3D10Device* g_d3d_device;

namespace
{
  int32_t kFrequency = 100;  //hz
  int32_t kTimestep = 1000 / kFrequency;

  const float width = 800;
  const float height = 600;
  const float near_plane = 0.1f;
  const float far_plane = 1000;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;

  D3DXVECTOR3 normalize(D3DXVECTOR3& v)
  {
    const float len = D3DXVec3Length(&v);
    if (len > 0) {
      return v / len;
    }
    return D3DXVECTOR3(0,0,0);
  }

  float dot(const D3DXVECTOR3& a, const D3DXVECTOR3& b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

}

struct Point
{
  Point()
    : calculated_force_(kVec3Zero)
    , inv_mass_(1)
    , pos_(kVec3Zero)
    , vel_(kVec3Zero)
    , acc_(kVec3Zero)
  {
  }

  Point(const D3DXVECTOR3& pos)
    : calculated_force_(kVec3Zero)
    , inv_mass_(1)
    , pos_(pos)
    , vel_(kVec3Zero)
    , acc_(kVec3Zero)
  {
  }

  virtual ~Point() {}

  D3DXVECTOR3 calculated_force_;
  float inv_mass_;    // 1 / mass, to allow for objects with infinite mass
  D3DXVECTOR3 pos_;
  D3DXVECTOR3 vel_;
  D3DXVECTOR3 acc_;
};

struct Force
{
  Force() {}
  Force(const Points& points) : points_(points) {}
  virtual ~Force() {}

  virtual void calc_force() = 0;

  Points points_;
};

struct Gravity : public Force
{
  Gravity(const D3DXVECTOR3& force, const Points& points);
  virtual void calc_force();

  D3DXVECTOR3 force_;
};

Gravity::Gravity(const D3DXVECTOR3& force, const Points& points)
  : Force(points)
  , force_(force)
{
}

void Gravity::calc_force()
{
  for (Points::iterator i = points_.begin(), e = points_.end(); i != e; ++i) {
    (*i)->calculated_force_ += force_;
  }
}

struct Spring : public Force
{
  Spring(const PointPtr& a, const PointPtr& b, const float stiffness, const float damping);
  virtual void calc_force();

  float stiffness_;
  float damping_;
  float rest_length_;
};

Spring::Spring(const PointPtr& a, const PointPtr& b, const float stiffness, const float damping)
  : stiffness_(stiffness)
  , damping_(damping)
  , rest_length_(D3DXVec3Length(&(a->pos_ - b->pos_)))
{
  points_.push_back(a);
  points_.push_back(b);
}


void Spring::calc_force()
{
  const PointPtr& a = points_[0];
  const PointPtr& b = points_[1];

  // Calc spring force
  const float cur_len = D3DXVec3Length(&(a->pos_ - b->pos_));
  const D3DXVECTOR3 dir(normalize(a->pos_ - b->pos_));
  D3DXVECTOR3 f = -stiffness_ * (cur_len - rest_length_) * dir;

  // Calc damping force
  f += -damping_ * dot(a->vel_ - b->vel_, dir) * dir;

  a->calculated_force_ += f;
  b->calculated_force_ -= f;

}


SpringTest::SpringTest(const SystemSPtr& system, const EffectManagerSPtr& effect_manager)
  : system_(system)
  , effect_manager_(effect_manager)
  , debug_renderer_(new DebugRenderer(g_d3d_device))
  , last_update_(0)
{
  Serializer::instance().add_instance(this);

  system_->add_renderable(this);
  debug_renderer_->init();

}

SpringTest::~SpringTest()
{
  container_delete(forces_);
  SAFE_DELETE(debug_renderer_);
}


bool SpringTest::init()
{
  points_.push_back(PointPtr(new Point(D3DXVECTOR3(0,0,0))));
  points_.push_back(PointPtr(new Point(D3DXVECTOR3(5,0,0))));
  points_.push_back(PointPtr(new Point(D3DXVECTOR3(10,0,0))));
  points_.front()->inv_mass_ = 0;

  forces_.push_back(new Spring(points_[0], points_[1], 2, 10));
  forces_.push_back(new Spring(points_[1], points_[2], 2, 10));

  forces_.push_back(new Gravity(D3DXVECTOR3(0,-1,0), points_));


  return true;
}

bool SpringTest::close()
{
  container_delete(forces_);
  return true;
}

void SpringTest::render(const int32_t time_in_ms, const int32_t delta)
{
  if (time_in_ms - last_update_ > kTimestep ) {
    calc_forces();
    apply_forces();
    last_update_ = time_in_ms;
  }

  D3DXVECTOR3 eye_pos;
  D3DXMATRIX mtx_proj;
  D3DXMATRIX mtx_view;

  system_->get_free_fly_camera(eye_pos, mtx_view);
  D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, near_plane, far_plane);

  const D3DXMATRIX view_proj = mtx_view * mtx_proj;

  debug_renderer_->start_frame();


  for (Points::iterator i = points_.begin(), e = points_.end(); i != e; ++i) {
    PointPtr p = *i;
    debug_renderer_->add_wireframe_sphere(p->pos_, 1, D3DXCOLOR(1,1,1,1), kMtxId, view_proj);
    debug_renderer_->add_line(p->pos_, p->pos_ + p->acc_, D3DXCOLOR(1,0,0,1), view_proj);
    debug_renderer_->add_line(p->pos_, p->pos_ + p->vel_, D3DXCOLOR(0,1,0,1), view_proj);

    debug_renderer_->add_text(p->pos_, mtx_view, mtx_proj, true, "0x%.8x", p.get());
    debug_renderer_->add_debug_string("0x%.8x, acc: %f, %f, %f", p.get(), p->acc_.x, p->acc_.y, p->acc_.z);
    debug_renderer_->add_debug_string("0x%.8x, vel: %f, %f, %f", p.get(), p->vel_.x, p->vel_.y, p->vel_.z);
  }

  for (Forces::iterator i = forces_.begin(), e = forces_.end(); i != e; ++i) {
    Force* f = *i;
    debug_renderer_->add_line(f->points_[0]->pos_, f->points_[1]->pos_, D3DXCOLOR(1,1,1,1), view_proj);
  }

  debug_renderer_->end_frame();
  debug_renderer_->render();
}

void SpringTest::calc_forces()
{
  for (Forces::iterator i = forces_.begin(), e = forces_.end(); i != e; ++i) {
    (*i)->calc_force();
  }

}

void SpringTest::apply_forces()
{
  const float timestep = ((float)kTimestep) / 1000.0f;
  // forward euler integrator
  for (Points::iterator i = points_.begin(), e = points_.end(); i != e; ++i) {
    PointPtr p = *i;
    // acc from total forces
    p->acc_ = p->calculated_force_ * p->inv_mass_;
    p->calculated_force_ = kVec3Zero;

    // integrate pos and vel
    p->pos_ += p->vel_ * timestep;
    p->vel_ += p->acc_ * timestep;

  }
}

void SpringTest::pick(const D3DXVECTOR3& org, const D3DXVECTOR3& dir)
{
  ray_org_ = org;
  ray_dir_ = dir;
}
