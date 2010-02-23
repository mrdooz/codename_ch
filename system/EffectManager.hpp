#ifndef EFFECT_MANAGER_HPP
#define EFFECT_MANAGER_HPP

#include "HandleManager.hpp"
#include <celsus/StringId.hpp>
#include <celsus/tinyjson.hpp>

struct  Material
{
  Material();
  void set_variables();
  const bool is_transparent() const { return transparency_ > 0.0f; }
  const StringId& name() const { return name_; }

  typedef std::pair<ID3D10EffectScalarVariable*, float> ScalarVariable;
  typedef std::pair<ID3D10EffectVectorVariable*, D3DXVECTOR2> Vec2Variable;
  typedef std::pair<ID3D10EffectVectorVariable*, D3DXVECTOR3> Vec3Variable;
  typedef std::pair<ID3D10EffectVectorVariable*, D3DXVECTOR4> Vec4Variable;
  typedef std::pair<ID3D10EffectVectorVariable*, D3DXCOLOR> ColorVariable;
  typedef std::pair<ID3D10EffectMatrixVariable*, D3DXCOLOR> MatrixVariable;
  typedef std::pair<ID3D10EffectShaderResourceVariable*, D3DXCOLOR> ShaderResourceVariable;

  typedef std::vector<ScalarVariable> ScalarVariables;
  typedef std::vector<Vec2Variable> Vec2Variables;
  typedef std::vector<Vec3Variable> Vec3Variables;
  typedef std::vector<Vec4Variable> Vec4Variables;
  typedef std::vector<ColorVariable> ColorVariables;
  typedef std::vector<MatrixVariable> MatrixVariables;
  typedef std::vector<ShaderResourceVariable> ShaderResourceVariables;


  float transparency_;

  ScalarVariables scalar_variables_;
  Vec2Variables vec2_variables_;
  Vec3Variables vec3_variables_;
  Vec4Variables vec4_variables_;
  ColorVariables color_variables_;
  MatrixVariables matrix_variables_;
  ShaderResourceVariables shader_resource_variables_;

  StringId name_;

  ID3D10Effect* effect_;
};

Material* load_material_fron_json(const json::json_element& element, ID3D10Effect* effect);

typedef boost::shared_ptr<Material> MaterialSPtr;

struct Technique 
{
  Technique(const std::string& name, const D3D10_TECHNIQUE_DESC& desc, ID3D10EffectTechnique* technique) 
    : name(name), desc(desc), technique(technique) {}

  std::string name;
  D3D10_TECHNIQUE_DESC desc;
  ID3D10EffectTechnique* technique;
};



class EffectManager
{
public:

  EffectManager(HandleManager* handle_manager);
  ~EffectManager();
  ID3D10Effect* effect_from_handle(const Handle& handle);

  void add_material(Material* material);

  MaterialSPtr find_material_by_name(const StringId& name);

private:

  typedef std::vector<MaterialSPtr> Materials;
  Materials materials_;
  HandleManager* handle_manager_;
};

#endif // #ifndef EFFECT_MANAGER_HPP
