#include "stdafx.h"
#include "Dynamic.hpp"
#include "Mesh.hpp"
#include "ReduxLoader.hpp"
#include "DebugRenderer.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Tentacle.hpp"

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

  VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> >* vb = NULL;
  IndexBuffer<uint32_t>* ib = NULL;

}

namespace mpl = boost::mpl;


Tentacle tentacle;

Dynamic::Dynamic(const SystemSPtr& system, const EffectManagerSPtr& effect_manager)
  : system_(system)
  , effect_manager_(effect_manager)
  , animation_manager_(new AnimationManager())
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , dynamic_mgr_(new DebugRenderer(g_d3d_device))
  , splits_(20)
  , effect_(g_d3d_device)
{

  ib = new IndexBuffer<uint32_t>(g_d3d_device);
  vb = new VertexBuffer< mpl::vector<D3DXVECTOR3, D3DXNORMAL> >(g_d3d_device);

  //tentacle.init(g_d3d_device);
  tentacle.init(vb, ib, g_d3d_device),
  system_->add_renderable(this);
  Serializer::instance().add_instance(this);

  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

}

Dynamic::~Dynamic()
{
  SAFE_DELETE(vb);
  SAFE_DELETE(ib);

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

bool Dynamic::close()
{
  return true;
}

#define RETURN_ON_FALSE(x) if (!(x)) { return false; } else {}

bool Dynamic::init() 
{
  RETURN_ON_FALSE(effect_.load("effects/countdown.fx"));
  RETURN_ON_FALSE(dynamic_mgr_->init());

  D3D10_PASS_DESC desc;
  RETURN_ON_FALSE(effect_.get_pass_desc(desc, "render"));
  RETURN_ON_FALSE(vb->init(desc));
  RETURN_ON_FALSE(ib->init());
  return true;
}

void Dynamic::render(const int32_t time_in_ms, const int32_t delta)
{
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

  tentacle.render(time_in_ms);
}

void Dynamic::process_input_callback(const Input& input)
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
