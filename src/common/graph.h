#ifndef _GRAPH_H_
#define _GRAPH_H_

void do_init_graph(void);

// auriga�̏�Ԃ𒲍�����Z���T�[��ǉ�����B
// string        : �Z���T�[�̖���(Login Users �Ȃ�)
// inetrval      : �Z���T�[�̒l���擾����Ԋu(msec)
// callback_func : �Z���T�[�̒l��Ԃ��֐�( unsigned int login_users(void); �Ȃ�)

void graph_add_sensor(const char* string, int interval, double (*callback_func)(void));

#endif

