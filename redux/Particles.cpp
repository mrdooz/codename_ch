#include "stdafx.h"
#include "Particles.hpp"
#include "Mesh.hpp"
#include "DebugRenderer.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

using namespace std;
using namespace boost::assign;

extern ID3D10Device* g_d3d_device;

namespace 
{
}


Particles::Particles(SystemInterface* system, EffectManager* effect_manager)
  : _system(system)
  , _effect_manager(effect_manager)
  , animation_manager_(new AnimationManager())
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , _dynamic_mgr(new DebugRenderer(g_d3d_device))
  , effect_(g_d3d_device)
{

  //ib = new IndexBuffer<uint32_t>(g_d3d_device);
  //vb = new VertexBuffer< mpl::vector<D3DXVECTOR3> >(g_d3d_device);

  _system->add_renderable(this);
  Serializer::instance().add_instance(this);

  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

}

Particles::~Particles()
{

  clear_vector(loaded_effects_);
  clear_vector(loaded_materials_);
  clear_vector(loaded_material_connections_);
  effect_list_.clear();
  effects_.clear();

  delete(xchg_null(_dynamic_mgr));
  container_delete(effect_connections_);

  xchg_null(_system);
  xchg_null(_effect_manager);
}

bool Particles::close()
{
  return true;
}

#define RETURN_ON_FALSE(x) if (!(x)) { return false; } else {}

bool Particles::init() 
{
  RETURN_ON_FALSE(effect_.load("effects/Particles.fx"));
  RETURN_ON_FALSE(_dynamic_mgr->init());

  D3D10_PASS_DESC desc;
  RETURN_ON_FALSE(effect_.get_pass_desc(desc, "render"));
  return true;
}
void Particles::render(const int32_t time_in_ms, const int32_t delta)
{
}

void Particles::process_input_callback(const Input& input)
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
