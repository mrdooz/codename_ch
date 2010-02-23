#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

/* A simple C++ reflection mark-up library. The strength
of this library is that it is entirely static (all the
reflection information is built without using dynamic
memory allocation), and it is exremely compact, allowing
you to declare the members of a structure right where
that struct is declared, leading to minimal risk of
version mismatch.

This code is placed in the public domain by Jon Watte.
http://www.enchantedage.com/cpp-reflection
Version 2009-04-20

Hacked to support serialization and tweakbaring by Magnus Österlind.

*/

#include <windows.h>

#include <typeinfo>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <hash_map>
#include <list>
#include <boost/shared_ptr.hpp>
#include <AntTweakBar.h>

bool write_cstring(const char* str, FILE* file);
bool read_cstring(std::string& str, FILE* file);

struct TypeBase
{
  virtual bool marshal(FILE*, const void*) const = 0;
  virtual bool demarshal(FILE*, void*) const = 0;
  virtual char const *name() const = 0;
  virtual size_t size() const = 0;
  virtual TwType tweak_type() const = 0;
};

// custom marshaling is handled by template specialization
template<typename T> struct Marshaler
{
  static bool marshal(FILE* file, const void* obj)
  {
    return fwrite(obj, sizeof(T), 1, file) == 1;
  }

  static bool demarshal(FILE* file, void* obj)
  {
    return fread(obj, sizeof(T), 1, file) == 1;
  }
};

// marshaling for std::string
template<> struct Marshaler<std::string>
{
  static bool marshal(FILE* file, const void* obj)
  {
    return write_cstring(((std::string*)obj)->c_str(), file);
  }

  static bool demarshal(FILE* file, void* obj)
  {
    return read_cstring(*(std::string*)obj, file);
  }
};

template<typename T> struct TweakType;

template<typename T> struct Type : TypeBase
{
  static Type<T> instance;
  virtual bool marshal(FILE* file, const void* obj) const
  {
    return Marshaler<T>::marshal(file, obj);
  }

  virtual bool demarshal(FILE* file, void* obj) const
  {
    return Marshaler<T>::demarshal(file, obj);
  }

  virtual TwType tweak_type() const
  {
    return TweakType<T>()();
  }

  virtual char const *name() const { return typeid(T).name(); }
  virtual size_t size() const { return sizeof(T); }
};

// devious hack to create a pointer to a static object of any type
template <typename T> T& instance()
{
  static T t;
  return t;
}

template<typename T, typename Q>
TypeBase *get_type(Q T::* /*mem*/)
{
  return &instance<Type<Q> >();
}

struct member_t
{
  char const *name;
  TypeBase *type;
  size_t offset;
};

struct ReflectionBase
{
  virtual ~ReflectionBase() {}
  void ReflectionConstruct();
  virtual size_t size() const = 0;
  virtual char const *name() const = 0;
  virtual char const *type_name() const = 0;
  virtual size_t memberCount() const = 0;
  virtual member_t const *members() const = 0;

  uint32_t data_size() const;
  bool save(FILE* file, uint8_t* obj);
  bool load(FILE* file, uint8_t* obj);
};

