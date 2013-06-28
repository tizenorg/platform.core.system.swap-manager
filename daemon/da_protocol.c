

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/sysinfo.h>

#include "da_protocol.h"
#include "daemon.h"
#include "sys_stat.h"
#include "elf.h"


#define SYSTEM_INFO_DEBUG
#define parse_deb_on


#define BUF_FILENAME "/tmp/daemon_events"
int buf_fd = 0;

int open_buf(void)
{
	buf_fd = creat(BUF_FILENAME, 0644);
	if (buf_fd == -1) {
		LOGE("Cannot open buffer: %s\n", strerror(errno));
		return 1;
	}
	LOGI("buffer opened: %s, %d\n", BUF_FILENAME, buf_fd);

	return 0;
}

void close_buf(void)
{
	close(buf_fd);
}

int write_to_buf(struct msg_data_t *msg)
{
	if (write(buf_fd, msg, MSG_DATA_HDR_LEN + msg->len) == -1) {
		LOGE("write to buf: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

void inline free_msg_data(struct msg_data_t *msg)
{
	free(msg);
}

void inline free_msg_payload(struct msg_t *msg)
{
	free(msg->payload);
}

void free_sys_info(struct system_info_t *sys_info)
{
	if (sys_info->cpu_frequency)
		free(sys_info->cpu_frequency);
	if (sys_info->cpu_load)
		free(sys_info->cpu_load);
	if (sys_info->thread_load)
		free(sys_info->thread_load);
	if (sys_info->process_load)
		free(sys_info->process_load);
	memset(sys_info, 0, sizeof(*sys_info));
}

/*
struct msg_t msg;
struct app_info_t app_info;
struct probe_t probe;
struct us_inst_t us_inst;
struct app_inst_t app_inst;
struct prof_session_t prof_session;
*/

static struct prof_session_t prof_session;

static void print_app_info( struct app_info_t *app_info);
static void print_conf(struct conf_t * conf);
static void print_us_func_inst(struct us_func_inst_t * func_inst, int count,char * tab);
static void print_us_lib_inst(struct us_lib_inst_t * lib_inst,int num,char * tab);
static void print_user_space_app_inst(struct app_inst_t * app_inst, int num, char * tab);
static void print_user_space_inst(struct user_space_inst_t * user_space_inst);
static void print_log_interval(log_interval_t log_interval);
static void print_probe(struct probe_t *probe);
static void print_us_inst(struct us_inst_t *us_inst);
static void print_prof_session(struct prof_session_t *prof_session);

//DEBUG FUNCTIONS
#define dstr(x) #x

#define check_and_return(par,check) if ( par == check ) {return dstr(check);}
#define check_2(a1,a2) check_and_return(ID,a1) else check_and_return(ID,a2)
#define check_4(a1,a2,a3,a4) check_2(a1,a2) else check_2(a3,a4)
#define check_8(a1,a2,a3,a4,a5,a6,a7,a8) check_4(a1,a2,a3,a4) else check_4(a5,a6,a7,a8)

#define check_all(a1, ...) check_and_return(ID,a1) //#else check_all(__VA_ARGS__)
char* msg_ID_str ( enum HostMessageT ID)
{
	check_8(
		NMSG_KEEP_ALIVE,
		NMSG_START,
		NMSG_STOP,
		NMSG_CONFIG,
		0,
		NMSG_BINARY_INFO,
		NMSG_GET_TARGET_INFO,
		NMSG_SWAP_INST_ADD
		)
	else
	check_8(
		NMSG_SWAP_INST_REMOVE,
		NMSG_KEEP_ALIVE_ACK,
		NMSG_START_ACK,
		NMSG_STOP_ACK,

		NMSG_CONFIG_ACK,
		NMSG_BINARY_INFO_ACK,
		NMSG_SWAP_INST_ACK,
		NMSG_GET_TARGET_INFO_ACK
		)
	else
	check_8(
		NMSG_SWAP_INST_ADD_ACK,
		NMSG_SWAP_INST_REMOVE_ACK,
		NMSG_PROCESS_INFO,
		NMSG_TERMINATE,

		NMSG_ERROR,
		NMSG_SAMPLE,
		NMSG_SYSTEM,
		NMSG_IMAGE
		)
	else
	check_8(
		NMSG_RECORD,
		NMSG_FUNCTION_ENTRY,
		NMSG_FUNCTION_EXIT,
		NMSG_CONTEXT_SWITCH_ENTRY,

		NMSG_CONTEXT_SWITCH_EXIT,
		NMSG_PROBE,
		NMSG_PROBE_MEMORY,
		NMSG_PROBE_UICONTROL
		)
	else
	check_8(
		NMSG_PROBE_UIEVENT,
		NMSG_PROBE_RESOURCE,
		NMSG_PROBE_LIFECYCLE,
		NMSG_PROBE_SCREENSHOT,

		NMSG_PROBE_SCENE,
		NMSG_PROBE_THREAD,
		NMSG_PROBE_CUSTOM,
		NMSG_PROBE_SYNC
	) else
	return "HZ";
/*
 *
	if	(
		( (ID >= NMSG_KEEP_ALIVE) &&  ( ID <= NMSG_GET_TARGET_INFO) ) 
		||
		( (ID >= NMSG_KEEP_ALIVE_ACK) &&  ( ID <= NMSG_GET_TARGET_INFO_ACK) )
		)
	{
		if (ID & NMSG_ACK_FLAG){
			ID &= ~ NMSG_ACK_FLAG;
			ID+=9;
		} 
	}
	else
	{
		ID = 0;
	}

	return MSG_to_str_arr[ID];
	*/
}


static char *msgErrStr(enum ErrorCode err)
{
	switch (err) {
	case ERR_NO:
		return "success";
	case ERR_LOCKFILE_CREATE_FAILED:
		return "lock file create failed";
	case ERR_ALREADY_RUNNING:
		return "already running";
	case ERR_INITIALIZE_SYSTEM_INFO_FAILED:
		return "initialize system info failed";
	case ERR_HOST_SERVER_SOCKET_CREATE_FAILED:
		return "host server socket create failed";
	case ERR_TARGET_SERVER_SOCKET_CREATE_FAILED:
		return "target server socket create failed";
	case ERR_SIGNAL_MASK_SETTING_FAILED: //TODO del (old parametr)
		return "ERR SIGNAL MASK SETTING FAILED";
	case ERR_WRONG_MESSAGE_FORMAT:
		return "wrong message format";
	case ERR_WRONG_MESSAGE_TYPE:
		return "wrong message type";
	case ERR_WRONG_MESSAGE_DATA:
		return "wrong message data";
	case ERR_CANNOT_START_PROFILING:
		return "cannot start profiling";
	case ERR_SERV_SOCK_CREATE:
		return "server socket creation failed (written in /tmp/da.port file)";
	case ERR_SERV_SOCK_BIND:
		return "server socket bind failed (written in /tmp/da.port file)";
	case ERR_SERV_SOCK_LISTEN:
		return "server socket listen failed (written in /tmp/da.port file)";
	case ERR_UNKNOWN:
	default:
		return "unknown error";
	}
	return "unknown error";
}


#define print_feature(f,in,to,delim) if (f & in)\
								{\
									sprintf(to, dstr(f) delim );\
									to+=strlen( dstr(f) delim );\
								}
#define print_feature_a(f) print_feature(f,feature,to,", ")
void feature_code_str(uint32_t feature, char * to)
{
	print_feature_a(FL_CPU);
	print_feature_a(FL_MEMORY);
	print_feature_a(FL_FUNCTION_PROFILING);
	print_feature_a(FL_MEMORY_ALLCATION_PROBING);
	print_feature_a(FL_FILE_API_PROBING);
	print_feature_a(FL_THREAD_API_PROBING);
	print_feature_a(FL_OSP_UI_API_PROBING);
	print_feature_a(FL_SCREENSHOT);
	print_feature_a(FL_USER_EVENT);
	print_feature_a(FL_RECORDING);
	print_feature_a(FL_SYSTCALL_FILE);
	print_feature_a(FL_SYSTCALL_IPC);
	print_feature_a(FL_SYSTCALL_PROCESS);
	print_feature_a(FL_SYSTCALL_SIGNAL);
	print_feature_a(FL_SYSTCALL_NETWORK);
	print_feature_a(FL_SYSTCALL_DESC);
	print_feature_a(FL_CONTEXT_SWITCH);
	print_feature_a(FL_NETWORK_API_PROBING);
	print_feature_a(FL_OPENGL_API_PROBING);

}
#ifdef parse_deb_on
#define parse_deb(...) do{\
							LOGI("%s->\t",__FUNCTION__);\
							LOGI(__VA_ARGS__);\
							}while(0)
#else
#define parse_deb(...) do{}while(0)
#endif /*parse_on*/

//PARSE FUNCTIONS
static int receive_msg_header(struct msg_t *msg)
{
	// TODO: cycle until m_id and m_len
	return 0;
}

static int receive_msg_payload(struct msg_t *msg)
{
	msg->payload = (char *)malloc(msg->len);
	if (!msg->payload) {
		// TODO: report error
		return 1;
	}
	// TODO: cycle until
	return 0;
}

static char * parse_string(char *buf, char **str)
{
	int len = strlen(buf);
	*str = strdup(buf);
	parse_deb("<%s>\n",*str);
	return buf + (len + 1);
}

static char *parse_chars(char *buf, uint32_t count, char **str)
{
	*str = (char *) malloc( (count+1) * (sizeof(char) ));
	memcpy(*str, buf, count);
	(*str)[count] = 0;
	parse_deb("<%s>\n",*str);
	return buf + (count);
}

static char *parse_int32(char *buf, uint32_t *val)
{
	*val = *(uint32_t *)buf;

	parse_deb("<%d><0x%08X>\n",*val,*val);
	return buf + sizeof(uint32_t);
}

static char *parse_int64(char *buf, uint64_t *val)
{
	*val = *(uint64_t *)buf;

	parse_deb("<%d><0x%016X>\n",*val,*val);
	return buf + sizeof(uint64_t);
}

static char *parse_app_info(char *buf,
			       struct app_info_t *app_info)
{
	char *p = buf;

	p = parse_int32(p, &app_info->app_type);
	if (!p) {
		LOGE("app type parsing error\n");
		return 0;
	}

	p = parse_string(p, &app_info->app_id);
	if (!p) {
		LOGE("app id parsing error\n");
		return 0;
	}
	p = parse_string(p, &app_info->exe_path);
	if (!p) {
		LOGE("app info parsing error\n");
		return 0;
	}

//	print_app_info(app_info);

	return p;
}

static char * parse_conf(char *buf, struct conf_t *conf)
{
	char *p = buf;

	p = parse_int64( p, &conf->use_features);
 	if (!p) {
		LOGE("use features parsing error\n");
		return 0;
	}

	p = parse_int32( p, &conf->system_trace_period);
	if (!p) {
		LOGE("system trace period parsing error\n");
		return 0;
	}

	p = parse_int32( p, &conf->data_message_period);
	if (!p) {
		LOGE("data message period parsing error\n");
		return 0;
	}
	//print_conf(conf);
	return buf + sizeof(*conf);
}


static char *parse_log_interval(char *buf, log_interval_t *log_interval)
{
	*log_interval = *(log_interval_t *)buf;

	return buf + sizeof(*log_interval);
}

static char *parse_probe(char *buf, struct probe_t *probe)
{
	char *p = buf;

	p = parse_int64(p, &probe->addr);
	if (!p) {
		LOGE("probe addr parsing error\n");
		return 0;
	}

	p = parse_int32(p, &probe->arg_num);
	if (!p) {
		LOGE("arg num parsing error\n");
		return 0;
	}

	probe->arg_fmt = (char *)malloc(probe->arg_num);
	if (!probe->arg_fmt) {
		LOGE("args alloc error\n");
		return 0;
	}

	memcpy(probe->arg_fmt, p, probe->arg_num);
	p += probe->arg_num;

	return p;
}

static char *parse_us_inst(char *buf, struct us_inst_t *us_inst)
{
	char *p = buf;
	int i = 0;

	p = parse_string(p, &us_inst->path);
	if (!p) {
		LOGE("app path parsing error\n");
		return 0;
	}
	p = parse_int32(p, &us_inst->probe_num);
	if (!p) {
		LOGE("probe num parsing error\n");
		return 0;
	}

	us_inst->probes = (struct probe_t *)malloc(us_inst->probe_num *
						 sizeof(*us_inst->probes));

	if (!us_inst->probes) {
		LOGE("probes alloc error\n");
		return 0;
	}
	for (i = 0; i < us_inst->probe_num; i++) {
		p = parse_probe(p, &us_inst->probes[i]);
		if (!p) {
			LOGE("probe parsing error\n");
			return 0;
		}
	}

	return p;
}

static char *parse_us_inst_func( char * buf, struct us_func_inst_t * dest)
{
	char *p = buf;

	p = parse_int64( p, &(dest->func_addr));
	if (!p) {
		LOGE("func addr parsing error\n");
		return 0;
	}

	p = parse_int32( p, &(dest->args_num));
	if (!p) {
		LOGE("args num parsing error\n");
		return 0;
	}

	p = parse_chars( p, dest->args_num, &(dest->args));
	if (!p) {
		LOGE("args format parsing error\n");
		return 0;
	}
	return p;

}

static char * parse_func_inst_list(char *buf,
			    uint32_t *num,
			    struct us_func_inst_t ** us_func_inst_list)
{
	uint32_t i = 0;
	char * p = buf;
	p = parse_int32( p, num);
	if (!p) {
		LOGE("func num parsing error\n");
		return 0;
	}
	//parse user space function list

	LOGI("us_func_inst_list size = %d * %d\n",(*num) , (int)sizeof(**us_func_inst_list) );
	*us_func_inst_list = 
		(struct us_func_inst_t *) 
		malloc( (*num) * sizeof(**us_func_inst_list) );
	if (!*us_func_inst_list){
		LOGE("func alloc error\n");
		return 0;
	};

	for ( i = 0; i < *num; i++){
		p = parse_us_inst_func( p, &((* us_func_inst_list)[i]));
	}
	return p;
}

static char *parse_us_inst_lib( char * buf, struct us_lib_inst_t * dest)
{
	char *p = buf;

	p = parse_string( p, &(dest)->bin_path);
	if (!p) {
		LOGE("bin path parsing error\n");
		return 0;
	}

	p = parse_func_inst_list( p, &dest->func_num, &dest->us_func_inst_list);
	if (!p) {
		LOGE("funcs parsing error\n");
		return 0;
	}
	return p;

}

static char * parse_lib_inst_list(char *buf,
			    uint32_t *num,
			    struct us_lib_inst_t ** us_lib_inst_list)
{
	uint32_t i = 0;
	char * p = buf;
	p = parse_int32( p, num);
	if (!p) {
		LOGE("lib num parsing error\n");
		return 0;
	}

	//parse user space lib list
	LOGI("lib_list size = %d\n", (*num) * (int)sizeof(**us_lib_inst_list) );
	*us_lib_inst_list = 
		(struct us_lib_inst_t *) 
		malloc( (*num) * sizeof(**us_lib_inst_list) );
	if (!*us_lib_inst_list){
		LOGE("lib alloc error\n");
		return 0;
	};
	for ( i = 0; i < *num; i++){
		p = parse_us_inst_lib( p, &( (*us_lib_inst_list)[i] ) );
	}
	return p;
}

static char * parse_app_inst(char *buf,
			    struct app_inst_t *app_inst)
{
	char *p = buf;
	uint32_t num = 0;

	p = parse_int32( p, &app_inst->app_type);
	if (!p) {
		LOGE("app type parsing error\n");
		return 0;
	}
	p = parse_string( p, &app_inst->app_id);
	if (!p) {
		LOGE("app id parsing error\n");
		return 0;
	}
	p = parse_string( p, &app_inst->exec_path);
	if (!p) {
		LOGE("exec path parsing error\n");
		return 0;
	}
	p = parse_func_inst_list( p, &app_inst->func_num , &(app_inst->us_func_inst_list));
	if (!p) {
		LOGE("funcs parsing error\n");
		return 0;
	}
	//parse user space function list
	
	LOGI(">=%04X : %s, %s, %d\n", app_inst->app_type, app_inst->app_id, app_inst->exec_path , num);

	p = parse_lib_inst_list( p, &app_inst->lib_num , &app_inst->us_lib_inst_list);
	if (!p) {
		LOGE("libs parsing error\n");
		return 0;
	}

	return p;
}

char * parse_user_space_inst(char * buf, struct user_space_inst_t * user_space_inst) 
{
	char *p = buf;
	uint32_t num = 0 , i = 0;
	struct app_inst_t * list = 0;

	p = parse_int32 ( p, &num );
	if (!p) {
		LOGE("app num parsing error\n");
		return 0;
	}

	LOGI("%d * %d\n ",(int) sizeof(*(user_space_inst->app_inst_list)), num);
	//parse app list
	if ( num != 0 ) {
		list = (struct app_inst_t *) malloc ( 
					sizeof(*(user_space_inst->app_inst_list))
					*
					num
					);
		if ( !list ){
			LOGE("apps alloc error\n");
			return 0;
		};

		for ( i = 0; i < num; i++){
			p = parse_app_inst( p, &(list[i]) );
		};

		user_space_inst->app_num = num;
		user_space_inst->app_inst_list = list;
	}
	return p;
}

//REPLAY EVENTS PARSE
static char *parse_timeval(char *buf, struct timeval *tv)
{
	char *p = buf;

	parse_deb("time\n");

	// FIXME: is sec/usec order correct?
	p = parse_int32(p, (uint32_t *)&tv->tv_usec);
	if (!p) {
		LOGE("usec parsing error\n");
		return 0;
	}

	p = parse_int32(p, (uint32_t *)&tv->tv_sec);
	if (!p) {
		LOGE("sec parsing error\n");
		return 0;
	}

	return p;
}

static char *parse_replay_event(char *buf,
				    struct replay_event_t *re)
{
	char *p = buf;

	p = parse_timeval(p, &re->ev.time);
	if (!p) {
		LOGE("time parsing error\n");
		return 0;
	}

	p = parse_int32(p, &re->id);
	if (!p) {
		LOGE("id parsing error\n");
		return 0;
	}

	p = parse_int32(p, (uint32_t *)&re->ev.type);
	if (!p) {
		LOGE("type parsing error\n");
		return 0;
	}

	p = parse_int32(p, (uint32_t *)&re->ev.code);
	if (!p) {
		LOGE("code parsing error\n");
		return 0;
	}

	p = parse_int32(p, (uint32_t *)&re->ev.value);
	if (!p) {
		LOGE("value parsing error\n");
		return 0;
	}

	return p;
}

static char *parse_replay_event_seq(char *buf,
				    struct replay_event_seq_t *res)
{
	char *p = buf;
	int i = 0;
	parse_deb("REPLAY\n");
	parse_deb("enable\n");
	p = parse_int32(p, &res->enabled);
	if (!p) {
		LOGE("enabled parsing error\n");
		return 0;
	}

	parse_deb("time main\n");
	p = parse_timeval(p, &res->tv);
	if (!p) {
		LOGE("time parsing error\n");
		return 0;
	}

	parse_deb("count\n");
	p = parse_int32(p, &res->event_num);
	if (!p) {
		LOGE("event num parsing error\n");
		return 0;
	}

	res->events = (struct replay_event_t *)malloc(res->event_num *
						      sizeof(*res->events));
	if (!res->events) {
		LOGE("events alloc error\n");
		return 0;
	}

	for (i = 0; i < res->event_num; i++) {
		parse_deb("sub_rep\n");
		p = parse_replay_event(p, &res->events[i]);
		if (!p) {
			LOGE("event parsing error\n");
			goto free_events;
		}
	}

	goto end;

free_events:
	free(res->events);
	res->event_num = 0;

end:
	return p;
}

//*REPLAY EVENT PARSE

static int parse_prof_session(char *msg_payload,
			      struct prof_session_t *prof_session)
{
	char *p = msg_payload;

	p = parse_app_info(p, &prof_session->app_info);
	if (!p) {
		LOGE("app info parsing error\n");
		return 0;
	}
	p = parse_conf(p, &prof_session->conf);
	if (!p) {
		LOGE("conf parsing error\n");
		return 0;
	}

	p = parse_user_space_inst(p, &prof_session->user_space_inst);
	if (!p) {
		LOGE("user space inst parsing error\n");
		return 0;
	}

	p = parse_replay_event_seq(p, &prof_session->replay_event_seq);
	if (!p) {
		LOGE("replay parsing error\n");
		return 0;
	}

/*	p = parse_log_interval(p, &prof_session->log_interval);
	if (!p) {
		LOGE("app type parsing error\n");
		return 1;
	}
	p = parse_app_inst(p, &prof_session->app_inst);
	if (!p) {
		LOGE("app type parsing error\n");
		return 1;
	}
*/
	print_prof_session(prof_session);
	return 1;
}

int get_sys_mem_size(uint32_t *sys_mem_size){
	struct sysinfo info;
	sysinfo(&info);
	*sys_mem_size = info.totalram;
	return 0;
}
//int sysinfo(struct sysinfo *info);


//warning allocate memory
//need free after call
static char *parse_target_info(char *msg_payload,char ** buf, uint32_t *len)
{
	struct target_info_t *target_info;
	char *p = msg_payload;
	*len = sizeof(*target_info);

	*buf = malloc(*len);
	target_info = (struct target_info_t *) *buf;
	fill_target_info(target_info);

	return p;
}

static char *parse_msg_config(char * msg_payload, 
			      struct conf_t * conf)
{
	char *p = msg_payload;

	p = parse_conf(p, conf);
	if (!p) {
		LOGE("conf parsing error\n");
		return 0;
	}

	print_conf(conf);
	return p;
}

static char *parse_msg_binary_info(char * msg_payload, 
			      struct app_info_t *app_info)
{
	char *p = msg_payload;

	p = parse_app_info(p, app_info);
	if (!p) {
		LOGE("app info parsing error\n");
		return 0;
	}

	print_app_info(app_info);
	return p;
}

static void reset_msg(struct msg_t *msg)
{
	free(msg->payload);
}

static void reset_app_info(struct app_info_t *app_info)
{
	free(app_info->app_id);
	free(app_info->exe_path);
}

static void reset_conf(struct conf_t *conf)
{
	memset(conf, 0, sizeof(*conf));
}

static void reset_log_interval(log_interval_t *log_interval)
{
	memset(log_interval, 0, sizeof(*log_interval));
}

static void reset_probe(struct probe_t *probe)
{
	probe->addr = 0;
	probe->arg_num = 0;
	free(probe->arg_fmt);
}

static void reset_us_inst(struct us_inst_t *us_inst)
{
	int i = 0;

	us_inst->path[0] = '\0';
	for (i = 0; i < us_inst->probe_num; i++) {
		reset_probe(&us_inst->probes[i]);
	}
	free(us_inst->probes);
	us_inst->probe_num = 0;
}

static void reset_app_inst(struct app_inst_t *app_inst)
{
	/*
	int i = 0;

	reset_us_inst(&app_inst->app);

	for (i = 0; i < app_inst->lib_num; i++) {
		reset_us_inst(&app_inst->libs[i]);
	}
	free(app_inst->libs);
	app_inst->lib_num = 0;
	*/
}

static void reset_prof_session(struct prof_session_t *prof_session)
{
	reset_app_info(&prof_session->app_info);
	reset_conf(&prof_session->conf);
	//reset_log_interval(&prof_session->log_interval);
	//reset_app_inst(&prof_session->app_inst);
}

static struct msg_t *gen_binary_info_reply(struct app_info_t *app_info)
{
	uint32_t binary_type = get_binary_type(app_info->exe_path);
	char binary_path[] = "unknown path";
	struct msg_t *msg;
	char *p = NULL;

	if (binary_type == BINARY_TYPE_UNKNOWN) {
		LOGE("Binary is neither relocatable, nor executable\n");
		return NULL;
	}

	msg = malloc(sizeof(*msg));
	if (!msg) {
		LOGE("Cannot alloc bin info msg\n");
		return NULL;
	}

	msg->payload = malloc(sizeof(binary_type) + strlen(binary_path) + 1);
	if (!msg) {
		LOGE("Cannot alloc bin info msg payload\n");
		return NULL;
	}

	msg->id = NMSG_BINARY_INFO_ACK;
	p = msg->payload;

	pack_int(p, binary_type);
	pack_str(p, binary_path);

	msg->len = p - msg->payload;

	return msg;
}

// return 0 for normal case
// return negative value for error case
int parseHostMessage(struct msg_t* log, char* msg)
{
	int ret = 0;	// parsing success
	memcpy( log, msg, 8);//message ID, length
	log->payload = malloc( log->len);
	if ( log->payload != 0 )
	{
		memcpy( log->payload, msg+8, log->len);
	}
	else
	{
		LOGE("cannot malloc\n");
		ret = -1;
	}
	return ret;
}

static int send_reply(struct msg_t *msg)
{
	if (send(manager.host.control_socket,
		 msg, MSG_CMD_HDR_LEN, MSG_NOSIGNAL) == -1) {
		LOGE("Cannot send reply header: %s\n", strerror(errno));
		return 1;
	}

	if (send(manager.host.control_socket,
		 msg->payload, msg->len, MSG_NOSIGNAL) == -1) {
		LOGE("Cannot send reply payload: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

static int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size)
{
	if (manager.host.control_socket != -1)
	{
		struct msg_t *msg; 
		uint32_t err = err_code;
		int loglen = sizeof(*msg) - sizeof(msg->payload) +
					 sizeof(err) + //return ID
					 payload_size;
		char *logstr = malloc(loglen);
		msg = (struct msg_t *)logstr;
		char *p = &(msg->payload);

		//get ack message ID
		switch (resp) {
			case NMSG_KEEP_ALIVE:
				resp = NMSG_KEEP_ALIVE_ACK;
				break;
			case NMSG_START:
				resp = NMSG_START_ACK;
				break;
			case NMSG_STOP:
				resp = NMSG_STOP_ACK;
				break;
			case NMSG_CONFIG:
				resp = NMSG_CONFIG_ACK;
				break;
			case NMSG_BINARY_INFO:
				resp = NMSG_BINARY_INFO_ACK;
				break;
			case NMSG_GET_TARGET_INFO:
				resp = NMSG_GET_TARGET_INFO_ACK;
				break;
			case NMSG_SWAP_INST_ADD:
				resp = NMSG_SWAP_INST_ADD_ACK;
				break;
			case NMSG_SWAP_INST_REMOVE:
				resp = NMSG_SWAP_INST_REMOVE_ACK;
				break;
			default:
				//TODO report error
				free(logstr);
				return 1;
		}

		//set message id 
		msg->id = resp;
		//set payload lenth
		msg->len = payload_size + sizeof(err);
		//set return id
		*(uint32_t *)p = err; p+=sizeof(err);
		//copy payload data
		memcpy(p, payload, payload_size);

		LOGI("ACK (%s) payload=%d; size=%d\n",msg_ID_str(resp),payload,payload_size);
		printBuf(logstr, loglen);

		send(manager.host.control_socket, logstr, loglen, MSG_NOSIGNAL);
		free(logstr);
		return 0;
	}
	else
		return 1;
}

int hostMessageHandle(struct msg_t *msg)
{
	char * answer = 0;
	uint32_t answer_len = 0;
	uint32_t ID = msg->id;
	struct user_space_inst_t user_space_inst;
	struct app_info_t app_info;
	struct msg_t *msg_reply;

	LOGI("MY HANDLE %s (%X)\n",msg_ID_str(msg->id),msg->id);
	switch (ID) {
	case NMSG_KEEP_ALIVE:
		sendACKToHost(ID,ERR_NO,0,0);
		break;
	case NMSG_START:
		if (!parse_prof_session(msg->payload, &prof_session)) {
			LOGE("prof session parsing error\n");
			sendACKToHost(ID,ERR_WRONG_MESSAGE_FORMAT,0,0);
			return 1;
		}

		// prepare buffer to write
		if (open_buf() != 0) {
			LOGE("Cannot open buffer\n");
			return 1;
		}

		// TODO: launch translator thread

		// TODO: get app path
		/* get_executable(manager.appPath, execPath, PATH_MAX); // get exact app executable file name */
		/* LOGI("executable app path %s\n", manager.appPath); */

		// TODO: kill app
/* #ifdef RUN_APP_LOADER */
/* 		kill_app(manager.appPath); */
/* #else */
/* 		kill_app(execPath); */
/* #endif */

		// TODO: get install path (via pseudo readelf)

		// TODO: get build option (via pseudo readelf)

		// TODO: apply_prof_session()

		if (0) {
			sendACKCodeToHost(MSG_NOTOK, ERR_CANNOT_START_PROFILING);
			return -1;
		}

		// TODO: start_sampling_thread()
		if (samplingStart() < 0)
			return -1;

		// TODO: exec app

		// TODO: start app launch timer

		//start replay events
		replay_thread(&prof_session.replay_event_seq);

		// success
		sendACKToHost(ID,ERR_NO,0,0);
		break;
	case NMSG_STOP:
		sendACKToHost(ID,ERR_NO,0,0);
		// TODO: remove_prof_session()
		close_buf();
		reset_prof_session(&prof_session);
		break;
	case NMSG_CONFIG:
		if (!parse_msg_config(msg->payload, &prof_session.conf)) {
			LOGE("config parsing error\n");
			LOGE("parse error <%s>\n",msg_ID_str(msg->id));
			sendACKToHost(ID,ERR_WRONG_MESSAGE_FORMAT,0,0);
			return 0;
		}

		sendACKToHost(ID,ERR_NO,0,0);
		break;
	case NMSG_BINARY_INFO:
		if (!parse_msg_binary_info(msg->payload, &app_info)) {
			LOGE("binary info parsing error\n");
			sendACKToHost(ID, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return 0;
		}
		msg_reply = gen_binary_info_reply(&app_info);
		if (!msg_reply) {
			sendACKToHost(ID, ERR_UNKNOWN, 0, 0);
			return 0;
		}

		if (send_reply(msg_reply) != 0) {
			LOGE("Cannot send reply\n");
		}

		reset_app_info(&app_info);
		reset_msg(msg_reply);
		free(msg_reply);
		break;
	case NMSG_SWAP_INST_ADD:
		if (!parse_user_space_inst(msg->payload, &prof_session.user_space_inst)){
			LOGE("user space inst parsing error\n");
			return 0;
		}
		sendACKToHost(ID,ERR_NO,0,0);
		break;
	case NMSG_SWAP_INST_REMOVE:
		if (!parse_user_space_inst(msg->payload, &prof_session.user_space_inst)){
			LOGE("user space inst parsing error\n");
			return 0;
		}
		sendACKToHost(ID,ERR_NO,0,0);
		break;
	case NMSG_GET_TARGET_INFO:
		LOGI("GET_TARGET_INFO case\n");
		if (!parse_target_info(msg->payload, &answer, &answer_len)) {
			LOGE("target info parsing error\n");
			sendACKToHost(ID,ERR_WRONG_MESSAGE_FORMAT,
					answer,answer_len);
			return 1;
		}
		// TODO: apply_prof_session()
		sendACKToHost(ID,ERR_NO,answer,answer_len);
		break;

	default:
		LOGE("unknown message %d <0x%08X>\n",ID,ID);
	}

	//
	// TODO free memory in profsession_data after use
	//
	
	//free allocated memory
	if (!answer)
		free(answer);
	return 0;
}

static void dispose_payload(struct msg_t *msg)
{
	free(msg->payload);
}


// use sequence:
/* receive_msg_header */
/* receive_msg_payload */
/* handle_msg */
/* dispose_payload */

// testing
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
static void print_app_info( struct app_info_t *app_info)
{
	LOGI("application info=\n");
	LOGI("\tapp_type=<%d><0x%04X>\n"
		 "\tapp_id=<%s>\n"
		 "\texe_path=<%s>\n",
		 app_info->app_type,
		 app_info->app_type,
		 app_info->app_id,
		 app_info->exe_path
	);
}

static void print_conf(struct conf_t * conf)
{
	char buf[1024];
	memset(&buf[0],0,1024);
	feature_code_str(conf->use_features,buf);
	LOGI("conf = \n");
	LOGI("\tuse_features = 0x%016LX (%s)\n",
		 conf->use_features,buf);
	LOGI(
		 "\tsystem_trace_period = %d ms\n"
		 "\tdata message period = %d ms\n",
		 conf->system_trace_period,
		 conf->data_message_period
		 );
}

static void print_us_func_inst(struct us_func_inst_t * func_inst, int count,char * tab)
{
	LOGI("%s#%02d "
		"func_addr = <%Lu>/<0x%08LX>\t"
		"args_num = %d\t"
		"args = <%s>\n",
		tab,count,
		func_inst->func_addr,func_inst->func_addr,
		func_inst->args_num,
		func_inst->args
		);
}

static void print_us_lib_inst(struct us_lib_inst_t * lib_inst,int num,char * tab)
{
	uint32_t i = 0;

//	LOGI("lib_inst = %08x\n",lib_inst);
//	LOGI("lib_inst->us_func_inst_lis = %08x\n",lib_inst->us_func_inst_list);

	LOGI("%s#%02d path = <%s>\n"
		"%s\tfunc_count = %d\n",
		tab,num,
		lib_inst->bin_path,
		tab,lib_inst->func_num
		);

	for ( i = 0; i < lib_inst->func_num; i++) {
		print_us_func_inst(&(lib_inst->us_func_inst_list[i]),i+1,"\t\t\t\t\t\t");
	}

}

static void print_user_space_app_inst(struct app_inst_t * app_inst, int num, char * tab)
{
	uint32_t i = 0;

	LOGI("%s#%02d app_inst : \n"
		"%s\tapp_type = 0x%04X\n"
		"%s\tapp_id=<%s>\n"
		"%s\texec_path=<%s>\n",
		tab,num,
		tab,app_inst->app_type,
		tab,app_inst->app_id,
		tab,app_inst->exec_path
		);
	LOGI("\t%sfunc_num = %d\n", tab, app_inst->func_num);
	for ( i = 0; i < app_inst->func_num; i++) {
		print_us_func_inst(&app_inst->us_func_inst_list[i],i+1,"\t\t\t\t");
	}

	LOGI("\t%slib_num = %d\n", tab, app_inst->lib_num);
	for ( i = 0; i < app_inst->lib_num; i++) {
		print_us_lib_inst(&app_inst->us_lib_inst_list[i],i+1,"\t\t\t\t");
	}


}

static void print_user_space_inst(struct user_space_inst_t * user_space_inst)
{
	uint32_t i = 0;
	LOGI("user_space_inst = \n"
		"\tapp_num = %d\n",
		user_space_inst->app_num
		);
	for ( i = 0; i < user_space_inst->app_num; i ++){
		print_user_space_app_inst(&(user_space_inst->app_inst_list[i]),i+1,"\t\t");
	}
}

static void print_log_interval(log_interval_t log_interval)
{
	printf("log_interval = %x\n", log_interval);
}

static void print_probe(struct probe_t *probe)
{
	printf("probe:\n");
	printf("addr = %lx\n", probe->addr);
	printf("arg_num = %d\n", probe->arg_num);
	int i;
	for (i = 0; i < probe->arg_num; i++) {
		printf("%c", probe->arg_fmt[i]);
	}
	printf("\n");
}

static void print_us_inst(struct us_inst_t *us_inst)
{
	printf("us inst:\n");
	printf("path = %s\n", us_inst->path);
	printf("probe num = %d\n", us_inst->probe_num);
	int i;
	for (i = 0; i < us_inst->probe_num; i++) {
		print_probe(&us_inst->probes[i]);
	}
}

//static
void print_replay_event( struct replay_event_t *ev, uint32_t num, char *tab)
{
	LOGW("%s\t#%04d:time=0x%08X %08X, "
		" id=0x%08X,"
		" type=0x%08X,"
		" code=0x%08X,"
		" value=0x%08X\n",
		tab,num,
		ev->ev.time.tv_sec,//timeval
		ev->ev.time.tv_usec,//timeval
		ev->id,
		ev->ev.type,//u16
		ev->ev.code,//u16
		ev->ev.value//s32
		);
}

void print_replay_event_seq( struct replay_event_seq_t *event_seq)
{
	uint32_t i = 0;
	char *tab="\t";

	LOGI( "%senabled=0x%08X; "\
		"time_start=0x%08X %08X; "\
		"count=0x%08X\n",
		tab,event_seq->enabled,
		event_seq->tv.tv_sec,event_seq->tv.tv_usec,
		event_seq->event_num);
	for (i=0;i<event_seq->event_num;i++)
		print_replay_event(&event_seq->events[i], i+1, tab);

}

static void print_prof_session(struct prof_session_t *prof_session)
{
	printf("prof session\n");
	print_app_info(&prof_session->app_info);
	print_conf(&prof_session->conf);
	print_user_space_inst(&prof_session->user_space_inst);
	print_replay_event_seq(&prof_session->replay_event_seq);

//	print_log_interval(prof_session->log_interval);
//	print_app_inst(&prof_session->app_inst);
}


