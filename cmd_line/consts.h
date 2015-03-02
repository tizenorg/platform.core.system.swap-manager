
#ifndef _CONSTS_H_
#define _CONSTS_H_


    #define MSG_CANT_FIND_PROBE "Can't find probe"
    #define MSG_CANT_CHANGE_APP_PATH "Can't change path to application because there are probes for current path to application binary"
    #define MSG_CANT_SET_APP_PROBE_PATH_IS_NOTSET "Can't set app probe because path to application is not set"
    #define MSG_CANT_SET_LIB_PROBE_PATH_IS_NOTSET "Can't set lib probe because path is not set"
    #define MSG_CANT_FIND_FUNCTION "Can't find function"
    #define MSG_CANT_IDENTIFY_FB_PROBE_BY_NAME "Can't identify FB probe by name"
    #define MSG_CANT_IDENTIFY_PROBE_BY_NAME "Can't identify probe by name"
    #define MSG_IT_IS_NOT_SUPPORTED "It's not supported"
    #define MSG_CANT_EXECUTE_COMMAND_IN_CURRENT_MODE "Can't execute command in current mode"
    #define MSG_CANT_START_DATA_COLLECTION "Error while start data collection"
    #define MSG_PATH_TOO_LONG "Path is too long"
    #define MSG_START_SHELL "Start shell"
    #define MSG_EXIT_SHELL "Stop shell"
    #define TARGET_SHELL_PATH "/bin/sh"
    #define CMD_PATH_TO_SHELL TARGET_SHELL_PATH

enum probe_level_type_t {
	PROBE_TYPE_LEVEL_KERNEL		= 0x1,
	PROBE_TYPE_LEVEL_APP		= 0x2,
	PROBE_TYPE_LEVEL_LIB		= 0x4,
};


   const int PROBE_TYPE__DEX		= 0x10;

    const int DATA_TYPE__SDEC32		= 0x1;
    const int DATA_TYPE__UHEX32		= 0x2;
    const int DATA_TYPE__FLOAT		= 0x4;
    const int DATA_TYPE__CHAR		= 0x8;
    const int DATA_TYPE__STR		= 0x10;

    const int VAR__TARGET_APP_FNAME	= 0x1;
    const int VAR__HOST_APP_FNAME	= 0x2;
    const int VAR__TARGET_IP		= 0x4;
    const int VAR__TARGET_PORT		= 0x8;
    const int VAR__BUFFER_SIZE		= 0x10;
    const int VAR__BUFFER_CONT		= 0x20;
    const int VAR__ENGINE_TYPE		= 0x40;
    const int VAR__HOST_LIB_PATH	= 0x80;
    const int VAR__TARGET_LIB_PATH	= 0x100;
    const int VAR__EVENT_MASK		= 0x200;
    const int VAR__LIB_NAME		= 0x400;
    const int VAR__PID			= 0x8000;
    const int VAR__TARGET_DEX_FNAME	= 0x10000;
    const int VAR__HOST_DEX_FNAME	= 0x2000;
    const int VAR__POCO			= 0x4000;
    const int VAR__JAVA_INST		= 0x8000;
    const int VAR__LIB_DL_NAME		= 0x10000;
    const int VAR__SMAP_FNAME		= 0x20000;
    const int VAR__DB_FNAME		= 0x40000;
    const int VAR__MAPPINGS		= 0x80000;

//    const int APP_TYPE_NATIVE		= 0x1;
//    const int APP_TYPE_RUNNING		= 0x2;
//    const int APP_TYPE_COMMON		= 0x3;
//    const int APP_TYPE_WEB		= 0x4;

#endif /* _PRINT_FUNCS_H_ */
