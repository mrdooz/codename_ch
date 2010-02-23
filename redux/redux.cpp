// redux.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DefaultRenderer.hpp"

#ifdef STANDALONE
#else
using namespace boost::python;
BOOST_PYTHON_MODULE(redux_ext)
{
  class_<DefaultRenderer>("DefaultRenderer", init<SystemInterface&, object, object>())
    .def("load_scene", &DefaultRenderer::load_scene)
    ;
}
#endif
