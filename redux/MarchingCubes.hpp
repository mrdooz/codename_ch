#ifndef MARCHING_CUBES_HPP
#define MARCHING_CUBES_HPP

#include "Light.hpp"
#include "Camera.hpp"
#include "ReduxTypes.hpp"
#include "Camera.hpp"
#include "AnimationNode.hpp"
#include "Scene.hpp"
#include "ReduxTypes.hpp"
#include "AnimationManager.hpp"
#include "../system/EffectManager.hpp"
#include "../system/Renderable.hpp"
#include "../system/Serializer.hpp"
#include <celsus/D3DTypes.hpp>
#include "EffectWrapper.hpp"

class Mesh;
class Camera;
class Geometry;
class Light;
class EffectMgr;
struct Input;

typedef std::vector<MeshSPtr> Meshes;

typedef boost::shared_ptr<SystemInterface> SystemSPtr;
typedef boost::shared_ptr<EffectManager> EffectManagerSPtr;

class DebugRenderer;

struct Grid;

class MarchingCubes : public Renderable
{
  friend class ReduxLoader;
public:
  MarchingCubes(const SystemSPtr& system, const EffectManagerSPtr& effect_manager);
  ~MarchingCubes();
  virtual bool init();
  virtual bool close();
  virtual void  render(const int32_t time_in_ms, const int32_t delta);

  void  load_scene(const std::string& filename);
private:
  void render_mesh(const int32_t time_in_ms);

  typedef stdext::hash_map<MaterialName, Meshes> MeshesByMaterial;
  struct MeshLists
  {
    MeshesByMaterial opaque_materials_;
    MeshesByMaterial transparent_materials_;
  };

  typedef stdext::hash_map<EffectName, MeshLists> MaterialsByEffect;

  struct MaterialConnection
  {
    ~MaterialConnection() 
    {
      material_.reset();
      meshes_.clear();
    }
    MaterialSPtr  material_;
    Meshes  meshes_;
  };

  typedef std::vector<MaterialConnection*> MaterialConnections;
  struct EffectConnection
  {
    ~EffectConnection()
    {
      container_delete(opaque_materials_);
      container_delete(transparent_materials_);
    }
    LPD3D10EFFECT effect_;
    MaterialConnections opaque_materials_;
    MaterialConnections transparent_materials_;
  };

  typedef std::vector< EffectName > LoadedEffects;
  typedef std::vector< std::string > LoadedMaterials;
  typedef std::vector< std::string > LoadedMaterialConnections;

  void  process_input_callback(const Input& input);

  void  create_input_layout();

  Scene scene_;
  uint32_t current_camera_;
  bool free_fly_camera_enabled_;

  LoadedEffects loaded_effects_;
  LoadedMaterials loaded_materials_;
  LoadedMaterialConnections loaded_material_connections_;

  MaterialsByEffect effect_list_;

  AnimationManager* animation_manager_;

  stdext::hash_map<EffectName, Handle> effects_;

  typedef std::vector<EffectConnection*> EffectConnections;
  EffectConnections effect_connections_;
  SystemSPtr system_;
  EffectManagerSPtr effect_manager_;
  EffectWrapper effect_;

  boost::scoped_ptr<DebugRenderer> dynamic_mgr_;
  boost::scoped_ptr<Grid> _grid;

  uint32_t splits_;
  SERIALIZE(MarchingCubes, MEMBER(splits_));
};

#endif
