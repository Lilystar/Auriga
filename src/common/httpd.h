#ifndef _HTTPD_H_
#define _HTTPD_H_

// ����
//   1.athena������httpd �ő傫�ȃt�@�C���𑗐M���邱�Ƃ͂����߂��܂���B
//     200KB �𒴂���悤�ȃt�@�C���́A�ʂ̃\�t�g�𗘗p���邱�Ƃ����߂܂��B
//   2.�t�@�C�����Ɏg���镶���́A[A-Za-z0-9-_\.] �ł��B���̕������g���ƁA
//     BAD REQUEST �Œe����܂��B

#include <time.h>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <sys/types.h>
#endif

enum {
	HTTPD_METHOD_UNKNOWN = 0, HTTPD_METHOD_GET , HTTPD_METHOD_POST
};

struct httpd_access;

struct httpd_session_data {
	int fd;
	int status;
	int http_ver;
	int header_len;
	int data_len;
	int method;
	int persist;
	int request_count;
	unsigned int tick;
	unsigned char* url;
	unsigned char* query;
	unsigned char* content_type;
	unsigned char* referer;
	unsigned char* user_agent;
	unsigned char* cookie;
	int query_len;
	unsigned char* auth;
	unsigned char user[33];
	unsigned char request_line[1024];
	unsigned char* req_head[32];
	struct httpd_access *access;
	int auth_digest_stale;
	unsigned int precond;
	int reshead_flag;
	time_t date;
	int file_pos;
	unsigned char* filename;
	int range_start, range_end, inst_len;	// �͈͊J�n�ʒu�A�I���ʒu�A�G���e�B�e�B�S�̂̃T�C�Y

	int cgi_state;
#ifdef _WIN32
	HANDLE	cgi_hCIn, cgi_hPIn, cgi_hOut, cgi_hErr, cgi_hProcess;
	DWORD cgi_dwProcessID;
#else
	int cgi_in, cgi_out, cgi_err;
	pid_t cgi_cpid;
#endif
};

void httpd_pages(const char* url,void(*httpd_func)(struct httpd_session_data* sd,const char* url));

// �w�肳�ꂽURL �ɑ΂���R�[���o�b�N�֐���ݒ肷��B���̊֐��́A�ȉ��̂悤��
// ��������K�v������B
//
// 1. URL �́A�擪�̃X���b�V�����Ȃ��ꂽ�t�@�C�����ł��B�Ⴆ�΁A"GET / HTTP/1.0"
//    �Ƃ������Ƀ��N�G�X�g���ꂽ���AURL �ɂ�""(�󕶎�)������A"GET /hoge HTTP/1.0"
//    �̎��ɂ́A"hoge"������܂��B
// 2. ���N�G�X�g���ꂽ�y�[�W������������Ahttpd_send() �܂��́Ahttpd_send_head()
//    ��httpd_send_data() �̑g���Ăяo���A�f�[�^���o�͂���B
// 3. httpd_send_file ���w�肷��ƁAhttpd/ �ȉ��ɂ���t�@�C�����o�͂���B�t�@�C����
//    �󕶎����w�肳�ꂽ���́Aindex.html���w�肳�ꂽ���̂Ƃ݂Ȃ����B

void httpd_erase_pages(const char* url);
// �w�肳�ꂽURL �ɑ΂���R�[���o�b�N�֐����폜����

char* httpd_get_value(struct httpd_session_data* sd,const char* val);

// ���N�G�X�g���ꂽ�A�h���X�ɓn���ꂽ�t�H�[���f�[�^�̂����A�Y�����镶�����Ԃ��B
// �Ⴆ�΁A"GET /status/graph?image=users HTTP/1.0"�Ƃ������N�G�X�g�̏ꍇ�A
// httpd_get_value(sd,"image"); �́A "users"��Ԃ��B���̊֐��̖߂�l�́A�Ăяo������
// ������Ȃ���΂Ȃ�Ȃ��B�܂��A�Y�����镶���񂪖������́A��̕������Ԃ��B

int httpd_get_method(struct httpd_session_data* sd);

// ���N�G�X�g�`����Ԃ��B
// 
//     GET  : HTTPD_METHOD_GET
//     POST : HTTPD_METHOD_POST

unsigned int httpd_get_ip(struct httpd_session_data *sd);

// �N���C�A���g��IP��Ԃ��B


void httpd_default_page(void(*httpd_func)(struct httpd_session_data* sd,const char* url));

// �w�肳�ꂽURL ���o�^����Ă��Ȃ����ɌĂяo���֐���ݒ肷��B���̊֐����Ăяo���Ȃ����A
// �֐��̈�����NULL���w�肷��ƁA404 Not Found ��Ԃ��B




void httpd_send(struct httpd_session_data* sd,int status,const char *content_type,int content_len,const void *data);

//	HTTP�w�b�_�A�f�[�^��g�ɂ��đ��M����B���̊֐����Ăяo������ɁAhttpd_send_data ��
//  �Ăяo���Ă͂Ȃ�Ȃ��B
// 
//		sd           : httpd_set_parse_func() �ɓn���ꂽ���̂����̂܂ܓn�����ƁB
//		status       : HTTP�w�b�_�ɉ�����status�B�ʏ��200�B
//		content_type : ���M����f�[�^�̃^�C�v�Btext/html , image/jpeg �ȂǁB
//		content_len  : ���M����f�[�^�̒����B
//		data         : ���M����f�[�^�ւ̃|�C���^



