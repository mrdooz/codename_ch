#include "stdafx.h"
#include "ShadowRenderer.hpp"
#include "Mesh.hpp"
#include "ReduxLoader.hpp"
#include "EffectWrapper.hpp"
#include <celsus/D3D10Descriptions.hpp>
#include "../system/Input.hpp"
#include "../system/SystemInterface.hpp"
#include "../system/EffectManager.hpp"
#include "../system/ResourceManager.hpp"
#include "RenderableTexture2D.hpp"
#include "DebugRenderer.hpp"
#include "TextureCache.hpp"
#include "PostProcess.hpp"

using namespace std;
using namespace boost::assign;

namespace 
{
  const uint32_t kShadowMapSize = 512;
  const uint32_t kShadowMapWidth = kShadowMapSize;
  const uint32_t kShadowMapHeight = kShadowMapSize;

  D3DXVECTOR3 light_pos(0, 100, 0);
  D3DXVECTOR3 light_dir(0,-1,0);
  D3DXVECTOR3 light_up(0,0,-1);


  const float width = 800;
  const float height = 600;
  const float near_plane = 0.1f;
  const float far_plane = 1000;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;

  struct StateMachine;

  ID3D10Effect* g_haxor_effect = NULL;

  typedef std::vector< MeshName > MeshNames;
  typedef std::vector< MaterialName > MaterialNames;
  typedef stdext::hash_map< MaterialName, MeshNames > MaterialConnections;
  typedef stdext::hash_map< EffectName, MaterialNames > EffectConnections;

  struct StateBase
  {
    StateBase(StateMachine* state_machine) : done_(false), array_stack_(INT_MIN), state_machine_(state_machine) {}
    virtual ~StateBase() {};

    virtual bool is_done() const { return done_; }

    virtual int cb_null() { return 1; }
    virtual int cb_boolean(int value) { return 1; }
    virtual int cb_int(long value) { return 1; }
    virtual int cb_double(double value) { return 1; }
    virtual int cb_number(const char* str, const uint32_t len) { return 1; }
    virtual int cb_string(const char* str, const uint32_t len) { return 1; }
    virtual int cb_map_start() { return 1; }
    virtual int cb_map_end() { return 1; }
    virtual int cb_map_key(const char* str, const uint32_t len) { return 1; }
    virtual int cb_array_start() 
    { 
      if (array_stack_ == INT_MIN) {
        array_stack_ = 0;
      }
      ++array_stack_; 
      return 1; 
    }
    virtual int cb_array_end() { --array_stack_; return 1; }

    bool done_;
    int32_t array_stack_;
    StateMachine* state_machine_;
  };

  struct StateMachine
  {
    StateMachine(MaterialConnections& mat_cons, EffectConnections& effect_cons, std::vector<Material*>& materials);

    // proxy callbacks
    static int cb_null(void* ctx);
    static int cb_boolean(void* ctx, int value);
    static int cb_int(void* ctx, long value);
    static int cb_double(void* ctx, double value);
    static int cb_number(void* ctx, const char* str, const uint32_t len);
    static int cb_string(void* ctx, const uint8_t* str, const uint32_t len);
    static int cb_map_start(void* ctx);
    static int cb_map_end(void* ctx);
    static int cb_map_key(void* ctx, const uint8_t* str, const uint32_t len);
    static int cb_array_start(void* ctx);
    static int cb_array_end(void* ctx);

    int cb_null();
    int cb_boolean(int value);
    int cb_int(long value);
    int cb_double(double value);
    int cb_number(const char* str, const uint32_t len);
    int cb_string(const char* str, const uint32_t len);
    int cb_map_start();
    int cb_map_end();
    int cb_map_key(const char* str, const uint32_t len);
    int cb_array_start();
    int cb_array_end();
    void check_for_new_state();

    typedef std::stack< StateBase* > StateStack;

    MaterialConnections& mat_cons_;
    EffectConnections& effect_cons_;
    std::vector<Material*>& materials_;
    StateStack state_stack_;
    static yajl_callbacks callbacks_;
  };


  struct ColorState : public StateBase
  {
    ColorState(StateMachine* state_machine, Material* material, const std::string& var_name)
      : StateBase(state_machine)
      , material_(material)
      , var_name_(var_name)
      , idx_(0) 
    {
    }

    virtual bool is_done() const
    {
      if (array_stack_ == 0) {
        // done
        if (ID3D10EffectVariable* effect_variable = g_haxor_effect->GetVariableByName(var_name_.c_str())) {
          if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
            material_->color_variables_.push_back( std::make_pair(typed_variable, value_));
          }
        }
      }
      return array_stack_ == 0;
    }

