#include "stdafx.h"
#include "Scene.hpp"
#include "Mesh.hpp"

Scene::~Scene()
{
  reset();
}

MeshSPtr Scene::find_mesh_by_name(const std::string& mesh_name) 
{
  for (size_t i = 0; i < meshes_.size(); ++i) {
    if (meshes_[i]->name() == mesh_name) {
      return meshes_[i];
    }
  }
  return MeshSPtr();
}

void Scene::reset() 
{
  clear_vector(lights_);
  clear_vector(cameras_);
  clear_vector(meshes_);
}

void Scene::create_input_layout(SystemInterface& system, const D3D10_PASS_DESC& desc)
{
  for (size_t i = 0; i < meshes_.size(); ++i) {
    meshes_[i]->create_input_layout(system, desc);
  }
}
