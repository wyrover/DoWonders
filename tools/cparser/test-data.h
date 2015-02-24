
#include <pshpack2.h>
__attribute__((__aligned__(1)))
union __mingw_ldbl_type_t {
	long double x;
#include <pshpack4.h>
	struct {
		unsigned int low;
		unsigned int high;
		int sign_exponent : 16;
		int res1 : 16;
		int res0 : 32;
	} lh;
#include <poppack.h>
};
#include <poppack.h>
typedef union __mingw_ldbl_type_t __mingw_ldbl_type_t;