    virtual int cb_string(const char* str, const uint32_t len) 
    { 
      value_[idx_++] = (float)atof(str);
      done_ = true;
      return 1; 
    }

    virtual int cb_double(double value) 
    { 
      value_[idx_++] = (float)value;
      done_ = true;
      return 1; 
    }

    virtual int cb_number(const char* str, const uint32_t len) 
    { 
      value_[idx_++] = (float)atof(str);
      done_ = true;
      return 1; 
    }

    Material* material_;
    const std::string& var_name_;
    D3DXCOLOR value_;
    uint32_t idx_;
  };

  struct StringState : public StateBase
  {
    StringState(StateMachine* sm, std::string& value) : StateBase(sm), value_(value) {}
    virtual int cb_string(const char* str, const uint32_t len) 
    { 
      value_ = std::string(str, len);
      done_ = true;
      return 1; 
    }
    std::string& value_;
  };

  struct StringIdState : public StateBase
  {
    StringIdState(StateMachine* sm, StringId& value) : StateBase(sm), value_(value) {}
    virtual int cb_string(const char* str, const uint32_t len) 
    { 
      char* tmp = (char*)_alloca(len + 1);
      memcpy(tmp, str, len);
      tmp[len] = 0;
      value_.set(tmp);
      done_ = true;
      return 1; 
    }
    StringId& value_;
  };

  struct FloatState : public StateBase
  {
    FloatState(StateMachine* sm, float& value) : StateBase(sm), value_(value) {}

    virtual int cb_string(const char* str, const uint32_t len) 
    { 
      value_ = (float)atof(str);
      done_ = true;
      return 1; 
    }

    virtual int cb_double(double value) 
    { 
      value_ = (float)value;
      done_ = true;
      return 1; 
    }

    virtual int cb_number(const char* str, const uint32_t len) 
    { 
      value_ = (float)atof(str);
      done_ = true;
      return 1; 
    }

    float& value_;
  };

  struct StringListState : public StateBase
  {
    typedef std::vector< std::string > StringList;
    StringListState(StateMachine* sm, StringList& string_list) : StateBase(sm), string_list_(string_list) {}

    virtual bool is_done() const
    {
      return array_stack_ == 0;
    }

    virtual int cb_string(const char* str, const uint32_t len) 
    { 
      string_list_.push_back( std::string(str, len));
      return 1; 
    }

    StringList& string_list_;

  };

  struct ValueState : public StateBase
  {
    ValueState(StateMachine* sm, Material* mat) : StateBase(sm), material_(mat) {}

    virtual bool is_done() const
    {
      return array_stack_ == 0;
    }

    virtual int cb_map_key(const char* str, const uint32_t len) 
    { 
      if (0 == strncmp("name", str, len)) {
        state_machine_->state_stack_.push(new StringState(state_machine_, value_name_));
      } else if (0 == strncmp("type", str, len)) {
        state_machine_->state_stack_.push(new StringState(state_machine_, value_type_));
      } else if (0 == strncmp("value", str, len)) {

        if (value_type_ == "float") {
          if (value_name_ == "transparency") {
            state_machine_->state_stack_.push(new FloatState(state_machine_, material_->transparency_));
          } else {

          }
          //material_->floats_.push_back(FloatValue());
          //material_->floats_.back().name = value_name_;
          //state_machine_->state_stack_.push(new FloatState(state_machine_, material_->floats_.back().value));

        } else if (value_type_ == "color") {
          state_machine_->state_stack_.push(new ColorState(state_machine_, material_, value_name_));
        }
      }

      return 1; 
    }

    std::string value_name_;
    std::string value_type_;
    Material* material_;
  };



  struct MaterialConnectionsState : public StateBase
  {
    MaterialConnectionsState(StateMachine* sm, MaterialConnections& cons) : StateBase(sm), cons_(cons) {}
    virtual bool is_done() const 
    {
      return array_stack_ == 0;
    }

    virtual int cb_map_end()
    {
      cons_[mat].push_back(mesh);
      return 1;
    }

    virtual int cb_map_key(const char* str, const uint32_t len) 
    { 
      if (0 == strncmp("mesh", str, len)) {
        state_machine_->state_stack_.push(new StringState(state_machine_, mesh));
      } else if (0 == strncmp("material", str, len)) {
        state_machine_->state_stack_.push(new StringState(state_machine_, mat));
      }

      return 1; 
    }

    std::string mesh;
    std::string mat;
    MaterialConnections& cons_;
  };

  struct EffectConnectionsState : public StateBase
  {
    EffectConnectionsState(StateMachine* sm, EffectConnections& cons) : StateBase(sm), cons_(cons) {}
    virtual bool is_done() const 
    {
      return array_stack_ == 0;
    }

    virtual int cb_map_key(const char* str, const uint32_t len) 
    { 
      if (0 == strncmp("effect", str, len)) {
        state_machine_->state_stack_.push(new StringState(state_machine_, effect));
      } else if (0 == strncmp("materials", str, len)) {
        cons_[effect] = StringListState::StringList();
        state_machine_->state_stack_.push(new StringListState(state_machine_, cons_[effect]));
      }

      return 1; 
    }

    std::string effect;
    EffectConnections& cons_;
  };


  struct MaterialsState : public StateBase
  {
    MaterialsState(StateMachine* sm, std::vector<Material*>& materials) : StateBase(sm), materials_(materials) {}
    virtual bool is_done() const
    {
      return array_stack_ == 0;
    }

    virtual int cb_map_key(const char* str, const uint32_t len) 
    { 
      if (0 == strncmp("name", str, len)) {
        Material* m = new Material();
        state_machine_->state_stack_.push(new StringIdState(state_machine_, m->name_));
        materials_.push_back(m);
      } else if (0 == strncmp("values", str, len)) {
        state_machine_->state_stack_.push(new ValueState(state_machine_, materials_.back()));
      }

      return 1; 
    }

    std::vector<Material*>& materials_;
  };

  StateMachine::StateMachine(MaterialConnections& mat_cons, EffectConnections& effect_cons, std::vector<Material*>& materials)
    : mat_cons_(mat_cons)
    , effect_cons_(effect_cons)
    , materials_(materials)
  {

  }

  int StateMachine::cb_null(void* ctx)
  {
    return ((StateMachine*)ctx)->cb_null();
  }

  int StateMachine::cb_boolean(void* ctx, int value)
  {
    return ((StateMachine*)ctx)->cb_boolean(value);
  }

  int StateMachine::cb_int(void* ctx, long value)
  {
    return ((StateMachine*)ctx)->cb_boolean(value);
  }

  int StateMachine::cb_double(void* ctx, double value)
  {
    return ((StateMachine*)ctx)->cb_double(value);
  }

  int StateMachine::cb_number(void* ctx, const char* str, const uint32_t len)
  {
    return ((StateMachine*)ctx)->cb_number(str, len);
  }

  int StateMachine::cb_string(void* ctx, const uint8_t* str, const uint32_t len)
  {
    return ((StateMachine*)ctx)->cb_string((const char*)str, len);
  }

  int StateMachine::cb_map_start(void* ctx)
  {
    return ((StateMachine*)ctx)->cb_map_start();
  }

  int StateMachine::cb_map_end(void* ctx)
  {
    return ((StateMachine*)ctx)->cb_map_end();
  }

  int StateMachine::cb_map_key(void* ctx, const uint8_t* str, const uint32_t len)
  {
    return ((StateMachine*)ctx)->cb_map_key((const char*)str, len);
  }

  int StateMachine::cb_array_start(void* ctx)
  {
    return ((StateMachine*)ctx)->cb_array_start();
  }

  int StateMachine::cb_array_end(void* ctx)
  {
    return ((StateMachine*)ctx)->cb_array_end();
  }

  int StateMachine::cb_null()
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_null();
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_boolean(int value)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_boolean(value);
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_int(long value)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_int(value);
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_double(double value)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_double(value);
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_number(const char* str, const uint32_t len)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_number(str, len);
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_string(const char* str, const uint32_t len)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_string(str, len);
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_map_start()
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_map_start();
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_map_end()
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_map_end();
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_map_key(const char* str, const uint32_t len)
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_map_key(str, len);
      check_for_new_state();
      return res;
    }

    std::string key((const char*)str, len);
    if (key == "materials") {
      state_stack_.push(new MaterialsState(this, materials_));
    } else if (key == "material_connections") {
      state_stack_.push(new MaterialConnectionsState(this, mat_cons_));
    } else if (key == "effect_connections") {
      state_stack_.push(new EffectConnectionsState(this, effect_cons_));
    }
    return 1;
  }

  int StateMachine::cb_array_start()
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_array_start();
      check_for_new_state();
      return res;
    }

    return 1;
  }

  int StateMachine::cb_array_end()
  {
    if (!state_stack_.empty()) {
      const int res = state_stack_.top()->cb_array_end();
      check_for_new_state();
      return res;
    }

    return 1;
  }

  void StateMachine::check_for_new_state()
  {
    if (state_stack_.empty()) {
      return;
    }

    if (state_stack_.top()->is_done()) {
      StateBase* top = state_stack_.top();
      SAFE_DELETE(top);
      state_stack_.pop();
    }
  }

  yajl_callbacks StateMachine::callbacks_ = 
  {
    StateMachine::cb_null,
    StateMachine::cb_boolean,
    StateMachine::cb_int,
    StateMachine::cb_double,
    StateMachine::cb_number,
    StateMachine::cb_string,
    StateMachine::cb_map_start,
    StateMachine::cb_map_key,
    StateMachine::cb_map_end,
    StateMachine::cb_array_start,
    StateMachine::cb_array_end
  };


}

