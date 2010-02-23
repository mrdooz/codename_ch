#include "stdafx.h"

using namespace std;
using namespace boost::python;

namespace
{
  string str_vector2(const D3DXVECTOR2& v)
  {
    ostringstream os;
    os << v.x << ", " << v.y;
    return os.str();
  }

  string str_vector3(const D3DXVECTOR3& v)
  {
    ostringstream os;
    os << v.x << ", " << v.y << ", " << v.z;
    return os.str();
  }

  string str_vector4(const D3DXVECTOR4& v)
  {
    ostringstream os;
    os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
    return os.str();
  }

  string str_color(const D3DXCOLOR& v)
  {
    ostringstream os;
    os << "[rgba] " << v.r << ", " << v.g << ", " << v.b << ", " << v.a;
    return os.str();
  }
}

struct vec2_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const D3DXVECTOR2& v)
  {
    return make_tuple(v.x, v.y);
  }
};

struct vec3_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const D3DXVECTOR3& v)
  {
    return make_tuple(v.x, v.y, v.z);
  }
};

struct vec4_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const D3DXVECTOR4& v)
  {
    return make_tuple(v.x, v.y, v.z, v.w);
  }
};

struct color_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const D3DXCOLOR& v)
  {
    return make_tuple(v.r, v.g, v.b, v.a);
  }
};

struct matrix_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const D3DXMATRIX& v)
  {
    return make_tuple(
      v._11, v._12, v._13, v._14,
      v._21, v._22, v._23, v._24,
      v._31, v._32, v._33, v._34,
      v._41, v._42, v._43, v._44);
  }
};


BOOST_PYTHON_MODULE(dx_ext)
{
  class_<D3DXVECTOR2>("Vector2", init<float, float>())
    .def_readwrite("x", &D3DXVECTOR2::x)
    .def_readwrite("y", &D3DXVECTOR2::y)
    .def("__str__", &str_vector2)
    .def_pickle(vec2_pickle_suite())
    ;

  class_<D3DXVECTOR3>("Vector3", init<float, float, float>())
    .def_readwrite("x", &D3DXVECTOR3::x)
    .def_readwrite("y", &D3DXVECTOR3::y)
    .def_readwrite("z", &D3DXVECTOR3::z)
    .def("__str__", &str_vector3)
    .def_pickle(vec3_pickle_suite())
    ;

  class_<D3DXVECTOR4>("Vector4", init<float, float, float, float>())
    .def_readwrite("x", &D3DXVECTOR4::x)
    .def_readwrite("y", &D3DXVECTOR4::y)
    .def_readwrite("z", &D3DXVECTOR4::z)
    .def_readwrite("w", &D3DXVECTOR4::w)
    .def("__str__", &str_vector4)
    .def_pickle(vec4_pickle_suite())
    ;

  class_<D3DXCOLOR>("Color", init<float, float, float, float>())
    .def_readwrite("r", &D3DXCOLOR::r)
    .def_readwrite("g", &D3DXCOLOR::g)
    .def_readwrite("b", &D3DXCOLOR::b)
    .def_readwrite("a", &D3DXCOLOR::a)
    .def("__str__", &str_color)
    .def_pickle(color_pickle_suite())
    ;

  class_<D3DXMATRIX>("Matrix", init<
    float, float, float, float,
    float, float, float, float,
    float, float, float, float,
    float, float, float, float>())
    .def_pickle(matrix_pickle_suite())
    ;
}
