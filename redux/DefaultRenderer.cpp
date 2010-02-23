#include "stdafx.h"
#include "DefaultRenderer.hpp"
#include "Mesh.hpp"
#include "ReduxLoader.hpp"
#include "M2Loader.hpp"
#include <celsus/D3D10Descriptions.hpp>
#include "../system/Input.hpp"
#include "../system/SystemInterface.hpp"
#include <yajl/yajl_parse.h>

using namespace std;
using namespace boost::assign;

#ifndef STANDALONE
namespace py = boost::python;
#endif

namespace 
{

#ifndef STANDALONE
  typedef std::list<std::string> StringList;
  template<typename T>
  std::list<T> sequence_to_stl_list(const py::object& ob) 
  {
    py::stl_input_iterator<T> begin(ob), end;
    return std::list<T>(begin, end);
  }
#endif
  const float width = 800;
  const float height = 600;
  const float far_plane = 100;
  const float fov = static_cast<float>(D3DX_PI) * 0.25f;
  const float aspect_ratio = width / height;
}

DefaultRenderer::DefaultRenderer(
#ifdef STANDALONE
  const SystemSPtr& system, 
  const EffectManagerSPtr& effect_manager
#else
  boost::python::object effect_manager, 
  boost::python::object material_manager
#endif
  )
#ifdef STANDALONE
  : system_(system)
  , effect_manager_(effect_manager)
#else
  : system_(system)
  , effect_manager_(effect_manager)
  , material_manager_(material_manager)
#endif
  , animation_manager_(new AnimationManager())
  , opaque_blend_state_(NULL)
  , transparent_blend_state_(NULL)
  , current_camera_(0)
  , free_fly_camera_enabled_(true)
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


  LOG_MGR.
    EnableOutput(LogMgr::File).
    OpenOutputFile("log.txt").
    BreakOnError(true);

}

