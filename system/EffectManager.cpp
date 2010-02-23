#include "stdafx.h"
#include "EffectManager.hpp"

Material::Material()
  : transparency_(0)
{
}

void Material::set_variables()
{
  for (uint32_t i = 0; i < scalar_variables_.size(); ++i) {
    if (FAILED(scalar_variables_[i].first->SetFloat(scalar_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < vec2_variables_.size(); ++i) {
    if (FAILED(vec2_variables_[i].first->SetFloatVector(vec2_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < vec3_variables_.size(); ++i) {
    if (FAILED(vec3_variables_[i].first->SetFloatVector(vec3_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < vec4_variables_.size(); ++i) {
    if (FAILED(vec4_variables_[i].first->SetFloatVector(vec4_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < color_variables_.size(); ++i) {
    if (FAILED(color_variables_[i].first->SetFloatVector(color_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < matrix_variables_.size(); ++i) {
    if (FAILED(matrix_variables_[i].first->SetMatrix(matrix_variables_[i].second))) {
      return;
    }
  }

  for (uint32_t i = 0; i < shader_resource_variables_.size(); ++i) {
    // TODO
    /*
    if (FAILED(shader_resource_variables_[i].first->SetResource(shader_resource_variables_[i].second))) {
    return;
    }
    */
  }
}


EffectManager::EffectManager(HandleManager* handle_manager)
  : handle_manager_(handle_manager)
{
}

EffectManager::~EffectManager()
{
  materials_.clear();
}


ID3D10Effect* EffectManager::effect_from_handle(const Handle& handle)
{
  Effect* effect = NULL;
  
  if (!handle_manager_->get(handle, effect)) {
    return NULL;
  }
  return effect->effect;
}

void EffectManager::add_material(Material* material)
{
  materials_.push_back(boost::shared_ptr<Material>(material));
}

MaterialSPtr EffectManager::find_material_by_name(const StringId& name)
{
  for (uint32_t i = 0; i < materials_.size(); ++i ) {
    if (materials_[i]->name() == name) {
      return materials_[i];
    }
  }
  return MaterialSPtr();
}

Material* load_material_fron_json(const json::json_element& element, ID3D10Effect* effect)
{
  std::string material_name(element.find_object("name").get<std::string>());
  Material* material = new Material();
  material->name_.set(material_name.c_str());
  material->effect_ = effect;

  json::json_element values = element.find_object("values");
  for (uint32_t i = 0; i < values.length(); ++i) {
    std::string name(values[i].find_object("name").get<std::string>());
    std::string type(values[i].find_object("type").get<std::string>());
    json::json_element value(values[i].find_object("value"));

    if (name == "transparency") {
      material->transparency_ = (float)value.get<double>();
      continue;;
    } 

    if (type == "float") {
      if (ID3D10EffectVariable* effect_variable = effect->GetVariableByName(name.c_str())) {
        if (ID3D10EffectScalarVariable* typed_variable = effect_variable->AsScalar()) {
          material->scalar_variables_.push_back( std::make_pair(typed_variable, 
            (float)value.get<double>()));
        }
      }
    } else if (type == "color") {
      if (ID3D10EffectVariable* effect_variable = effect->GetVariableByName(name.c_str())) {
        if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
          material->color_variables_.push_back( std::make_pair(typed_variable, 
            D3DXCOLOR( 
            (float)value[0].get<double>(), 
            (float)value[1].get<double>(), 
            (float)value[2].get<double>(), 
            (float)value[3].get<double>())));
        }
      }
    } else if (type == "vector2") {
      if (ID3D10EffectVariable* effect_variable = effect->GetVariableByName(name.c_str())) {
        if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
          material->color_variables_.push_back( std::make_pair(typed_variable, 
            D3DXVECTOR2( 
            (float)value[0].get<double>(), 
            (float)value[1].get<double>())));
        }
      }
    } else if (type == "vector3") {
      if (ID3D10EffectVariable* effect_variable = effect->GetVariableByName(name.c_str())) {
        if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
          material->color_variables_.push_back( std::make_pair(typed_variable, 
            D3DXVECTOR3( 
            (float)value[0].get<double>(), 
            (float)value[1].get<double>(), 
            (float)value[2].get<double>())));
        }
      }
    } else if (type == "vector4") {
      if (ID3D10EffectVariable* effect_variable = effect->GetVariableByName(name.c_str())) {
        if (ID3D10EffectVectorVariable* typed_variable = effect_variable->AsVector()) {
          material->color_variables_.push_back( std::make_pair(typed_variable, 
            D3DXVECTOR4( 
            (float)value[0].get<double>(), 
            (float)value[1].get<double>(), 
            (float)value[2].get<double>(), 
            (float)value[3].get<double>())));
        }
      }
    }
  }

  return material;
}
