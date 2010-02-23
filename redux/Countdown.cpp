#include "stdafx.h"
#include "Countdown.hpp"
#include "Mesh.hpp"
#include "ReduxLoader.hpp"
#include "DebugRenderer.hpp"
#include "Utils.hpp"

using namespace std;
using namespace boost::assign;

extern ID3D10Device* g_d3d_device;

namespace 
{
  const float width = 800;
  const float height = 600;
  const float far_plane = 1000;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;

  ID3D10Buffer* create_fullscreen_quad(ID3D10Device* device)
  {
    // 0--1
    // 2--3

    struct Vtx
    {
      Vtx(const D3DXVECTOR3& pos, const D3DXCOLOR& col) : pos(pos), col(col) {}
      D3DXVECTOR3 pos;
      D3DXCOLOR col;
    };

    const float top_gray = 0.8f;
    const float bottom_gray = 0.0f;
    Vtx v0(D3DXVECTOR3(-1,+1,0), D3DXCOLOR(top_gray,top_gray,top_gray,1));
    Vtx v1(D3DXVECTOR3(+1,+1,0), D3DXCOLOR(top_gray,top_gray,top_gray,1));
    Vtx v2(D3DXVECTOR3(-1,-1,0), D3DXCOLOR(bottom_gray,bottom_gray,bottom_gray,1));
    Vtx v3(D3DXVECTOR3(+1,-1,0), D3DXCOLOR(bottom_gray,bottom_gray,bottom_gray,1));

    std::vector<Vtx> verts;
    // 0, 1, 2
    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);

    // 2, 1, 3
    verts.push_back(v2);
    verts.push_back(v1);
    verts.push_back(v3);

    D3D10_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage     = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    buffer_desc.ByteWidth = verts.size() * sizeof(Vtx);

    D3D10_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(init_data));
    init_data.pSysMem = &verts[0];
    init_data.SysMemPitch = 0;
    init_data.SysMemSlicePitch = 0;

    ID3D10Buffer* buffer = NULL;
    if (FAILED(device->CreateBuffer(&buffer_desc, &init_data, &buffer))) {
      return NULL;
    }
    return buffer;
  }

  ID3D10BufferPtr fs_buffer;
  ID3D10InputLayoutPtr input_layout;

}

namespace mpl = boost::mpl;

struct CountdownTentacle
{
  VertexBuffer< boost::mpl::vector<D3DXVECTOR3, D3DXNORMAL> >* vb;
  IndexBuffer<uint32_t>* ib;

