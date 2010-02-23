#ifndef COUNTDOWN_HPP
#define COUNTDOWN_HPP

#include <Celsus/DXUtils.hpp>
#include <boost/mpl/vector.hpp>

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
#include "EffectWrapper.hpp"
#include "IndexBuffer.hpp"
#include "VertexBuffer.hpp"

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

class Countdown : public Renderable
{
  friend class ReduxLoader;
public:
  Countdown(const SystemSPtr& system, const EffectManagerSPtr& effect_manager);
  ~Countdown();
  virtual bool init();
  virtual bool close();
  virtual void  render(const int32_t time_in_ms);

  void  load_scene(const std::string& filename);
private:
  void render_mesh(const int32_t time_in_ms);

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

  Scene scene_;
  uint32_t current_camera_;
  bool free_fly_camera_enabled_;

  LoadedEffects loaded_effects_;
  LoadedMaterials loaded_materials_;
  LoadedMaterialConnections loaded_material_connections_;

  MaterialsByEffect effect_list_;

  AnimationManager* animation_manager_;

  std::map<EffectName, Handle> effects_;

  typedef std::vector<EffectConnection*> EffectConnections;
  EffectConnections effect_connections_;
  SystemSPtr system_;
  EffectManagerSPtr effect_manager_;
  EffectWrapper effect_;

  boost::scoped_ptr<DebugRenderer> dynamic_mgr_;

  VertexBuffer< boost::mpl::vector<D3DXVECTOR3, D3DXNORMAL> >* vb_;
  IndexBuffer<uint32_t>* ib_;

  uint32_t splits_;
  SERIALIZE(Countdown, MEMBER(splits_));
};

#endif
