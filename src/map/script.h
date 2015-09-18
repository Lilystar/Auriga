#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include "utils.h"

struct script_data {
	int type;
	union {
		int num;
		char *str;
	} u;
	struct linkdb_node** ref; // ���t�@�����X
};

struct script_code {
	int script_size;
	unsigned char* script_buf;
	struct linkdb_node* script_vars;
};

struct script_state {
	struct script_stack {
		int sp,sp_max,defsp;
		struct script_data *stack_data;
		struct linkdb_node **var_function;	// �֐��ˑ��ϐ�
	} *stack;
	int start,end;
	int pos,state;
	int rid,oid;
	struct script_code *script, *scriptroot;
	struct sleep_data {
		int tick,timer,charid;
	} sleep;
};

#define SCRIPT_CONF_NAME	"conf/script_auriga.conf"

struct script_code* parse_script(unsigned char *,const char*,int);
void run_script(struct script_code*,int,int,int);
void script_error(char *src,const char *file,int start_line, const char *error_msg, const char *error_pos);

void script_free_stack(struct script_stack *stack);
void script_free_code(struct script_code* code);

struct dbt* script_get_label_db(void);
struct dbt* script_get_userfunc_db(void);

int script_config_read(char *cfgName);
int do_init_script(void);
int do_final_script(void);

extern struct script_code error_code;

// @readvars, @writevars
int script_check_variable(const char *name,int array_flag,int read_only);
void* script_read_vars(struct map_session_data *sd,char *var,int elem,struct linkdb_node **ref);
void script_write_vars(struct map_session_data *sd,char *var,int elem,void *v,struct linkdb_node **ref);

#ifdef TXT_ONLY
extern char mapreg_txt[256];
#else
extern MYSQL mysql_handle_script;
#endif /* TXT_ONLY */

#endif

