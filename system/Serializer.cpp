
#include "stdafx.h"
#include "Serializer.hpp"

void ReflectionBase::ReflectionConstruct()
{
  members();
  memberCount();
  name();
  size();
}

uint32_t ReflectionBase::data_size() const
{
  uint32_t res = 0;
  for (size_t i = 0; i < memberCount(); ++i) {
    res += members()[i].type->size();
  }
  return res;
}

bool ReflectionBase::save(FILE* file, uint8_t* obj)
{
  for (size_t i = 0; i < memberCount(); ++i) {
    const member_t& m = members()[i];
    if (!m.type->marshal(file, obj + m.offset)) {
      return false;
    }
  }
  return true;
}

bool ReflectionBase::load(FILE* file, uint8_t* obj)
{
  for (size_t i = 0; i < memberCount(); ++i)
  {
    const member_t& m = members()[i];
    if (!m.type->demarshal(file, obj + m.offset)) {
      return false;
    }
  }
  return true;
}

Serializer::Serializer()
  : instance_counter_(0)
{
  create_types();
}

void Serializer::create_types()
{
  TwStructMember vec2_members[] = { 
    { "X", TW_TYPE_FLOAT, offsetof(D3DXVECTOR2, x), "Step=0.01" },
    { "Y", TW_TYPE_FLOAT, offsetof(D3DXVECTOR2, y), "Step=0.01" },
  };

  TwStructMember vec3_members[] = { 
    { "X", TW_TYPE_FLOAT, offsetof(D3DXVECTOR3, x), "Step=0.01" },
    { "Y", TW_TYPE_FLOAT, offsetof(D3DXVECTOR3, y), "Step=0.01" },
    { "Z", TW_TYPE_FLOAT, offsetof(D3DXVECTOR3, z), "Step=0.01" },
  };

  TwStructMember vec4_members[] = { 
    { "X", TW_TYPE_FLOAT, offsetof(D3DXVECTOR4, x), "Step=0.01" },
    { "Y", TW_TYPE_FLOAT, offsetof(D3DXVECTOR4, y), "Step=0.01" },
    { "Z", TW_TYPE_FLOAT, offsetof(D3DXVECTOR4, z), "Step=0.01" },
    { "W", TW_TYPE_FLOAT, offsetof(D3DXVECTOR4, w), "Step=0.01" },
  };

  vec2_type_ = TwDefineStruct("Vec2", vec2_members, 2, sizeof(D3DXVECTOR2), NULL, NULL);
  vec3_type_ = TwDefineStruct("Vec3", vec3_members, 3, sizeof(D3DXVECTOR3), NULL, NULL);
  vec4_type_ = TwDefineStruct("Vec4", vec4_members, 4, sizeof(D3DXVECTOR4), NULL, NULL);
}
Serializer& Serializer::instance()
{
  static Serializer obj;
  return obj;
}

bool write_cstring(const char* str, FILE* file)
{
  const uint32_t len = strlen(str);
  if (fwrite(&len, sizeof(len), 1, file) != 1) {
    return false;
  }
  if (fwrite(str, len, 1, file) != 1) {
    return false;
  }
  return true;
}

bool read_cstring(std::string& str, FILE* file)
{
  uint32_t len = 0;
  if (fread(&len, sizeof(len), 1, file) != 1) {
    return false;
  }
  str.resize(len);
  if (fread((void*)str.data(), len, 1, file) != 1) {
    return false;
  }
  return true;
}

bool Serializer::save(const char* filename)
{
#pragma warning(suppress: 4996)
  FILE* file = fopen(filename, "wb");
  if (file == NULL) {
    return false;
  }
  boost::shared_ptr<FILE> scoped_file(file, &fclose);

  const uint32_t prototype_count = objects_.size();
  fwrite(&prototype_count, sizeof(prototype_count), 1, file);

  for (Objects::const_iterator i = objects_.begin(), e = objects_.end(); i != e; ++i) {
    if (ReflectionBase* prot = find_prototype_by_type(i->first.c_str())) {
      const Instances& instances = i->second;
      // save prototype name and data length
      write_cstring(prot->type_name(), file);
      const uint32_t data_size = prot->data_size();
      fwrite(&data_size, sizeof(uint32_t), 1, file);
      const uint32_t instance_count = instances.size();
      fwrite(&instance_count, sizeof(instance_count), 1, file);
      for (Instances::const_iterator i_ptr = instances.begin(), e_ptr = instances.end(); i_ptr != e_ptr; ++i_ptr) {
        write_cstring(i_ptr->name.c_str(), file);
        if (!prot->save(file, i_ptr->ptr)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool Serializer::load(const char* filename)
{
#pragma warning(suppress: 4996)
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    return false;
  }
  boost::shared_ptr<FILE> scoped_file(file, &fclose);

  uint32_t prototype_count;
  fread(&prototype_count, sizeof(prototype_count), 1, file);
  for (uint32_t i = 0; i < prototype_count; ++i) {
    std::string proto_name;
    read_cstring(proto_name, file);
    uint32_t data_size;
    fread(&data_size, sizeof(data_size), 1, file);

    if (ReflectionBase* prot = find_prototype_by_type(proto_name.c_str())) {
      const uint32_t real_data_size = prot->data_size();
      if (data_size != real_data_size) {
        LOG_WARNING_LN("Invalid data size");
        return false;
      }

      uint32_t instance_count;
      fread(&instance_count, sizeof(instance_count), 1, file);
      for (uint32_t j = 0; j < instance_count; ++j) {
        std::string instance_name;
        read_cstring(instance_name, file);
        if (uint8_t* ptr = find_instance_by_name(instance_name.c_str())) {
          if (!prot->load(file, ptr)) {
            LOG_WARNING_LN("Error loading prototype");
            return false;
          }
        }
      }
    }

  }
  return true;

}

ReflectionBase* Serializer::find_prototype_by_type(const char* name)
{
  for (Prototypes::iterator i = prototypes_.begin(), e = prototypes_.end(); i != e; ++i) {
    if ( strcmp((*i)->type_name(), name) == 0) {
      return *i;
    }
  }
  return NULL;
}

uint8_t* Serializer::find_instance_by_name(const char* name)
{
  for (Objects::const_iterator i = objects_.begin(), e = objects_.end(); i != e; ++i) {
    const Instances& instances = i->second;
    for(Instances::const_iterator i_inst = instances.begin(), e_inst = instances.end(); i_inst != e_inst; ++i_inst) {
      if (0 == strcmp(i_inst->name.c_str(), name)) {
        return i_inst->ptr;
      }
    }
  }
  return NULL;
}

void Serializer::add_prototype(ReflectionBase* ptr)
{
  if (NULL == find_prototype_by_type(ptr->type_name())) {
    prototypes_.push_back(ptr);
  }
}

bool Serializer::remove_serializable(ReflectionBase* ptr)
{
  for (Prototypes::iterator i = prototypes_.begin(), e = prototypes_.end(); i != e; ++i) {
    if (*i == ptr) {
      prototypes_.erase(i);
      return true;
    }
  }
  return false;
}

std::string Serializer::create_instance_name(const char* instance_name)
{
  std::string valid_instance_name;
  if (instance_name == NULL) {
    char buf[16];
#pragma warning(suppress: 4996)
    sprintf(buf, "%d", instance_counter_);
    valid_instance_name = buf;
  } else {
    valid_instance_name = instance_name;
  }
  return valid_instance_name;
}
