#if !defined(__cplusplus) || (__cplusplus < 199711L)
    #error Modern C++ compiler required! You lose.
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#undef __STRICT_ANSI__
#define __NO_INLINE__   // https://github.com/g-truc/glm/issues/300

#include <cstdlib>          // standard library
#include <cstdio>           // standard I/O
#include <cstring>          // C string
#include <cassert>          // for assert

#include <vector>           // for std::vector
#include <string>           // for std::string
#include <map>              // for std::map
#include <unordered_map>    // for std::unordered_map
#include <set>              // for std::set
#include <deque>            // for std::deque
#include <algorithm>        // for std::sort, std::unique

#include <memory>
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using std::make_shared;

#include "Location.h"       // CR_Location
#include "Main.h"
#include "TypeSystem.h"
#include "AST.h"     // for cparser::Node, cparser::TokenInfo

#include <windows.h>
#include <tchar.h>
#include "mzc2mini.h"