ShadowRenderer::ShadowRenderer(
  const SystemSPtr& system, 
  const EffectManagerSPtr& effect_manager
  )
  : system_(system)
  , effect_manager_(effect_manager)
  , animation_manager_(new AnimationManager())
  , opaque_blend_state_(NULL)
  , transparent_blend_state_(NULL)
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
  , light_fov_(45.0f)
  , light_dist_atten_(50, 200)
  , shadow_map_(NULL)
  , sort_meshes_(true)
  , cull_meshes_(true)
  , debug_draw_(false)
  , draw_meshes_(true)
  , visible_meshes_(0)
  , debug_renderer_(new DebugRenderer(system->get_device()))
  , m_FullTextures(new TextureCache())
  , m_GenerateSATRDSamples(8)
  , m_PostProcess(NULL)
  , min_filter_width_(1.0f)
{
  Serializer::instance().add_instance(this);

  ID3D10Device* device = system_->get_device();

  system_->add_renderable(this);
  debug_renderer_->init();

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
    .Create(device);

  opaque_depth_stencil_state_ = rt::D3D10::DepthStencilDescription()
    .Create(device);

  transparent_depth_stencil_state_ = rt::D3D10::DepthStencilDescription()
    .DepthWriteMask_(D3D10_DEPTH_WRITE_MASK_ZERO)
    .Create(device);

  shadow_effect_ = new EffectWrapper(device);

  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

  AddFullTextures(2);

}

