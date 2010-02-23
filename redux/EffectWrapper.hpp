#ifndef EFFECT_WRAPPER_HPP
#define EFFECT_WRAPPER_HPP

#include <celsus/D3DTypes.hpp>

#include "../system/SystemInterface.hpp"

class EffectWrapper
{
public:
  EffectWrapper(const ID3D10DevicePtr& device);
  ~EffectWrapper();
  bool load(const char* filename);

  bool  set_technique(const std::string& technique_name);
  bool  set_variable(const std::string& name, const float value);
  bool  set_variable(const std::string& name, const D3DXVECTOR2& value);
  bool  set_variable(const std::string& name, const D3DXVECTOR3& value);
  bool  set_variable(const std::string& name, const D3DXVECTOR4& value);
  bool  set_variable(const std::string& name, const D3DXMATRIX& value);

  bool  get_technique(ID3D10EffectTechnique*& technique, const std::string& name);
  bool  get_variable(ID3D10EffectScalarVariable*& var, const std::string& name);
  bool  get_variable(ID3D10EffectVectorVariable*& var, const std::string& name);
  bool  get_variable(ID3D10EffectMatrixVariable*& var, const std::string& name);
  bool  get_cbuffer(ID3D10Buffer*& buffer, const std::string& name);
  bool  get_resource(ID3D10EffectShaderResourceVariable*& resource, const std::string& name);
  bool  set_resource(const std::string& name, ID3D10ShaderResourceView* resource);
  bool  get_pass_desc(D3D10_PASS_DESC& desc, const std::string& technique_name, const uint32_t pass = 0);

  ID3D10EffectPtr effect();
private:

  void  file_changed();
  bool  load_inner(const char* filename);

  void collect_variables(const D3D10_EFFECT_DESC& desc);
  void collect_techniques(const D3D10_EFFECT_DESC& desc);
  void collect_cbuffers(const D3D10_EFFECT_DESC& desc);

  typedef stdext::hash_map< std::string, ID3D10EffectTechnique* > Techniques;
  typedef stdext::hash_map< std::string, ID3D10EffectScalarVariable*> ScalarVariables;
  typedef stdext::hash_map< std::string, ID3D10EffectVectorVariable*> VectorVariables;
  typedef stdext::hash_map< std::string, ID3D10EffectMatrixVariable*> MatrixVariables;
  typedef stdext::hash_map< std::string, ID3D10EffectShaderResourceVariable*> ShaderResourceVariables;

  typedef stdext::hash_map< std::string, ID3D10Buffer* > ConstantBuffers;

  std::string _filename;
  ID3D10DevicePtr device_;
  ID3D10EffectPtr effect_;
  Techniques techniques_;

  ScalarVariables scalar_variables_;
  VectorVariables vector_variables_;
  MatrixVariables matrix_variables_;
  ConstantBuffers constant_buffers_;
  ShaderResourceVariables shader_resource_variables_;
};

inline ID3D10EffectPtr EffectWrapper::effect()
{
  return effect_;
}

#endif
