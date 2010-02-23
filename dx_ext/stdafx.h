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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


#include <stdint.h>
#pragma warning(push)
#pragma warning(disable: 4100 4121 4244 4512)
#include <boost/python.hpp>
#pragma warning(pop)

#include <D3D10.h>
#include <D3DX10.h>