ShadowRenderer::~ShadowRenderer()
{
  SAFE_DELETE(m_PostProcess);
  SAFE_DELETE(m_FullTextures);
  SAFE_DELETE(shadow_map_);
  SAFE_DELETE(shadow_effect_);
  clear_vector(loaded_effects_);
  clear_vector(loaded_materials_);
  clear_vector(loaded_material_connections_);
  effect_list_.clear();
  effect_handles_.clear();

  SAFE_RELEASE(opaque_blend_state_);
  SAFE_RELEASE(transparent_blend_state_);
  SAFE_RELEASE(opaque_depth_stencil_state_);
  SAFE_RELEASE(transparent_depth_stencil_state_);

  SAFE_DELETE(animation_manager_);
  container_delete(effect_connections_);
  system_.reset();
  effect_manager_.reset();
}

bool ShadowRenderer::init() 
{

  load_scene("data/scenes/gununit.rdx");

  create_shadow_textures();
  return true;
}

bool ShadowRenderer::close()
{
  return true;
}

struct SortedMesh
{
  SortedMesh(const float dist, Mesh* mesh) : dist(dist), mesh(mesh) {}
  float dist;
  Mesh* mesh;
};

bool operator<(const SortedMesh& a, const SortedMesh& b)
{
  return a.dist < b.dist;
}

bool operator>(const SortedMesh& a, const SortedMesh& b)
{
  return a.dist > b.dist;
}

float distance_to_point(const D3DXPLANE& plane, const D3DXVECTOR3& pt)
{
  return plane.a * pt.x + plane.b * pt.y + plane.c * pt.z + plane.d;
}

void planes_from_proj_matrix(D3DXPLANE* planes, const D3DXMATRIX& proj, const bool normalize)
{

  // Left clipping plane
  planes[0].a = proj._14 + proj._11;
  planes[0].b = proj._24 + proj._21;
  planes[0].c = proj._34 + proj._31;
  planes[0].d = proj._44 + proj._41;

  // Right clipping plane
  planes[1].a = proj._14 - proj._11;
  planes[1].b = proj._24 - proj._21;
  planes[1].c = proj._34 - proj._31;
  planes[1].d = proj._44 - proj._41;

  // Top clipping plane
  planes[2].a = proj._14 - proj._12;
  planes[2].b = proj._24 - proj._22;
  planes[2].c = proj._34 - proj._32;
  planes[2].d = proj._44 - proj._42;

  // Bottom clipping plane
  planes[3].a = proj._14 + proj._12;
  planes[3].b = proj._24 + proj._22;
  planes[3].c = proj._34 + proj._32;
  planes[3].d = proj._44 + proj._42;

  // Near clipping plane
  planes[4].a = proj._13;
  planes[4].b = proj._23;
  planes[4].c = proj._33;
  planes[4].d = proj._43;

  // Far clipping plane
  planes[5].a = proj._14 - proj._13;
  planes[5].b = proj._24 - proj._23;
  planes[5].c = proj._34 - proj._33;
  planes[5].d = proj._44 - proj._43;

  // Normalize the plane equations, if requested
  if (normalize)
  {
    for (uint32_t i = 0; i < 6; ++i) {
      D3DXPlaneNormalize(&planes[i], &planes[i]);
    }
  }
}