  void init(const ID3D10DevicePtr& device)
  {
    splits_ = 20;
    start_time_ = 0;

    ControlPoints pts0;
    pts0.push_back(D3DXVECTOR3(0, 10, 0));
    pts0.push_back(D3DXVECTOR3(3, 5, 0));
    pts0.push_back(D3DXVECTOR3(7, 0, 0));
    pts0.push_back(D3DXVECTOR3(3, -5, 0));
    pts0.push_back(D3DXVECTOR3(0, -10, 0));
    pts0.push_back(D3DXVECTOR3(-3, -5, 0));
    pts0.push_back(D3DXVECTOR3(-7, 0, 0));
    pts0.push_back(D3DXVECTOR3(-3, 5, 0));
    pts0.push_back(D3DXVECTOR3(0, 10, 0));
    control_points_.push_back(pts0);

    ControlPoints pts1;
    pts1.push_back(D3DXVECTOR3(-2, 7, 0));
    pts1.push_back(D3DXVECTOR3(0, 10, 0));
    pts1.push_back(D3DXVECTOR3(0, 7, 0));
    pts1.push_back(D3DXVECTOR3(0, 2, 0));
    pts1.push_back(D3DXVECTOR3(0, -4, 0));
    pts1.push_back(D3DXVECTOR3(0, -7, 0));
    pts1.push_back(D3DXVECTOR3(0, -10, 0));
    pts1.push_back(D3DXVECTOR3(-2, -10, 0));
    pts1.push_back(D3DXVECTOR3(2, -10, 0));
    control_points_.push_back(pts1);

    ControlPoints pts2;
    pts2.push_back(D3DXVECTOR3(-3, 7, 0));
    pts2.push_back(D3DXVECTOR3(0, 10, 0));
    pts2.push_back(D3DXVECTOR3(3, 7, 0));
    pts2.push_back(D3DXVECTOR3(3, 3, 0));
    pts2.push_back(D3DXVECTOR3(2, -2, 0));
    pts2.push_back(D3DXVECTOR3(0, -4, 0));
    pts2.push_back(D3DXVECTOR3(-3, -10, 0));
    pts2.push_back(D3DXVECTOR3(0, -10, 0));
    pts2.push_back(D3DXVECTOR3(3, -10, 0));
    control_points_.push_back(pts2);

    ControlPoints pts3;
    pts3.push_back(D3DXVECTOR3(-3, 7, 0));
    pts3.push_back(D3DXVECTOR3(0, 10, 0));
    pts3.push_back(D3DXVECTOR3(2, 7, 0));
    pts3.push_back(D3DXVECTOR3(2, 4, 0));
    pts3.push_back(D3DXVECTOR3(0, 0, 0));
    pts3.push_back(D3DXVECTOR3(2, -4, 0));
    pts3.push_back(D3DXVECTOR3(2, -7, 0));
    pts3.push_back(D3DXVECTOR3(0, -10, 0));
    pts3.push_back(D3DXVECTOR3(-3, -7, 0));
    control_points_.push_back(pts3);

    ControlPoints pts4;
    pts4.push_back(D3DXVECTOR3(3, 0, 0));
    pts4.push_back(D3DXVECTOR3(0, 0, 0));
    pts4.push_back(D3DXVECTOR3(-3, 0, 0));
    pts4.push_back(D3DXVECTOR3(-1.5f, 5, 0));
    pts4.push_back(D3DXVECTOR3(0, 10, 0));
    pts4.push_back(D3DXVECTOR3(0, 5, 0));
    pts4.push_back(D3DXVECTOR3(0, 0, 0));
    pts4.push_back(D3DXVECTOR3(0, -5, 0));
    pts4.push_back(D3DXVECTOR3(0, -10, 0));
    control_points_.push_back(pts4);

    ControlPoints pts5;
    pts5.push_back(D3DXVECTOR3(3, 10, 0));
    pts5.push_back(D3DXVECTOR3(0, 10, 0));
    pts5.push_back(D3DXVECTOR3(-3, 10, 0));
    pts5.push_back(D3DXVECTOR3(-3, 0, 0));
    pts5.push_back(D3DXVECTOR3(0, 0, 0));
    pts5.push_back(D3DXVECTOR3(3, -5, 0));
    pts5.push_back(D3DXVECTOR3(0, -10, 0));
    pts5.push_back(D3DXVECTOR3(-3, -10, 0));
    pts5.push_back(D3DXVECTOR3(-3, -8, 0));
    control_points_.push_back(pts5);

    ControlPoints pts6;
    pts6.push_back(D3DXVECTOR3(3, 10, 0));
    pts6.push_back(D3DXVECTOR3(0, 10, 0));
    pts6.push_back(D3DXVECTOR3(-3, 4, 0));
    pts6.push_back(D3DXVECTOR3(-3, -4, 0));
    pts6.push_back(D3DXVECTOR3(0, -10, 0));
    pts6.push_back(D3DXVECTOR3(3, -4, 0));
    pts6.push_back(D3DXVECTOR3(2, 0, 0));
    pts6.push_back(D3DXVECTOR3(0, 0, 0));
    pts6.push_back(D3DXVECTOR3(-3, 0, 0));
    control_points_.push_back(pts6);

    ControlPoints pts7;
    pts7.push_back(D3DXVECTOR3(-3, 10, 0));
    pts7.push_back(D3DXVECTOR3(-1.5f, 10, 0));
    pts7.push_back(D3DXVECTOR3(0, 10, 0));
    pts7.push_back(D3DXVECTOR3(1.5f, 10, 0));
    pts7.push_back(D3DXVECTOR3(3, 10, 0));
    pts7.push_back(D3DXVECTOR3(1.5f, 5, 0));
    pts7.push_back(D3DXVECTOR3(0, 0, 0));
    pts7.push_back(D3DXVECTOR3(-1.5f, -5, 0));
    pts7.push_back(D3DXVECTOR3(-3, -10, 0));
    control_points_.push_back(pts7);

    ControlPoints pts8;
    pts8.push_back(D3DXVECTOR3(0, 10, 0));
    pts8.push_back(D3DXVECTOR3(-3, 5, 0));
    pts8.push_back(D3DXVECTOR3(0, 0, 0));
    pts8.push_back(D3DXVECTOR3(-3, -5, 0));
    pts8.push_back(D3DXVECTOR3(0, -10, 0));
    pts8.push_back(D3DXVECTOR3(3, -5, 0));
    pts8.push_back(D3DXVECTOR3(0, 0, 0));
    pts8.push_back(D3DXVECTOR3(3, 5, 0));
    pts8.push_back(D3DXVECTOR3(0, 10, 0));
    control_points_.push_back(pts8);

    ControlPoints pts9;
    pts9.push_back(D3DXVECTOR3(0, -10, 0));
    pts9.push_back(D3DXVECTOR3(1.5f, -5, 0));
    pts9.push_back(D3DXVECTOR3(3, 0, 0));
    pts9.push_back(D3DXVECTOR3(1.5f, 5, 0));
    pts9.push_back(D3DXVECTOR3(0, 10, 0));
    pts9.push_back(D3DXVECTOR3(-1.5f, 7, 0));
    pts9.push_back(D3DXVECTOR3(-3, 5, 0));
    pts9.push_back(D3DXVECTOR3(0, 0, 0));
    pts9.push_back(D3DXVECTOR3(1.5f, 5, 0));
    control_points_.push_back(pts9);
  }