DefaultRenderer::~DefaultRenderer()
{
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

bool DefaultRenderer::init() 
{
  load_scene("data/scenes/gununit.rdx");
  return true;
}

bool DefaultRenderer::close()
{
  return true;
}

void DefaultRenderer::render_meshes(ID3D10BlendState* blend_state, ID3D10DepthStencilState* depth_state, MeshesByMaterial& meshes)
{
  float blend_factor[] = {0, 0, 0, 0};
  system_->get_device()->OMSetBlendState(blend_state, blend_factor, 0xffffffff);
  system_->get_device()->OMSetDepthStencilState(depth_state, 0xffffffff);

  for (MeshesByMaterial::iterator it = meshes.begin(); it != meshes.end(); ++it) {
    const string& material_name = it->first;
#ifdef STANDALONE
#else
    PyObj material = material_manager_.attr("get_material")(material_name);
    effect_manager_.attr("apply_material")(material);
#endif

    FOREACH(MeshSPtr m, it->second) {

      const D3DXMATRIX world_mtx = m->animation_node()->transform();
      system_->set_variable("world", world_mtx);

#ifdef STANDALONE
#else
      if (material_manager_.attr("material_has_texture")(material)) {
        system_->set_active_technique("render");
      } else {
        system_->set_active_technique("render_no_texture");
      }
#endif

      m->render(*system_);
    }
  }
}
/*
void set_render_targets(const RenderTargets& render_targets) 
{
  for (size_t i = 0; i < render_targets.size(); ++i) {
  }
}
*/

#ifdef STANDALONE
void DefaultRenderer::render(const int /*time_in_ms*/) 
{
  //set_render_targets(list_of(make_pair(0, "default"))(make_pair(1, "default")));
  system_->set_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (uint32_t i = 0; i < effect_connections_.size(); ++i) {
    EffectConnection* ec = effect_connections_[i];

    system_->set_active_effect(ec->effect_);

    D3DXVECTOR3 eye_pos;
    D3DXMATRIX mtx_proj;
    D3DXMATRIX mtx_view;

    static uint32_t start_time = timeGetTime();
    const uint32_t cur_time = timeGetTime();
    const uint32_t elapsed_time = (cur_time - start_time) / 10;

    animation_manager_->update_transforms(elapsed_time);

    if (!free_fly_camera_enabled_ && scene_.cameras_.size() > 0) {

      CameraPtr camera = scene_.cameras_[current_camera_];

      D3DXMATRIX mtx_cam;
      const char* shape_suffix = strstr(camera->name().c_str(), "Shape");
      char transform_name[256];
      const int32_t len = shape_suffix - camera->name().c_str();
      strncpy_s(transform_name, sizeof(transform_name), camera->name().c_str(), len);
      transform_name[len] = 0;

      animation_manager_->get_transform_for_node(eye_pos, "camera1_group|camera1");
      D3DXVECTOR3 up;
      animation_manager_->get_transform_for_node(up, "camera1_group|camera1_up");
      D3DXVECTOR3 aim;
      animation_manager_->get_transform_for_node(aim, "camera1_group|camera1_aim");
      camera->set_pos_up_dir(eye_pos, up, aim - eye_pos);

      mtx_proj = camera->projection_matrix();
      mtx_view = camera->view_matrix();
    } else {
      system_->get_free_fly_camera(eye_pos, mtx_view);
      D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, 1, far_plane);
    }

    system_->set_variable("projection", mtx_proj);
    system_->set_variable("view", mtx_view);
    system_->set_variable("eye_pos", eye_pos);

    float blend_factor[] = {0, 0, 0, 0};
    system_->get_device()->OMSetBlendState(opaque_blend_state_, blend_factor, 0xffffffff);
    system_->get_device()->OMSetDepthStencilState(opaque_depth_stencil_state_, 0xffffffff);

    for (uint32_t j = 0; j < ec->opaque_materials_.size(); ++j) {
      MaterialConnection* con = ec->opaque_materials_[j];
      con->material_->set_variables();
      for (Meshes::iterator it = con->meshes_.begin(); it != con->meshes_.end(); ++it) {
        MeshSPtr m = *it;

        const D3DXMATRIX world_mtx = m->animation_node()->transform();
        system_->set_variable("world", world_mtx);
        system_->set_active_technique("render_no_texture");
        m->render(*system_);

      }
    }
  }
}
#else
void DefaultRenderer::render(const int /*time_in_ms*/) 
{
  //set_render_targets(list_of(make_pair(0, "default"))(make_pair(1, "default")));
  system_->set_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (MaterialsByEffect::iterator it = effect_list_.begin(); it != effect_list_.end(); ++it) {
    const string& effect_name = it->first;
    system_->set_active_effect(safe_find_value(effects_, effect_name));
    system_->set_variable("world", kMtxId);

    D3DXVECTOR3 eye_pos;
    D3DXMATRIX mtx_proj;
    D3DXMATRIX mtx_view;

    static uint32_t start_time = timeGetTime();
    const uint32_t cur_time = timeGetTime();
    const uint32_t elapsed_time = (cur_time - start_time) / 10;

    animation_manager_->update_transforms(elapsed_time);

    if (!free_fly_camera_enabled_ && scene_.cameras_.size() > 0) {

      CameraPtr camera = scene_.cameras_[current_camera_];

      D3DXMATRIX mtx_cam;
      const char* shape_suffix = strstr(camera->name().c_str(), "Shape");
      char transform_name[256];
      const int32_t len = shape_suffix - camera->name().c_str();
      strncpy_s(transform_name, sizeof(transform_name), camera->name().c_str(), len);
      transform_name[len] = 0;

      animation_manager_->get_transform_for_node(eye_pos, "camera1_group|camera1");
      D3DXVECTOR3 up;
      animation_manager_->get_transform_for_node(up, "camera1_group|camera1_up");
      D3DXVECTOR3 aim;
      animation_manager_->get_transform_for_node(aim, "camera1_group|camera1_aim");
      camera->set_pos_up_dir(eye_pos, up, aim - eye_pos);

      mtx_proj = camera->projection_matrix();
      mtx_view = camera->view_matrix();
    } else {
      system_->get_free_fly_camera(eye_pos, mtx_view);
      D3DXMatrixPerspectiveFovLH(&mtx_proj, fov, aspect_ratio, 1, far_plane);
    }

    system_->set_variable("projection", mtx_proj);
    system_->set_variable("view", mtx_view);
    system_->set_variable("eye_pos", eye_pos);

    render_meshes(opaque_blend_state_, opaque_depth_stencil_state_, it->second.opaque_materials_);
    render_meshes(transparent_blend_state_, transparent_depth_stencil_state_, it->second.transparent_materials_);
  }

}
#endif

void DefaultRenderer::process_input_callback(const Input& input)
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