bool is_sphere_visible(const D3DXVECTOR3& center, const float radius, const D3DXMATRIX& world, const D3DXMATRIX& view, const D3DXPLANE* planes)
{
  D3DXVECTOR3 ws_center;
  D3DXVec3TransformCoord(&ws_center, &center, &world);

  // transform center to view space
  D3DXVECTOR3 vs_center;
  D3DXVec3TransformCoord(&vs_center, &ws_center, &view);
  const float scale = D3DXVec3Length(&get_scale(world));
  const float scaled_radius = scale * radius;

  // if the signed distance is negative and greater than the bounding sphere radius,
  // then the sphere is outside the frustum
  for (uint32_t j = 0; j < 6; ++j) {
    const float dist = distance_to_point(planes[j], vs_center);
    if (dist < - scaled_radius) {
      return false;
    }
  }
  return true;
}

bool is_mesh_visible(D3DXVECTOR3& vs_center, MeshSPtr mesh, const D3DXMATRIX& view, D3DXPLANE* planes)
{
  const D3DXVECTOR3 center = mesh->bounding_sphere_center();
  const float radius = mesh->bounding_sphere_radius();
  const D3DXMATRIX world_mtx = mesh->animation_node()->transform();
  D3DXVECTOR3 ws_center;
  D3DXVec3TransformCoord(&ws_center, &center, &world_mtx);

  // transform center to view space
  D3DXVec3TransformCoord(&vs_center, &ws_center, &view);
  const float scale = D3DXVec3Length(&get_scale(world_mtx));
  const float scaled_radius = scale * radius;

  // if the signed distance is negative and greater than the bounding sphere radius,
  // then the sphere is outside the frustum
  for (uint32_t j = 0; j < 6; ++j) {
    const float dist = distance_to_point(planes[j], vs_center);
    if (dist < - scaled_radius) {
      return false;
    }
  }

  return true;
}

void ShadowRenderer::render_inner(const D3DXVECTOR3& pos, const D3DXMATRIX& view, const D3DXMATRIX& proj, const std::string& technique)
{
  ID3D10Device* device = system_->get_device();
  system_->set_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  EffectWrapper* e = effect_connections_.front()->effect_;

  e->set_variable("g_ViewPos", pos);

  D3DXMATRIX light_view;
  D3DXMatrixLookAtLH(&light_view, &light_pos, &(light_pos + light_dir), &light_up);
  D3DXMATRIX light_proj;
  float Near = 0.04f;
  float Far  = light_dist_atten_.y;
  D3DXMatrixPerspectiveFovLH(&light_proj, (float)D3DXToRadian(light_fov_), 1, Near, Far);

  D3DXPLANE planes[6];
  planes_from_proj_matrix(planes, proj, true);

  e->set_variable("g_LightPosition", light_pos);
  e->set_variable("g_LightDirection", light_dir);
  e->set_variable("g_LightFOV", (float)D3DXToRadian(light_fov_));
  e->set_variable("g_LightDistFalloff", light_dist_atten_);
  e->set_variable("g_ShadowTextureSize", D3DXVECTOR2((float)kShadowMapWidth, (float)kShadowMapHeight));
  e->set_variable("g_LightViewProjMatrix", light_view * light_proj);

  // Compute linear (distance to light) near and min/max for this perspective matrix
  float CosLightFOV = std::cos(0.5f * (float)D3DXToRadian(light_fov_));
  const float near2 = Near;
  const float far2 = Far / (CosLightFOV * CosLightFOV);

  e->set_variable("g_LightLinNearFar", D3DXVECTOR2(near2, far2));

  // Lighting parameters
  e->set_variable("g_AmbientIntensity", 0.1f);
  e->set_variable("g_LightColor", D3DXVECTOR3(1,1,1));

  // Surface parameters
  e->set_variable("g_SpecularPower", 10);
  e->set_variable("g_SpecularColor", D3DXVECTOR3(1,1,1));
  e->set_resource("texShadow", shadow_map_->GetShaderResource());

  ID3D10EffectMatrixVariable* world_mtx_var = NULL;
  ID3D10EffectMatrixVariable* world_view_mtx_var = NULL;
  ID3D10EffectMatrixVariable* world_view_proj_mtx_var = NULL;
  e->get_variable(world_mtx_var, "g_WorldMatrix");
  e->get_variable(world_view_mtx_var, "g_WorldViewMatrix");
  e->get_variable(world_view_proj_mtx_var, "g_WorldViewProjMatrix");

  e->set_technique(technique);

  ID3D10Buffer* object_cbuffer;
  e->get_cbuffer(object_cbuffer, "object_related");


  EffectConnection* ec = effect_connections_.front();
  const Meshes& meshes = ec->opaque_materials_.front()->meshes_;

  // sort the mesh wrt the eye pos
  std::vector<SortedMesh> sorted_meshes;
  sorted_meshes.reserve(meshes.size());
  D3DXVECTOR3 vs_center;
  visible_meshes_ = 0;

  for (uint32_t i = 0, e = meshes.size(); i < e; ++i) {

    const MeshSPtr& m = meshes[i];
    bool visible = true;
    if (cull_meshes_) {
      visible = is_mesh_visible(vs_center, m, view, planes);
    } else {
      D3DXVec3TransformCoord(&vs_center, &m->bounding_sphere_center(), &(m->animation_node()->transform() * view));
    }
    if (visible) {
      visible_meshes_++;
      const float dist = std::max<float>(0, D3DXVec3Length(&(vs_center - pos)) - m->bounding_sphere_radius());
      sorted_meshes.push_back(SortedMesh(dist, m.get()));
    }
  }

  if (sort_meshes_) {
    std::sort(sorted_meshes.begin(), sorted_meshes.end(), std::less<SortedMesh>());
  } else {
    std::sort(sorted_meshes.begin(), sorted_meshes.end(), std::greater<SortedMesh>());
  }

  for (uint32_t i = 0, e = sorted_meshes.size(); i < e; ++i) {

    Mesh* m = sorted_meshes[i].mesh;

    const D3DXMATRIX world_mtx = m->animation_node()->transform();
    const D3DXMATRIX world_view_mtx = world_mtx * view;
    const D3DXMATRIX world_view_proj_mtx = world_view_mtx * proj;

    D3DXMATRIX mm[3];
    D3DXMatrixTranspose(&mm[0], &world_mtx);
    D3DXMatrixTranspose(&mm[1], &world_view_mtx);
    D3DXMatrixTranspose(&mm[2], &world_view_proj_mtx);

    device->UpdateSubresource(object_cbuffer, D3D10CalcSubresource(0,0,1), NULL, (void*)&mm[0], sizeof(D3DXMATRIX) * 3, 0);

    m->render(*system_);
  }

}