  void render_digit(const uint32_t cur, const uint32_t next, const float ratio, const D3DXVECTOR3& ofs)
  {

    vb->start_frame();

    float inc = 1 / (float)(splits_ - 1);

    const uint32_t spins = 20;

    bool first_point = true;

    D3DXVECTOR3 binormal;
    D3DXVECTOR3 prev_binormal;
    D3DXVECTOR3 tangent;
    D3DXVECTOR3 prev_tangent;
    D3DXVECTOR3 normal;
    D3DXVECTOR3 prev_normal;

    const int32_t num_numbers = control_points_.size();
    const int32_t num_knots = control_points_[0].size();
    for (int32_t i = 0; i < num_knots-1; ++i) {
      const bool last_knot = i == num_knots - 2;

      const D3DXVECTOR3 a0 = control_points_[cur][max(0,i-1)];
      const D3DXVECTOR3 a1 = control_points_[cur][i+0];
      const D3DXVECTOR3 a2 = control_points_[cur][min(num_knots-1, i+1)];
      const D3DXVECTOR3 a3 = control_points_[cur][min(num_knots-1, i+2)];

      const D3DXVECTOR3 b0 = control_points_[next][max(0,i-1)];
      const D3DXVECTOR3 b1 = control_points_[next][i+0];
      const D3DXVECTOR3 b2 = control_points_[next][min(num_knots-1, i+1)];
      const D3DXVECTOR3 b3 = control_points_[next][min(num_knots-1, i+2)];

      const D3DXVECTOR3 p_1 = ofs + a0 + ratio * (b0 - a0);
      const D3DXVECTOR3 p0 = ofs + a1 + ratio * (b1 - a1);
      const D3DXVECTOR3 p1 = ofs + a2 + ratio * (b2 - a2);
      const D3DXVECTOR3 p2 = ofs + a3 + ratio * (b3 - a3);

      D3DXVECTOR3 m0 = (p1 - p_1) / 2;
      D3DXVECTOR3 m1 = (p2 - p0) / 2;

      float cur_t = 0;
      for (uint32_t j = 0; j < splits_; ++j, cur_t += inc) {
        D3DXVECTOR3 cur;
        D3DXVec3Hermite(&cur, &p0, &m0, &p1, &m1, cur_t);
        cur = catmull(cur_t, p_1, p0, p1, p2);
        tangent = catmull_vel(cur_t, p_1, p0, p1, p2);
        D3DXVec3Normalize(&tangent, &tangent);

        if (first_point) {
          D3DXVECTOR3 q = catmull_acc(cur_t, p_1, p0, p1, p2);
          D3DXVec3Cross(&normal, &tangent, &q);
          if (D3DXVec3LengthSq(&normal) == 0) {
            // if the curvature is 0, choose any normal perpedictular to the tangent
            normal = find_orthogonal(tangent);
          } else {
            D3DXVec3Cross(&normal, &normal, &tangent);
          }
          D3DXVec3Normalize(&normal, &normal);

          D3DXVec3Cross(&binormal, &tangent, &normal);
          first_point = false;
        } else {
          // Ken Sloan's method to propagate the reference frame
          D3DXVec3Cross(&normal, &prev_binormal, &tangent);
          D3DXVec3Cross(&binormal, &tangent, &normal);
        }

        prev_binormal = binormal;
        prev_tangent = tangent;


        const float delta_angle = 2 * (float)D3DX_PI / (float)(spins - 1);
        float cur_angle = 0;
        D3DXVECTOR3 tmp;
        D3DXVECTOR3 tmp2;
        D3DXMATRIX mtx_rot2(kMtxId);

        D3DXVECTOR3 scaled_s = normal;
        if (last_knot) {
          //scaled_s *= ((splits_-1) - j) / (float)splits_;
        }

        for (uint32_t k = 0; k < spins; ++k, cur_angle += delta_angle) {
          D3DXMatrixRotationAxis(&mtx_rot2, &tangent, cur_angle);
          D3DXVec3TransformCoord(&tmp, &scaled_s, &mtx_rot2);
          D3DXVec3TransformNormal(&tmp2, &normal, &mtx_rot2);

          vb->add(tmp + cur, tmp2);
        }
      }
    }

    vb->end_frame();

    ib->start_frame();
    for (uint32_t i = 0; i < vb->vertex_count() / spins  - 1; ++i) {
      for (uint32_t j = 0; j < spins; ++j) {

        // d-c
        // | |
        // b-a
        const uint32_t next_j = (j + 1) % spins;
        const uint32_t a = i*spins + j;
        const uint32_t b = i*spins + next_j;
        const uint32_t c = (i+1)*spins + j;
        const uint32_t d = (i+1)*spins + next_j;

        // a, b, c
        ib->add(a);
        ib->add(b);
        ib->add(c);

        // b, d, c
        ib->add(b);
        ib->add(d);
        ib->add(c);
      }
    }
    ib->end_frame();
    vb->set_input_layout();
    vb->set_vertex_buffer();
    ib->set_index_buffer();
    g_d3d_device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_d3d_device->DrawIndexed(ib->index_count(), 0, 0);
  }

