// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
//#define _SECURE_SCL 0

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "targetver.h"

// C
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <fcntl.h>

// C++
#include <io.h>
#include <iostream>

#include <celsus/Logger.hpp>
#include <celsus/celsus.hpp>
#include <celsus/tinyjson.hpp>
#include <celsus/Timer.hpp>
#include <celsus/Profiler.hpp>
#include <celsus/StringIdTable.hpp>
#include <celsus/DXUtils.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/if.hpp> 
#include <boost/mpl/int.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/accumulate.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/test/unit_test.hpp>

#ifdef STANDALONE
#else // #ifdef STANDALONE
#include <boost/python.hpp>

extern "C"
{
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
}
#endif


#include <direct.h>