void ShadowRenderer::render(const int /*time_in_ms*/) 
{
  ID3D10Device* device = system_->get_device();

  D3DXVECTOR3 eye_pos;
  D3DXMATRIX mtx_proj;
  D3DXMATRIX mtx_view;

  static uint32_t start_time = timeGetTime();
  const uint32_t cur_time = timeGetTime();
  const uint32_t elapsed_time = (cur_time - start_time) / 10;

  animation_manager_->update_transforms(elapsed_time);

  system_->get_free_fly_camera(eye_pos, mtx_view);
  D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, near_plane, far_plane);

  if (draw_meshes_) {
    ID3D10ShaderResourceView *const pSRV[1] = {NULL};
    device->PSSetShaderResources(0, 1, pSRV);

    if (shadow_map_ == NULL) {
      shadow_map_ = m_FullTextures->Get();
    }

    ID3D10RenderTargetView* rt = shadow_map_->GetRenderTarget();
    device->ClearRenderTargetView(rt, D3DXVECTOR4(0.5f, 1.0f, 0.0f, 0.0f));
    device->ClearDepthStencilView(depth_stencil_view_, D3D10_CLEAR_DEPTH, 1.0f, 0);

    // Setup shadow render target
    device->OMSetRenderTargets(1, &rt, depth_stencil_view_);
    device->RSSetViewports(1, &shadow_viewport_);

    D3DXMATRIX light_view;
    D3DXMatrixLookAtLH(&light_view, &light_pos, &(light_pos + light_dir), &light_up);
    D3DXMATRIX light_proj;
    float Near = 0.04f;
    float Far  = light_dist_atten_.y;
    D3DXMatrixPerspectiveFovLH(&light_proj, (float)D3DXToRadian(light_fov_), 1, Near, Far);

    render_inner(light_pos, light_view, light_proj, "Depth");

    shadow_effect_->set_technique("Depth");
    m_SAT = GenerateSATRecursiveDouble(shadow_map_, false);
    m_EffectSATTexture->SetResource(m_SAT->GetShaderResource());

    ID3D10RenderTargetView* rts[] = { system_->resource_manager()->get_render_target("default") };

    device->OMSetRenderTargets(1, 
      rts,
      system_->resource_manager()->get_depth_stencil_view("default"));

    device->RSSetViewports(1, &system_->viewport());

    var_min_filter_width_->SetFloat(min_filter_width_);

    render_inner(eye_pos, mtx_view, mtx_proj, "Shading");
    effect_connections_.front()->effect_->set_technique("Shading");
    device->PSSetShaderResources(0, 1, pSRV);

    m_FullTextures->Add(shadow_map_);
    shadow_map_ = NULL;
  }

  if (debug_draw_) {
    render_debug(mtx_view, mtx_proj);
  }

}