  void render(const uint32_t cur_time)
  {
    if (start_time_ == 0) {
      start_time_ = cur_time;
    }

    SYSTEMTIME systime;
    GetSystemTime(&systime);

    //vb->start_frame();

    const uint32_t cur_hour = systime.wHour;
    const uint32_t next_hour = (systime.wHour + 1) % 24;

    const uint32_t cur_min = systime.wMinute;
    const uint32_t next_min = (systime.wMinute + 1) % 60;

    const uint32_t cur_sec = systime.wSecond;
    const uint32_t next_sec = (systime.wSecond + 1) % 60;

    const float ratio = systime.wMilliseconds / 1000.0f;

    render_digit(cur_hour / 10, next_hour / 10, 0, D3DXVECTOR3(-30,0,0));
    render_digit(cur_hour % 10, next_hour % 10, 0, D3DXVECTOR3(-20,0,0));

    render_digit(cur_min / 10, next_min / 10, 0, D3DXVECTOR3(-10,0,0));
    render_digit(cur_min % 10, next_min % 10, 0, D3DXVECTOR3(0,0,0));

    render_digit(cur_sec / 10, next_sec / 10, ratio, D3DXVECTOR3(10,0,0));
    render_digit(cur_sec % 10, next_sec % 10, ratio, D3DXVECTOR3(20,0,0));

    const uint32_t spins = 20;
  }

  typedef std::vector<D3DXVECTOR3> ControlPoints;
  typedef std::vector<ControlPoints> AllControlPoints;
  AllControlPoints control_points_;
  uint32_t splits_;
  uint32_t start_time_;
};

CountdownTentacle countdown;

