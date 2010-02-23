#ifndef SCENE_HPP
#define SCENE_HPP

#include "ReduxTypes.hpp"

struct SystemInterface;

// TODO: Figure out how to use the scene. It should probably just be a struct
struct Scene
{
public:

  typedef std::vector<MeshSPtr> Meshes;
  ~Scene();
  MeshSPtr find_mesh_by_name(const std::string& mesh_name);
  void reset();
  void create_input_layout(SystemInterface& system, const D3D10_PASS_DESC& desc);
  friend class ReduxLoader;

  std::vector<LightPtr> lights_;
  std::vector<CameraPtr> cameras_;
  std::vector<ID3D10ShaderResourceView*> textures_;
  Meshes meshes_;
};

#endif
