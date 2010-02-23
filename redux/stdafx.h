#pragma once
//#define _SECURE_SCL 0

#include <vld.h>
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 4201)
#include <MMSystem.h>
#pragma warning(pop)

#include <stdint.h>

#include <D3D10.h>
#include <D3DX10.h>

#include <atlbase.h>

#include <string>
#include <vector>
#include <map>
#include <hash_map>

#include "../system/FastDelegate.hpp"
#include "../system/FastDelegateBind.hpp"

// celsus
#include <celsus/DXUtils.hpp>
#include <celsus/celsus.hpp>
#include <celsus/logger.hpp>
#include <celsus/ErrorHandling.hpp>
#include <celsus/MemoryMappedFile.hpp>
#include <celsus/tinyjson.hpp>
#include <celsus/StringId.hpp>
#include <celsus/Profiler.hpp>

// boost
#pragma warning( push )
#pragma warning( disable: 4996 )
#include <boost/array.hpp>
#pragma warning( pop )

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#pragma warning( push )
#pragma warning( disable: 4244 4245 )
#include <boost/crc.hpp>
#pragma warning( pop )
#include <boost/type_traits.hpp>
#include <boost/call_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#define FOREACH BOOST_FOREACH
#include <boost/format.hpp>
#pragma warning( push )
#pragma warning( disable: 4180 )
#include <boost/assign/list_of.hpp>
#pragma warning( pop )
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/algorithm/string/split.hpp>
#pragma warning( push )
#pragma warning( disable: 4512 )
#include <boost/algorithm/string/classification.hpp>
#pragma warning( pop )

#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/function_pointer.hpp>
#include <boost/function_types/member_function_pointer.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
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
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/type_traits/is_void.hpp> 
#include <boost/type_traits/remove_reference.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/intrusive/list.hpp>

// stl
#include <map>
#include <vector>
#include <stack>
#include <string>
#include <set>
#include <functional>

#include <yajl/yajl_parse.h>

#include <AntTweakBar.h>

#include <celsus/D3D10Descriptions.hpp>
#include "../system/Input.hpp"
#include "../system/SystemInterface.hpp"
