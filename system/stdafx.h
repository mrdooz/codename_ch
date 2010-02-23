#pragma once
//#define _SECURE_SCL 0

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 4201)
#include <MMSystem.h>
#pragma warning(pop)

#include <stdint.h>
#ifdef STANDALONE
#else
#pragma warning(push)
#pragma warning(disable: 4100 4121 4244 4512)
#include <boost/python.hpp>
#pragma warning(pop)
#endif

#include "FastDelegate.hpp"
#include "FastDelegateBind.hpp"

#pragma warning(push)
#pragma warning(disable: 4180 4512)
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
#pragma warning(pop)

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#define FOREACH BOOST_FOREACH

#include <D3D10.h>
#include <D3DX10.h>

#include <atlbase.h>

#include <string>
#include <vector>
#include <map>

// celsus
#include <celsus/celsus.hpp>
#include <celsus/logger.hpp>
#include <celsus/ErrorHandling.hpp>
#include <celsus/DXUtils.hpp>
#include <celsus/UnicodeUtils.hpp>