Countdown::Countdown(const SystemSPtr& system, const EffectManagerSPtr& effect_manager)
: system_(system)
, effect_manager_(effect_manager)
  , animation_manager_(new AnimationManager())
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , dynamic_mgr_(new DebugRenderer(g_d3d_device))
  , splits_(20)
  , effect_(g_d3d_device)
{

  ib_ = new IndexBuffer<uint32_t>(g_d3d_device);
  vb_ = new VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> >(g_d3d_device);
  fs_buffer = create_fullscreen_quad(g_d3d_device);

  countdown.ib = ib_;
  countdown.vb = vb_;

  //tentacle.init(g_d3d_device);
  countdown.init(g_d3d_device);
  system_->add_renderable(this);
  Serializer::instance().add_instance(this);

  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

}

Countdown::~Countdown()
{
  SAFE_DELETE(vb_);
  SAFE_DELETE(ib_);

  clear_vector(loaded_effects_);
  clear_vector(loaded_materials_);
  clear_vector(loaded_material_connections_);
  effect_list_.clear();
  effects_.clear();

  SAFE_DELETE(animation_manager_);
  container_delete(effect_connections_);
  system_.reset();
  effect_manager_.reset();
}

bool Countdown::close()
{
  return true;
}

#define RETURN_ON_FALSE(x) if (!(x)) { return false; } else {}


bool Countdown::init() 
{
  RETURN_ON_FALSE(effect_.load("effects/countdown.fx"));
  RETURN_ON_FALSE(dynamic_mgr_->init());

  D3D10_PASS_DESC desc;
  RETURN_ON_FALSE(effect_.get_pass_desc(desc, "render"));
  RETURN_ON_FALSE(vb_->init(desc));
  RETURN_ON_FALSE(ib_->init());

  D3D10_PASS_DESC desc2;
  RETURN_ON_FALSE(effect_.get_pass_desc(desc2, "fullscreen"));
/*
  mpl::for_each<mpl::vector<D3DXVECTOR3, D3DXCOLOR> >(layout_maker());
  layout_maker::InputDescs v = layout_maker::descs_;

  if (FAILED(g_d3d_device->CreateInputLayout(&layout_maker::descs_[0], layout_maker::descs_.size(), 
    desc2.pIAInputSignature, desc2.IAInputSignatureSize, &input_layout))) {
      return false;
  }
*/
  if (!create_input_layout<mpl::vector<D3DXVECTOR3, D3DXCOLOR> >(input_layout, desc2, g_d3d_device)) {
    return false;
  }
  return true;
}


void Countdown::render(const int32_t time_in_ms)
{

  effect_.set_technique("fullscreen");
  g_d3d_device->IASetInputLayout(input_layout);
  ID3D10Buffer* bufs[] = { fs_buffer };
  uint32_t strides [] = { sizeof(D3DXVECTOR3) + sizeof(D3DXCOLOR) };
  uint32_t offset = 0;
  g_d3d_device->IASetVertexBuffers(0, 1, bufs, strides, &offset);
  g_d3d_device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  g_d3d_device->Draw(6, 0);


  D3DXVECTOR3 eye_pos;
  D3DXMATRIX mtx_proj;
  D3DXMATRIX mtx_view;
  system_->get_free_fly_camera(eye_pos, mtx_view);
  D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, 0.1f, far_plane);

  effect_.set_technique("render");
  effect_.set_variable("projection", mtx_proj);
  effect_.set_variable("view", mtx_view);
  effect_.set_variable("eye_pos", eye_pos);
  effect_.set_variable("world", kMtxId);
  effect_.set_variable("world_view_proj", kMtxId * mtx_view * mtx_proj);

  countdown.render(time_in_ms);


}

void Countdown::process_input_callback(const Input& input)
{
  // TODO: For this to really work, we need to be able to get the key-up event, so we can use that to toggle
  // switch between different cameras.

  if (scene_.cameras_.empty()) {
    free_fly_camera_enabled_  = true;
  } else {
    if (input.key_status['0']) {
      free_fly_camera_enabled_ = true;
    }

    const uint32_t num_cameras = scene_.cameras_.size();
    for (uint32_t i = 0; i < num_cameras; ++i) {
      if (input.key_status['1' + i]) {
        free_fly_camera_enabled_ = false;
        current_camera_ = i;
        break;
      }
    }
  }

}
