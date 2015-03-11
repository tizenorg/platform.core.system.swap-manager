#include "da_protocol.h"

const char *feature_code_str_arr[] = {
#define dstr(x) #x
#define _x(a,b) dstr(FL_ ## a)
#define _x_del ,
feature_list
#undef dstr
#undef _x
#undef _x_del
};