void ShadowRenderer::render_debug(const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
  debug_renderer_->start_frame();

  EffectConnection* ec = effect_connections_.front();
  const Meshes& meshes = ec->opaque_materials_.front()->meshes_;
  const D3DXMATRIX view_proj = view * proj;
  D3DXPLANE planes[6];
  planes_from_proj_matrix(planes, proj, true);

  for (uint32_t i = 0, e = meshes.size(); i < e; ++i) {

    const MeshSPtr& m = meshes[i];
    const D3DXMATRIX& world = m->animation_node()->transform();
    const D3DXVECTOR3& center = m->bounding_sphere_center();
    const float radius = m->bounding_sphere_radius();
    if (is_sphere_visible(center, radius, world, view, planes)) {
      debug_renderer_->add_wireframe_sphere(m->bounding_sphere_center(), m->bounding_sphere_radius(), D3DXCOLOR(1,1,1,1), world, view_proj);
    }
  }
  debug_renderer_->end_frame();
  debug_renderer_->render();
}

void ShadowRenderer::process_input_callback(const Input& input)
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


void ShadowRenderer::load_scene(const std::string& filename) 
{
  SCOPED_FUNC_PROFILE();
  ReduxLoader loader(filename, &scene_, system_.get(), animation_manager_);
  loader.load();

  boost::filesystem::path path(filename);
  const string raw_filename(path.replace_extension().filename());


  effect_handles_["blinn_effect"] = system_->load_effect("blinn_effect");
  effect_handles_["blinn_effect2"] = system_->load_effect("blinn_effect2");

  std::vector< std::string > textures;
  std::string json_filename("data/scenes/" + raw_filename + ".json");

  ID3D10Effect* effect = effect_manager_->effect_from_handle(effect_handles_["blinn_effect"]);
  g_haxor_effect = effect;

  MaterialConnections material_connections2;
  EffectConnections effect_cons;

  uint8_t* buf;
  uint32_t len;
  if (!load_file(buf, len, json_filename.c_str())) {
    throw std::exception("error loading json");
  }

  std::vector<Material*> materials2;
  StateMachine s(material_connections2, effect_cons, materials2);
  yajl_handle h = yajl_alloc(&s.callbacks_, NULL, NULL, &s);
  yajl_parse(h, buf, len);
  yajl_free(h);
  SAFE_ADELETE(buf);

  for (uint32_t i = 0, e = materials2.size(); i < e; ++i) {
    effect_manager_->add_material(materials2[i]);
  }

  EffectConnection* ec = new EffectConnection();
  ec->effect_ = shadow_effect_;
  MaterialConnection* mc = new MaterialConnection();
  mc->meshes_ = scene_.meshes_;
  ec->opaque_materials_.push_back(mc);
  effect_connections_.push_back(ec);
/*
  for (EffectConnections::iterator i = effect_cons.begin(), e = effect_cons.end(); i != e; ++i) {

    const std::string& effect_name(i->first);
    LPD3D10EFFECT d3d_effect = effect_manager_->effect_from_handle(effect_handles_[effect_name]);
    EffectConnection* ec = new EffectConnection();
    ec->effect_ = d3d_effect;

    for (MaterialNames::iterator i_m = i->second.begin(), e_m = i->second.end(); i_m != e_m; ++i_m) {
      const std::string& cur_material_name(*i_m);
      MaterialSPtr material_ptr = effect_manager_->find_material_by_name(StringId(cur_material_name.c_str()));

      MaterialConnection* mc = new MaterialConnection();
      mc->material_ = material_ptr;
      const MeshNames& mesh_names = material_connections2[cur_material_name];
      for (uint32_t k = 0; k < mesh_names.size(); ++k) {
        if (MeshSPtr mesh = scene_.find_mesh_by_name(mesh_names[k])) {
          mc->meshes_.push_back(mesh);
        } else {
          LOG_WARNING_LN("Unable to find mesh: %s", mesh_names[k].c_str());
        }
      }
      if (material_ptr->is_transparent()) {
        ec->transparent_materials_.push_back(mc);
      } else {
        ec->opaque_materials_.push_back(mc);
      }

    }

    effect_connections_.push_back(ec);
  }

  FOREACH(string texture, textures) {
    system_->load_texture(texture);
  }
*/

  create_input_layout();

  system_->add_process_input_callback(boost::bind(&ShadowRenderer::process_input_callback, this, _1));
}

