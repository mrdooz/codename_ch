#include "stdafx.h"
#include "EffectWrapper.hpp"

EffectWrapper::EffectWrapper(const ID3D10DevicePtr& device)
  : device_(device)
{
}

EffectWrapper::~EffectWrapper()
{
  g_system->unwatch_file(_filename.c_str(), fastdelegate::bind(&EffectWrapper::file_changed, this));

}

bool EffectWrapper::load(const char* filename)
{
  if (!load_inner(filename)) {
    return false;
  }

  _filename = filename;
  g_system->watch_file(filename, fastdelegate::bind(&EffectWrapper::file_changed, this));
  return true;
}

void EffectWrapper::file_changed()
{
  if (!load_inner(_filename.c_str())) {
    LOG_WARNING_LN("Error reloading shader: %s", _filename.c_str());
  }
}

bool EffectWrapper::load_inner(const char* filename)
{
  ID3D10Effect* effect = load_effect(filename, device_);
  if (effect == NULL) {
    return false;
  }

  techniques_.clear();
  scalar_variables_.clear();
  vector_variables_.clear();
  matrix_variables_.clear();
  constant_buffers_.clear();
  shader_resource_variables_.clear();


  effect_.Attach(effect);
  D3D10_EFFECT_DESC desc;
  effect_->GetDesc(&desc);
  collect_variables(desc);
  collect_techniques(desc);
  collect_cbuffers(desc);
  return true;
}

bool EffectWrapper::get_pass_desc(D3D10_PASS_DESC& desc, const std::string& technique_name, const uint32_t pass)
{
  Techniques::iterator it = techniques_.find(technique_name);
  if (it != techniques_.end()) {
    return SUCCEEDED(it->second->GetPassByIndex(0)->GetDesc(&desc));
  }
  LOG_WARNING_LN("Unknown technique: %s", technique_name.c_str());
  return false;
}

bool EffectWrapper::get_technique(ID3D10EffectTechnique*& technique, const std::string& name)
{
  Techniques::iterator it = techniques_.find(name);
  if (it != techniques_.end()) {
    technique = it->second;
    return true;
  }
  technique = NULL;
  return false;
}

bool EffectWrapper::set_technique(const std::string& technique_name)
{
  Techniques::iterator it = techniques_.find(technique_name);
  if (it != techniques_.end()) {
    return SUCCEEDED(it->second->GetPassByIndex(0)->Apply(0));
  }
  LOG_WARNING_LN_ONESHOT("[%s] Unknown technique: %s", __FUNCTION__, technique_name.c_str());
  return false;
}

bool EffectWrapper::set_variable(const std::string& name, const float value)
{
  ScalarVariables::iterator it = scalar_variables_.find(name);
  if (it != scalar_variables_.end()) {
    if (SUCCEEDED(it->second->SetFloat(value))) {
      return true;
    }
  }
  return false;
}

bool EffectWrapper::set_variable(const std::string& name, const D3DXVECTOR2& value)
{
  return set_variable(name, D3DXVECTOR4(value.x, value.y, 0, 0));
}

bool EffectWrapper::set_variable(const std::string& name, const D3DXVECTOR3& value)
{
  return set_variable(name, D3DXVECTOR4(value.x, value.y, value.z, 0));
}

bool EffectWrapper::set_variable(const std::string& name, const D3DXVECTOR4& value)
{
  VectorVariables::iterator it = vector_variables_.find(name);
  if (it != vector_variables_.end()) {
    if (SUCCEEDED(it->second->SetFloatVector((float*)&value[0]))) {
      return true;
    }
  }
  return false;
}

bool EffectWrapper::set_variable(const std::string& name, const D3DXMATRIX& value)
{
  MatrixVariables::iterator it = matrix_variables_.find(name);
  if (it != matrix_variables_.end()) {
    if (SUCCEEDED(it->second->SetMatrix((float*)&value[0]))) {
      return true;
    }
  }
  return false;
}

bool EffectWrapper::set_resource(const std::string& name, ID3D10ShaderResourceView* resource)
{
  ShaderResourceVariables::iterator it = shader_resource_variables_.find(name);
  if (it != shader_resource_variables_.end()) {
    if (SUCCEEDED(it->second->SetResource(resource))) {
      return true;
    }
  }
  return false;
}

