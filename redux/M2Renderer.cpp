#include "stdafx.h"
#include "M2Renderer.hpp"
#include "Mesh.hpp"
#include "ReduxLoader.hpp"
#include "M2Loader.hpp"
#include <celsus/D3D10Descriptions.hpp>
#include "../system/Input.hpp"
#include "../system/SystemInterface.hpp"
#include <yajl/yajl_parse.h>
#include "EffectWrapper.hpp"

namespace mpl = boost::mpl;

namespace 
{
  const float width = 800;
  const float height = 600;
  const float near_plane = 0.1f;
  const float far_plane = 1000;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;
}

M2Renderer::M2Renderer(const SystemSPtr& system, const EffectManagerSPtr& effect_manager)
  : system_(system)
  , effect_manager_(effect_manager)
  , animation_manager_(new AnimationManager())
  , opaque_blend_state_(NULL)
  , transparent_blend_state_(NULL)
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , effect_(NULL)
{
  system_->add_renderable(this);

  opaque_blend_state_ = rt::D3D10::BlendDescription()
    .BlendEnable_(0, FALSE)
    .Create(system_->get_device());

  transparent_blend_state_ = rt::D3D10::BlendDescription()
    .BlendEnable_(0, TRUE)
    .SrcBlend_(D3D10_BLEND_SRC_COLOR)
    .DestBlend_(D3D10_BLEND_INV_SRC_COLOR)
    .BlendOp_(D3D10_BLEND_OP_ADD)
    .SrcBlendAlpha_(D3D10_BLEND_SRC_ALPHA)
    .DestBlendAlpha_(D3D10_BLEND_INV_SRC_ALPHA)
    .BlendOpAlpha_(D3D10_BLEND_OP_ADD)
    .RenderTargetWriteMask_(0, 0x0f)
    .Create(system_->get_device());

  opaque_depth_stencil_state_ = rt::D3D10::DepthStencilDescription()
    .Create(system_->get_device());

  transparent_depth_stencil_state_ = rt::D3D10::DepthStencilDescription()
    .DepthWriteMask_(D3D10_DEPTH_WRITE_MASK_ZERO)
    .Create(system_->get_device());

  effect_ = new EffectWrapper(g_d3d_device);
}

M2Renderer::~M2Renderer()
{
  SAFE_DELETE(effect_);
  clear_vector(loaded_effects_);
  clear_vector(loaded_materials_);
  clear_vector(loaded_material_connections_);
  effect_list_.clear();
  effects_.clear();

  SAFE_RELEASE(opaque_blend_state_);
  SAFE_RELEASE(transparent_blend_state_);
  SAFE_RELEASE(opaque_depth_stencil_state_);
  SAFE_RELEASE(transparent_depth_stencil_state_);

  SAFE_DELETE(animation_manager_);
  container_delete(effect_connections_);
  system_.reset();
  effect_manager_.reset();
}

bool M2Renderer::init() 
{
  //      bind(&Foo:func, &foo, _1, _2);

  system_->add_event_handler(EventId::FileChanged, fastdelegate::bind(&M2Renderer::file_changed, this));
  load_scene("data/scenes/gununit.rdx");
  return true;
}

bool M2Renderer::close()
{
  return true;
}


void M2Renderer::render(const int32_t time_in_ms, const int32_t delta) 
{
  D3DXVECTOR3 eye_pos;
  D3DXMATRIX mtx_proj;
  D3DXMATRIX mtx_view;

  system_->get_free_fly_camera(eye_pos, mtx_view);
  D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, near_plane, far_plane);

  ID3D10Device* device = system_->get_device();
  system_->set_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  EffectWrapper* e = effect_connections_.front()->effect_;

  EffectConnection* ec = effect_connections_.front();
  const Meshes& meshes = ec->opaque_materials_.front()->meshes_;

  e->set_variable("view", mtx_view);
  e->set_variable("projection", mtx_proj);
  e->set_variable("eye_pos", eye_pos);
  e->set_variable("world", kMtxId);
  e->set_resource("diffuse_texture", scene_.textures_.front());
  e->set_technique("render");
  meshes.front()->render();
}

void M2Renderer::process_input_callback(const Input& input)
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



void M2Renderer::load_scene(const std::string& filename) 
{
  SCOPED_FUNC_PROFILE();

  M2Loader l;
  //l.load("data//scenes//Draenei//Female//DraeneiFemale.m2", &scene_);
  //l.load("C:/projects/MpqExtract/dump/World/ArtTest/Boxtest/xyz.m2", &scene_);
//  l.load("C:/projects/MpqExtract/dump/World/critter/bats/bat02.m2", &scene_);
  l.load("C:/projects/MpqExtract/dump/Creature/madscientist/madscientist.m2", &scene_);
  
//  l.load("data/scenes/madscientist/madscientist.m2", &scene_);

  effect_->load("effects/m2_effect.fx");

  EffectConnection* ec = new EffectConnection();
  ec->effect_ = effect_;
  MaterialConnection* mc = new MaterialConnection();
  mc->meshes_ = scene_.meshes_;
  ec->opaque_materials_.push_back(mc);
  effect_connections_.push_back(ec);

  D3D10_PASS_DESC desc;
  effect_->get_pass_desc(desc, "render");
  ID3D10InputLayout* layout = NULL;

  if (!create_input_layout<mpl::vector<D3DXVECTOR3, D3DXNORMAL, D3DXVECTOR2> >(layout, desc, g_d3d_device)) {
    LOG_ERROR("Error creating input layout");
    return;
  }

  mc->meshes_.front()->set_input_layout(layout);

  system_->add_process_input_callback(boost::bind(&M2Renderer::process_input_callback, this, _1));
}

void M2Renderer::file_changed(const EventArgs* args)
{

}
