union __mingw_ldbl_type_t {
	long double x;
	struct {
		unsigned int low;
		unsigned int high;
		int sign_exponent : 16;
		int res1 : 16;
		int res0 : 32;
	} lh;
};
typedef union __mingw_ldbl_type_t __mingw_ldbl_type_t;