#ifdef STANDALONE
void DefaultRenderer::load_scene(const std::string& filename) 
{
  SCOPED_FUNC_PROFILE();

  ReduxLoader loader(filename, &scene_, system_.get(), animation_manager_);
  loader.load();

  boost::filesystem::path path(filename);
  const string raw_filename(path.replace_extension().filename());


  effects_["blinn_effect"] = system_->load_effect("blinn_effect");
  effects_["blinn_effect2"] = system_->load_effect("blinn_effect2");

  std::vector< std::string > textures;
  std::string json_filename("data/scenes/" + raw_filename + ".json");

  json::json_element elem;
  if (!json::json_element::load_from_file(elem, json_filename.c_str())) {
    LOG_WARNING_LN("Error loading JSON: %s", json_filename.c_str());
    return;
  }

  const json::json_element& effect_connections = elem.find_object("effect_connections");
  const json::json_element& materials = elem.find_object("materials");
  const json::json_element& material_connections = elem.find_object("material_connections");

  if (!(effect_connections.is_valid() && materials.is_valid() && material_connections.is_valid())) {
    LOG_WARNING_LN("Invalid JSON file: %s", json_filename.c_str());
    return;
  }

  ID3D10Effect* effect = effect_manager_->effect_from_handle(effects_["blinn_effect"]);
  {
    SCOPED_PROFILE("load materials");
    for (uint32_t i = 0; i < materials.length(); ++i ) {
      effect_manager_->add_material(load_material_fron_json(materials[i], effect));
    }
  }

  typedef std::vector< MeshName > MeshNames;
  typedef std::map< MaterialName, MeshNames > MaterialConnections;
  MaterialConnections material_connections2;

  const json::json_element::JsonList& cons = material_connections.get<const json::json_element::JsonList&>();
  for (uint32_t i = 0; i < material_connections.length(); ++i ) {
    const json::json_element& cur = cons[i];
    const std::string& mesh_name(cur.find_object("mesh").get<std::string>());
    const std::string& material_name(cur.find_object("material").get<std::string>());
    material_connections2[material_name].push_back(mesh_name);
  }

  for (uint32_t i = 0; i < effect_connections.length(); ++i) {

    const json::json_element& cur_effect = effect_connections[i];
    const std::string& effect_name(cur_effect.find_object("effect").get<std::string>());
    LPD3D10EFFECT d3d_effect = effect_manager_->effect_from_handle(effects_[effect_name]);
    const json::json_element& materials = cur_effect.find_object("materials");

    EffectConnection* ec = new EffectConnection();
    ec->effect_ = d3d_effect;

    for (uint32_t j = 0; j < materials.length(); ++j) {
      const std::string cur_material_name(materials[j].get<std::string>());
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

  create_input_layout();
  create_mesh_lists();

  system_->add_process_input_callback(boost::bind(&DefaultRenderer::process_input_callback, this, _1));
}
#else
void DefaultRenderer::load_scene(const std::string& filename) 
{
  SCOPED_FUNC_PROFILE();
  ReduxLoader loader(filename, &scene_, system_, animation_manager_);
  loader.load();

  boost::filesystem::path path(filename);
  const string raw_filename(path.replace_extension().filename());

  effects_["blinn_effect"] = system_->load_effect("blinn_effect");
  effects_["blinn_effect2"] = system_->load_effect("blinn_effect2");

  try {
    material_manager_.attr("load_materials_from_file")(raw_filename + "_materials");
    material_manager_.attr("load_material_connections")(raw_filename + "_scene");
  } catch (py::error_already_set&) {
    LOG_ERROR_LN("Error loading from file: %s", filename.c_str());
    return;
  }
  StringList textures = sequence_to_stl_list<std::string>(material_manager_.attr("get_textures")());

  FOREACH(string texture, textures) {
    system_->load_texture(texture);
  }

  create_input_layout();
  create_mesh_lists();

  system_->add_process_input_callback(boost::bind(&DefaultRenderer::process_input_callback, this, _1));
  system_->add_material_changed_callback(boost::bind(&DefaultRenderer::material_changed, this, _1, _2));
}
#endif


void DefaultRenderer::material_changed(const MeshName& mesh_name, const MaterialName& material_name)
{
  create_mesh_lists();
}

void DefaultRenderer::create_input_layout()
{
  D3D10_PASS_DESC desc;
  system_->get_technique_pass_desc(safe_find_value(effects_, "blinn_effect"), "render", desc);
  scene_.create_input_layout(*system_, desc);
}

void DefaultRenderer::add_material_connection(const std::string& mesh_name, const std::string& material_name)
{
}

#ifdef STANDALONE
void DefaultRenderer::create_mesh_lists()
{
}
#else
void DefaultRenderer::create_mesh_lists()
{
  effect_list_.clear();
  StringList effects = sequence_to_stl_list<std::string>(effect_manager_.attr("get_effects")());
  for (StringList::iterator it_effect = effects.begin(); it_effect != effects.end(); ++it_effect) {
    const string cur_effect = *it_effect;
    StringList materials = sequence_to_stl_list<std::string>(effect_manager_.attr("get_materials_for_effect")(cur_effect));
    MeshLists& mesh_lists = effect_list_[cur_effect];

    for (StringList::iterator it_mat = materials.begin(); it_mat != materials.end(); ++it_mat) {
      const string cur_material(*it_mat);
      PyObj material(material_manager_.attr("get_material")(cur_material));
      const bool is_transparent = material_manager_.attr("is_transparent")(material);
      StringList meshes = sequence_to_stl_list<std::string>(material_manager_.attr("get_meshes_for_material")(cur_material));

      for (StringList::iterator it_mesh = meshes.begin(); it_mesh != meshes.end(); ++it_mesh) {
        const string mesh_name(*it_mesh);
        MeshPtr mesh(scene_.find_mesh_by_name(mesh_name));
        if (mesh.get() != 0) {
          if (is_transparent) {
            mesh_lists.transparent_materials_[cur_material].push_back(mesh);
          } else {
            mesh_lists.opaque_materials_[cur_material].push_back(mesh);
          }
        } else {
          LOG_WARNING_LN("Unable to find mesh: %s", mesh_name.c_str());
        }
      }
    }

  }
}
#endif

