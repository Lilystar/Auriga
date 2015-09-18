#ifndef _INTER_H_
#define _INTER_H_

int inter_init(const char *file);
int inter_sync(void);
int inter_parse_frommap(int fd);
int inter_mapif_init(int fd);

int inter_check_length(int fd,int length);

int inter_log(char *fmt,...);

#define inter_cfgName "conf/inter_athena.conf"

extern char inter_log_filename[1024];

void do_final_inter(void);

#endif
