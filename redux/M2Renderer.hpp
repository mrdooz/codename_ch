#ifndef M2_RENDERER_HP
#define M2_RENDERER_HP

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

class Mesh;
class Camera;
class Geometry;
class Light;
class EffectMgr;
struct Input;
class EventArgs;
class EffectWrapper;

typedef std::vector<MeshSPtr> Meshes;

typedef boost::shared_ptr<SystemInterface> SystemSPtr;
typedef boost::shared_ptr<EffectManager> EffectManagerSPtr;


class M2Renderer : public Renderable
{
  friend class ReduxLoader;
public:
  M2Renderer(const SystemSPtr& system, const EffectManagerSPtr& effect_manager);
  ~M2Renderer();
  virtual bool init();
  virtual bool close();
  virtual void  render(const int32_t time_in_ms, const int32_t delta);

  void  load_scene(const std::string& filename);
private:

  typedef std::map<MaterialName, Meshes> MeshesByMaterial;
  struct MeshLists
  {
    MeshesByMaterial opaque_materials_;
    MeshesByMaterial transparent_materials_;
  };

  typedef std::map<EffectName, MeshLists> MaterialsByEffect;

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

  struct EffectConnection
  {
    ~EffectConnection()
    {
      container_delete(opaque_materials_);
      container_delete(transparent_materials_);
    }
    EffectWrapper* effect_;
    std::vector<MaterialConnection*> opaque_materials_;
    std::vector<MaterialConnection*> transparent_materials_;
  };

  typedef std::vector< EffectName > LoadedEffects;
  typedef std::vector< std::string > LoadedMaterials;
  typedef std::vector< std::string > LoadedMaterialConnections;

  void  process_input_callback(const Input& input);

  void file_changed(const EventArgs* args);

  Scene scene_;
  uint32_t current_camera_;
  bool free_fly_camera_enabled_;

  LoadedEffects loaded_effects_;
  LoadedMaterials loaded_materials_;
  LoadedMaterialConnections loaded_material_connections_;

  MaterialsByEffect effect_list_;

  AnimationManager* animation_manager_;

  std::map<EffectName, Handle> effects_;

  ID3D10BlendState* opaque_blend_state_;
  ID3D10DepthStencilState*  opaque_depth_stencil_state_;
  ID3D10BlendState* transparent_blend_state_;
  ID3D10DepthStencilState*  transparent_depth_stencil_state_;

  std::vector<EffectConnection*> effect_connections_;
  SystemSPtr system_;
  EffectManagerSPtr effect_manager_;

  EffectWrapper*  effect_;
};

#endif // #ifndef M2_RENDERER_HP