void httpd_send_head(struct httpd_session_data* sd,int status,const char *content_type,int content_len);

//	HTTP�w�b�_�𑗐M����B
//
//		sd           : ����
//		status       : ����
//		content_type : ����
//      content_len  : content_len��-1�Ɏw�肷�邱�ƂŁA���̊֐����Ă΂ꂽ���_��
//                     ������������Ȃ��f�[�^�𑗐M���邱�Ƃ��ł���B���̏ꍇ��
//                     �����I��HTTP/1.0 �ڑ��ƂȂ�A�I�[�o�[�w�b�h���傫���Ȃ�̂ŁA
//                     ���܂肨���߂͂��Ȃ��B




void httpd_send_data(struct httpd_session_data* sd,int content_len,const void *data);

// �f�[�^�𑗐M����B���̊֐����Ahttpd_send_head() ���Ăяo���O�ɌĂяo���ꂽ�ꍇ�A
// content_type = application/octet-stream, content_len = -1 �Ƃ��ăw�b�_�����M�����B
//		sd           : ����
//      content_len  : ���M����f�[�^��data�������w�肷��B
//      data         : ���M����f�[�^



void httpd_send_file(struct httpd_session_data* sd,const char* url);

// �t�@�C���𑗐M����B���̊֐��́Ahttpd_send_head() ���Ăяo���O�ɌĂяo���Ȃ����
// �Ȃ�Ȃ��B�t�@�C���ɋ󕶎����w�肳�ꂽ�Ƃ��́Aindex.html���w�肳�ꂽ�ƌ��Ȃ����B



void httpd_send_error(struct httpd_session_data* sd,int status);

// HTTP�G���[���b�Z�[�W�𑗐M����Bstatus ��HTTP�̃G���[�R�[�h�Ɠ����B
// 	400 Bad Request, 404 Not Found, 500 Internal Server Error �ȂǁB

int  httpd_parse(int fd);


// �F�؊֐��̊֐��|�C���^�� typedef
struct httpd_access;
typedef int(*HTTPD_AUTH_FUNC)( struct httpd_access* a, struct httpd_session_data* sd, const char *userid, char *passwd );
// * ���̊֐��|�C���^�̐�̊֐����s���ׂ����� *
// userid �ɑΉ�����v���[���p�X���[�h�� passwd (33�o�C�g�܂�) �ɐݒ肵�ĕԂ�
//  �߂�l: 0 �ŃG���[, 1 �Ő���
//  sd     = httpd �̃Z�b�V�����f�[�^�ւ̃|�C���^
//  a      = �F�؏��ւ̃|�C���^
//  userid = �F�؂̃��[�U�[ID�ւ̃|�C���^
//  passwd = �p�X���[�h��Ԃ����߂̃|�C���^�i������ strcpy ����j


// �F�؊֐��̐ݒ�ifunc_id �� httpd.conf �� authfunc �̔ԍ��j
void httpd_set_auth_func( int func_id, HTTPD_AUTH_FUNC func );

// ����������
void do_init_httpd(void);

// �h�L�������g���[�g�ݒ�
void httpd_set_document_root( const char *str );

// �^�C���A�E�g���Ԑݒ�
void httpd_set_request_timeout( int idx, int time );

// �����ʐM�ł̃��N�G�X�g�ő吔�ݒ�
void httpd_set_max_persist_requests( int count );

// ���O�t�@�C�����ݒ�
void httpd_set_logfile( const char *str );

// �ݒ�t�@�C���ǂݍ���
int httpd_config_read(char *cfgName);


// ���^����( < , > , & , " ) ���G�X�P�[�v�����������Ԃ��B
// XSS (�N���X�T�C�g�X�N���v�e�B���O)�ɑΏ����邽�߁A���[�U�[�����͂���
// ���������̂܂܏o�͂��鎞�́A���̊֐���ʂ��ăG�X�P�[�v�����������o��
// ����ׂ��ł��B���̊֐��̖߂�l�́A�Ăяo������������Ȃ���΂Ȃ�Ȃ��B
char* httpd_quote_meta(const char* p1);

// �^����ꂽ������� %xx �̌`�ɃG���R�[�h�����������Ԃ��B
// URL �̒��ɓ��{�ꂪ�������UFT-8�����đ��M����u���E�U������̂ŁA
// �}���`�o�C�g������ <a href=""> �̒��ɓ���鎞�Ɏg�p���Ă��������B
// ���̊֐��̖߂�l�́A�Ăяo������������Ȃ���΂Ȃ�Ȃ��B
char* httpd_binary_encode(const char* val);

// base64 ���f�R�[�h����B
// dest �ɂ͏\���ȃT�C�Y( [src �̒���] / 4 * 3 )�́A�������m�ۂ���
// �����K�v������B ��������� 1, ���s����� 0 ���Ԃ�
int httpd_decode_base64( char *dest, const char *src);

#endif