void ShadowRenderer::material_changed(const MeshName& mesh_name, const MaterialName& material_name)
{
}

void ShadowRenderer::create_input_layout()
{
  D3D10_PASS_DESC desc;
  system_->get_technique_pass_desc(safe_find_value(effect_handles_, "blinn_effect"), "render", desc);
  scene_.create_input_layout(*system_, desc);
}

void ShadowRenderer::add_material_connection(const std::string& mesh_name, const std::string& material_name)
{
}

void ShadowRenderer::create_shadow_textures()
{
  init_d3d10_viewport(&shadow_viewport_, 0, 0, kShadowMapWidth, kShadowMapHeight, 0, 1);
  shadow_effect_->load("effects/satint.fx");
  shadow_effect_->set_variable("g_ShadowTextureSize", D3DXVECTOR4((FLOAT)kShadowMapWidth, (FLOAT)kShadowMapHeight, 0, 0));
  shadow_effect_->get_variable(m_EffectSATPassOffset, "g_SATPassOffset");
  char buf[256];
  sprintf_s(buf, sizeof(buf), "GenerateSATRD%d", m_GenerateSATRDSamples);
  shadow_effect_->get_technique(m_GenerateSATRDTechnique, buf);
  shadow_effect_->get_variable(var_min_filter_width_, "g_MinFilterWidth");
  shadow_effect_->get_resource(m_EffectSATTexture, "texSAT");

  m_PostProcess = new PostProcess(g_d3d_device, shadow_effect_->effect());

  ID3D10Device* device = system_->get_device();

  // create depth/stencil
  ID3D10DepthStencilView* depth_stencil_view_raw = NULL;
  ID3D10Texture2D* depth_stencil_raw = NULL;
  ENFORCE_HR(create_depth_stencil_and_view(depth_stencil_raw, depth_stencil_view_raw, device, kShadowMapWidth, kShadowMapHeight, DXGI_FORMAT_D32_FLOAT));
  depth_stencil_.Attach(depth_stencil_raw);
  depth_stencil_view_.Attach(depth_stencil_view_raw);
}


//--------------------------------------------------------------------------------------
RenderableTexture2D* ShadowRenderer::GenerateSATRecursiveDouble(RenderableTexture2D *Src, bool MaintainSrc)
{

  // Grab a temporary texture
  RenderableTexture2D *Dest = m_FullTextures->Get();

  // If we have to maintain the source, grab another temporary texture
  RenderableTexture2D *Temp = MaintainSrc ? m_FullTextures->Get() : Src;

  m_PostProcess->Begin(g_d3d_device, kShadowMapWidth, kShadowMapHeight, Dest, Src, Temp);

  // Horizontal pass
  for (int i = 1; i < kShadowMapWidth; i *= m_GenerateSATRDSamples) {
    int PassOffset[4] = {i, 0, 0, 0};
    ENFORCE_HR(m_EffectSATPassOffset->SetIntVector(PassOffset));

    // We need to still propogate samples that were "just finished" from
    // the last pass. Alternately could copy the texture first, but probably
    // not worth it.
    int Done =  i / m_GenerateSATRDSamples;
    // Also if we're maintaining the source, the second pass will still need
    // to write the whole texture, since it wasn't ping-ponged in the first one.
    Done = (Done <= 1 && MaintainSrc) ? 0 : Done;

    D3D10_RECT Region = {Done, 0, kShadowMapWidth, kShadowMapHeight};
    m_PostProcess->Apply(m_GenerateSATRDTechnique, Region);
  }

  // Vertical pass
  for (int i = 1; i < kShadowMapHeight; i *= m_GenerateSATRDSamples) {
    int PassOffset[4] = {0, i, 0, 0};
    ENFORCE_HR(m_EffectSATPassOffset->SetIntVector(PassOffset));
    int Done = i / m_GenerateSATRDSamples;
    D3D10_RECT Region = {0, Done, kShadowMapWidth, kShadowMapHeight};
    m_PostProcess->Apply(m_GenerateSATRDTechnique, Region);
  }

  RenderableTexture2D *Result = m_PostProcess->End();

  // Return any non-result textures to the available list
  m_FullTextures->Add(Dest);
  m_FullTextures->Add(Temp);
  m_FullTextures->Remove(Result);

  return Result;
}

void ShadowRenderer::AddFullTextures(int Num)
{
  const bool mip_mapped = false;
  DXGI_FORMAT format = DXGI_FORMAT_R32G32_UINT;
  // Create full-sized textures
  for (int i = 0; i < Num; ++i) {
    m_FullTextures->Add(new RenderableTexture2D(g_d3d_device, kShadowMapWidth, kShadowMapHeight, mip_mapped ? 0 : 1, format));
  }
}
