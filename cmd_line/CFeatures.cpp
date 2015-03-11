#include "da_protocol.h"

#define dstr(x) #x
#define X(a,b) dstr(FL_ ## a),

const char *feature_to_str_arr[] = {
	feature_list
};

#undef dstr
#undef X
