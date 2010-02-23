#ifndef SHADOW_RENDERER_HPP
#define SHADOW_RENDERER_HPP

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

class Mesh;
class Camera;
class Geometry;
class Light;
class EffectMgr;
class EffectWrapper;
class RenderableTexture2D;
class DebugRenderer;
class PostProcess;
class TextureCache;
struct Input;

typedef std::vector<MeshSPtr> Meshes;

typedef boost::shared_ptr<SystemInterface> SystemSPtr;
typedef boost::shared_ptr<EffectManager> EffectManagerSPtr;
typedef boost::scoped_ptr<DebugRenderer> DebugRendererSPtr;

class ShadowRenderer : public Renderable
{
  friend class ReduxLoader;
public:
  ShadowRenderer(const SystemSPtr& system, const EffectManagerSPtr& effect_manager);
  ~ShadowRenderer();
  virtual bool init();
  virtual bool close();
  virtual void  render(const int32_t time_in_ms);

  void  load_scene(const std::string& filename);
private:

  void  create_shadow_textures();

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

  void  create_input_layout();
  void  material_changed(const MeshName& mesh_name, const MaterialName& material_name);

  void  render_meshes(ID3D10BlendState* blend_state, ID3D10DepthStencilState* depth_state, MeshesByMaterial& meshes);

  void  add_material_connection(const std::string& mesh_name, const std::string& material_name);

  void  render_inner(const D3DXVECTOR3& pos, const D3DXMATRIX& view, const D3DXMATRIX& proj, const std::string& technique);
  void  render_debug(const D3DXMATRIX& view, const D3DXMATRIX& proj);

  Scene scene_;
  uint32_t current_camera_;
  bool free_fly_camera_enabled_;

  LoadedEffects loaded_effects_;
  LoadedMaterials loaded_materials_;
  LoadedMaterialConnections loaded_material_connections_;

  MaterialsByEffect effect_list_;

  AnimationManager* animation_manager_;

  stdext::hash_map<EffectName, Handle> effect_handles_;

  ID3D10BlendState* opaque_blend_state_;
  ID3D10DepthStencilState*  opaque_depth_stencil_state_;
  ID3D10BlendState* transparent_blend_state_;
  ID3D10DepthStencilState*  transparent_depth_stencil_state_;

  std::vector<EffectConnection*> effect_connections_;
  SystemSPtr system_;
  EffectManagerSPtr effect_manager_;
  DebugRendererSPtr debug_renderer_;

  // shadow mapping stuff
  EffectWrapper* shadow_effect_;
  RenderableTexture2D* shadow_map_;
  D3D10_VIEWPORT  shadow_viewport_;

  ID3D10Texture2DPtr  depth_stencil_;
  ID3D10DepthStencilViewPtr depth_stencil_view_;

  float light_fov_;
  D3DXVECTOR2 light_dist_atten_;
  bool sort_meshes_;
  bool cull_meshes_;
  bool debug_draw_;
  bool draw_meshes_;
  uint32_t visible_meshes_;

  // Setup for SAT generation
  void InitGenerateSAT(int RDSamplesPerPass = 4);

  // Generate a single standard summed area table via recursive doubling
  RenderableTexture2D* GenerateSATRecursiveDouble(RenderableTexture2D *Src, bool MaintainSrc = false);

  void AddFullTextures(int Num);

  PostProcess*                        m_PostProcess;       // Post processing helper
  TextureCache*                        m_FullTextures;      // Texture cache

  ID3D10EffectTechnique*              m_FPToINTTechnique;        // Technique for converting fp32 to int32
  ID3D10EffectShaderResourceVariable* m_EffectSATTexture;        // Effect interface to SAT texture
  RenderableTexture2D*                m_SAT;                     // Summed area table
  RenderableTexture2D*                m_MSAAResolvedShadowMap;   // Potentially need a texture to resolve into

  bool                                m_IntFormat;               // Int of Float texture
  bool                                m_DistributePrecision;     // Distribute float precision

  // Recursive doubling
  ID3D10EffectTechnique*              m_GenerateSATRDTechnique;  // Technique to use for RD SAT generation
  ID3D10EffectVectorVariable*         m_EffectSATPassOffset;     // Pass offset in the effect file
  ID3D10EffectScalarVariable*         var_min_filter_width_;
  float                               min_filter_width_;
  int                                 m_GenerateSATRDSamples;    // Recursive doubling samples/pass

  SERIALIZE( ShadowRenderer, 
    MEMBER(sort_meshes_) MEMBER(cull_meshes_) MEMBER(light_fov_) 
    MEMBER(light_dist_atten_) MEMBER(debug_draw_) MEMBER(draw_meshes_)
    MEMBER(visible_meshes_)
    MEMBER(min_filter_width_)
    );

};

#endif // #ifndef SHADOW_RENDERER_HPP