#define MEMBER(x) \
{ #x, get_type(&T::x), (size_t)&((T*)0)->x },

#define SERIALIZE(_type, _mems) \
  public:\
  template<typename T> struct _info : ReflectionBase { \
    _info() { ReflectionConstruct(); } \
    /* overrides used by ReflectionBase */ \
    inline size_t size() const { return sizeof(T); } \
    inline char const *name() const { return #_type; } \
    inline char const *type_name() const { return typeid(_type).name(); } \
    inline size_t memberCount() const { size_t cnt; get_members(cnt); return cnt; } \
    inline member_t const *members() const { size_t cnt; return get_members(cnt); } \
    static inline member_t const *get_members(size_t &cnt) { \
      static member_t members[] = { _mems }; \
      cnt = sizeof(members) / sizeof(members[0]); \
      return members; \
    } \
    static inline _info<T> &info() { \
      return instance<_info<T> >(); \
    } \
  }; \
  inline static member_t const * members() { return _info<_type>::info().members(); } \
  inline static _info<_type> &info() { return _info<_type>::info(); }

class Serializer
{
private:
  Serializer();
public:
  static Serializer& instance();
  bool save(const char* filename);
  bool load(const char* filename);
  void add_prototype(ReflectionBase* ptr);
  bool remove_serializable(ReflectionBase* ptr);

  template<class T> void remove_instance(T* obj, const char* instance_name = NULL)
  {
    const std::string valid_instance_name(create_instance_name(instance_name));
    const char* type_name = typeid(T).name();
    uint8_t* ptr = (uint8_t*)obj;
    if (ReflectionBase* base = find_prototype_by_type(type_name)) {

      for (uint32_t i = 0; i < base->memberCount(); ++i) {
        //const char* var_name = base->members()[i].name;
        //TwAddVarRW(bar_, var_name, TW_TYPE_FLOAT, ptr + base->members()[i].offset, "");
      }

      Instances& instances = objects_[type_name];
      for (Instances::iterator i = instances.begin(), e = instances.end(); i != e; ++i) {
        if (i->ptr == ptr) {
          instances.erase(i);
          return;
        }
      }
    }
    LOG_WARNING_LN("Error removing instance: %s", valid_instance_name.c_str());
  }

  template<class T> void add_instance(T* obj, const char* instance_name = NULL)
  {
    Serializer::instance().add_prototype(&obj->info());
    ++instance_counter_;
    const std::string valid_instance_name(create_instance_name(instance_name));
    const char* type_name = typeid(T).name();
    uint8_t* ptr = (uint8_t*)obj;

    if (ReflectionBase* base = find_prototype_by_type(type_name)) {
      for (uint32_t i = 0; i < base->memberCount(); ++i) {
        const member_t& cur_var = base->members()[i];
        const char* var_name = base->members()[i].name;
        TwAddVarRW(bar_, var_name,  cur_var.type->tweak_type(), ptr + base->members()[i].offset, "");
      }
      objects_[type_name].push_back(Instance(valid_instance_name, ptr));
    } else {
      LOG_WARNING_LN("Error removing instance: %s", valid_instance_name.c_str());
    }
  }

  TwBar* bar_;

  TwType vec2_type() const { return vec2_type_; }
  TwType vec3_type() const { return vec3_type_; }
  TwType vec4_type() const { return vec4_type_; }
private:
  void create_types();
  std::string create_instance_name(const char* instance_name);
  ReflectionBase* find_prototype_by_type(const char* name);
  uint8_t* find_instance_by_name(const char* name);

  struct Instance
  {
    Instance(const std::string& name, uint8_t* ptr) : name(name), ptr(ptr) {}
    std::string name;
    uint8_t* ptr;
  };
  typedef std::list<ReflectionBase*> Prototypes;
  typedef std::vector<Instance> Instances;
  typedef std::string PrototypeName;
  typedef stdext::hash_map< PrototypeName, Instances > Objects;

  Objects objects_;
  Prototypes prototypes_;
  uint32_t instance_counter_;

  TwType vec2_type_;
  TwType vec3_type_;
  TwType vec4_type_;
};

template<> struct TweakType<bool> { TwType operator()() { return TW_TYPE_BOOLCPP; } };
template<> struct TweakType<int32_t> { TwType operator()() { return TW_TYPE_INT32; } };
template<> struct TweakType<uint32_t> { TwType operator()() { return TW_TYPE_UINT32; } };
template<> struct TweakType<float> { TwType operator()() { return TW_TYPE_FLOAT; } };
template<> struct TweakType<D3DXVECTOR2> { TwType operator()() { return Serializer::instance().vec2_type(); } };
template<> struct TweakType<D3DXVECTOR3> { TwType operator()() { return Serializer::instance().vec3_type(); } };
template<> struct TweakType<D3DXVECTOR4> { TwType operator()() { return Serializer::instance().vec4_type(); } };

#endif