bool EffectWrapper::get_resource(ID3D10EffectShaderResourceVariable*& resource, const std::string& name)
{
  ShaderResourceVariables::iterator it = shader_resource_variables_.find(name);
  if (it != shader_resource_variables_.end()) {
    resource = it->second;
    return true;
  }
  return false;
}

bool EffectWrapper::get_variable(ID3D10EffectScalarVariable*& var, const std::string& name)
{
  ScalarVariables::iterator it = scalar_variables_.find(name);
  if (it != scalar_variables_.end()) {
    var = it->second;
    return true;
  }
  return false;
}

bool EffectWrapper::get_variable(ID3D10EffectVectorVariable*& var, const std::string& name)
{
  VectorVariables::iterator it = vector_variables_.find(name);
  if (it != vector_variables_.end()) {
    var = it->second;
    return true;
  }
  return false;
}

bool EffectWrapper::get_variable(ID3D10EffectMatrixVariable*& var, const std::string& name)
{
  MatrixVariables::iterator it = matrix_variables_.find(name);
  if (it != matrix_variables_.end()) {
    var = it->second;
    return true;
  }
  return false;
}

bool EffectWrapper::get_cbuffer(ID3D10Buffer*& buffer, const std::string& name)
{
  ConstantBuffers::iterator it = constant_buffers_.find(name);
  if (it != constant_buffers_.end()) {
    buffer = it->second;
    return true;
  }
  return false;
}

void EffectWrapper::collect_variables(const D3D10_EFFECT_DESC& desc)
{
  for (uint32_t i = 0; i < desc.GlobalVariables; ++i) {

    ID3D10EffectVariable* var = effect_->GetVariableByIndex(i);
    ID3D10EffectType* type = var->GetType();
    D3D10_EFFECT_TYPE_DESC type_desc;
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    type->GetDesc(&type_desc);
    var->GetDesc(&var_desc);
    const char* type_name = type_desc.TypeName;
    const char* var_name = var_desc.Name;

    ID3D10EffectVariable* effect_variable = effect_->GetVariableByName(var_name);
    if (effect_variable == NULL) {
      LOG_WARNING_LN("Error getting variable: %s", var_name);
      continue;
    }

    if (strcmp(type_name, "float") == 0) {
      if (ID3D10EffectScalarVariable* typed_variable = effect_variable->AsScalar()) {
        scalar_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "float2") == 0) {
      if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
        vector_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "float3") == 0) {
      if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
        vector_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "float4") == 0) {
      if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
        vector_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "int2") == 0) {
      if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
        vector_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "float4x4") == 0) {
      if (ID3D10EffectMatrixVariable* typed_variable = effect_variable->AsMatrix()) {
        matrix_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else if (strcmp(type_name, "Texture2D") == 0) {
      if (ID3D10EffectShaderResourceVariable* typed_variable = effect_variable->AsShaderResource()) {
        shader_resource_variables_.insert(std::make_pair(var_name, typed_variable));
      }
    } else {
      LOG_WARNING_LN("Unknown variable: %s, type: %s", var_desc.Name, type_name);
    }
  }
}

void EffectWrapper::collect_techniques(const D3D10_EFFECT_DESC& desc)
{
  for (uint32_t i = 0; i < desc.Techniques; ++i ) {
    ID3D10EffectTechnique* technique = effect_->GetTechniqueByIndex(i);
    D3D10_TECHNIQUE_DESC technique_desc;
    technique->GetDesc(&technique_desc);
    techniques_.insert(std::make_pair(technique_desc.Name, technique));
  }
}

void EffectWrapper::collect_cbuffers(const D3D10_EFFECT_DESC& desc)
{
  for (uint32_t i = 0; i < desc.ConstantBuffers; ++i) {
    ID3D10EffectConstantBuffer* cbuffer = effect_->GetConstantBufferByIndex(i);
    D3D10_EFFECT_VARIABLE_DESC var_desc;
    cbuffer->GetDesc(&var_desc);
    ID3D10Buffer* buf = NULL;
    cbuffer->GetConstantBuffer(&buf);
    constant_buffers_.insert(std::make_pair(var_desc.Name, buf));
  }
}
