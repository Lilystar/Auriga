@ECHO OFF
@echo �Â��I�u�W�F�N�g�t�@�C�����̃N���[���A�b�v

del src\char\*.obj > NUL
del src\char\char.exe > NUL
del src\login\*.obj > NUL
del src\login\login.exe > NUL
del src\map\*.obj > NUL
del src\map\map.exe > NUL
del login-server.exe > NUL
del char-server.exe > NUL
del map-server.exe > NUL
del char-converter.exe > NUL
del login-converter.exe > NUL
del src\converter\*.obj > NUL
del src\converter\*.exe > NUL

@echo �R���p�C���J�n
@rem �p�X�̒ǉ��ł������ϐ��Ɋ��ɑg�ݍ���ł���l�͕s�v�ł��B
set path=%path%;C:\borland\bcc55\bin;C:\borland\bcc55\Include;C:\borland\bcc55\lib

set __common__=..\common\core.c ..\common\db.c ..\common\grfio.c ..\common\lock.c ..\common\malloc.c ..\common\nullpo.c ..\common\socket.c ..\common\timer.c ..\common\httpd.c ..\common\graph.c ..\common\md5calc.c ..\common\utils.c

@rem �R���p�C���I�v�V����
@rem SQL��TEXT�̐؂�ւ��ASQL�łɂ���ꍇ�͈ȉ��̃R���p�C���I�v�V�������R�����g�A�E�g���Ă�������
:set __base__=-DTXT_ONLY

@rem Zlib.dll���R���p�C������(�ʏ�̓R�����g�A�E�g)
:set __ZLIB__=-DLOCALZLIB

@rem Login_ID2�ŁAIP������悤�ɂ��܂�(�ʏ�̓R�����g�A�E�g)
@rem Password�̑����IP�����Đڑ����܂��A�ŐV�̓������g����悤�ɂȂ�܂���
@rem �P�[�u��TV���O���[�o��IP���O���ɏo�Ȃ��v���o�C�_����͐ڑ��s�ɂȂ�܂�
:set __SKIP__=-DCMP_AUTHFIFO_IP -DCMP_AUTHFIFO_LOGIN2

@rem httpd �����S�ɖ����ɂ���ꍇ�R�����g�A�E�g���͂���(�ʏ�̓R�����g�A�E�g)
:set __NO_HTTPD__=-DNO_HTTPD

@rem DKDC�ł��ɂ傲�ɂ� (�ʏ�̓R�����g�A�E�g)
:set __EXCLASS__=-DCLASS_DKDC

@rem ���I��MOB��sc_data���m�ۂ������l�̓R�����g�A�E�g���͂���
:set __DYNAMIC_STATUS_CHANGE__=-DDYNAMIC_SC_DATA

@rem account regist MailAddress
:set __AC_MAIL__=-DAC_MAIL

@rem CPU�œK���X�C�b�` �ȉ��̗���Q�l��CPU�����L�����Ă��������B
set _model_=Pentium4

@rem 8000�T���v�� ���v�̌��ʂ��R���p�C���ɔ��f�����܂�
if "%_model_%"=="Athlon" set __cpu__=-6 -O2
if "%_model_%"=="Athlon_XP" set __cpu__=-5
if "%_model_%"=="Athlon_MP" set __cpu__=-5
if "%_model_%"=="Duron" set __cpu__=-5
if "%_model_%"=="Sempron" set __cpu__=-5
if "%_model_%"=="586" set __cpu__=-4 -VI
if "%_model_%"=="686" set __cpu__=-4 -VI
if "%_model_%"=="Pentium2" set __cpu__=-4
if "%_model_%"=="Pentium3" set __cpu__=-5
if "%_model_%"=="Pentium4" set __cpu__=-5
if "%_model_%"=="P2_Celeron" set __cpu__=-4
if "%_model_%"=="P3_Celeron" set __cpu__=-5
if "%_model_%"=="P4_Celeron" set __cpu__=-5 -Ov
if "%_model_%"=="P3_Xeon" set __cpu__=-6
if "%_model_%"=="P4_Xeon" set __cpu__=-6 -O2 -f
if "%_model_%"=="CeleronD" set __cpu__=-5
if "%_model_%"=="MobileP2" set __cpu__=-4 -O2 -Ov
if "%_model_%"=="MobileP3" set __cpu__=-5 -O2
if "%_model_%"=="MobileP4" set __cpu__=-5 -Oc -Ov
if "%_model_%"=="PentiumM" set __cpu__=-5 -O1
if "%_model_%"=="Crusoe" set __cpu__=-4 -Oc
if "%_model_%"=="Opteron" set __cpu__=-5 -O2 -Oi -f -tWM
if "%_model_%"=="Athlon_64" set __cpu__=-6 -Oi -tWM
if "%_model_%"=="Athlon_64x2" set __cpu__=-6 -Ov -Oi -f -ff -tWM
if "%_model_%"=="Turion64" set __cpu__=-4 -Oc -Ov -Oi
if "%_model_%"=="Itanium64" set __cpu__=-4 -Oc -Ov -Oi
if "%_model_%"=="Prescott" set __cpu__=-5 -Oc -Ov -tWM
if "%_model_%"=="Prescott2" set __cpu__=-5 -Oc -Ov -Oi -f -ff -tWM
if "%_model_%"=="Smithfield" set __cpu__=-6 -Oc -Ov -Oi -tWM
if "%_model_%"=="Prestonia" set __cpu__=-6 -Oc -Ov -Oi -f -ff -tWM
if "%_model_%"=="Manchester" set __cpu__=-6 -Oi -f -ff -tWM
if "%_model_%"=="Pentium4X" set __cpu__=-5 -Oc -Ov -Oi -f -ff -tWM
if "%_model_%"=="Athlon_X" set __cpu__=-5 -Oc -Ov -Oi -f -ff -tWM
if "%_model_%"=="Pentium4C" set __cpu__=-5 -Ov -Oi
if "%_model_%"=="AMD32" set __cpu__=-5 -Oc -Ov -f -ff
if "%_model_%"=="AMD64" set __cpu__=-6 -Oc -Ov -f -ff -tWM

set __define__=%__cpu__% -DPACKETVER=7 -DNEW_006b -DFD_SETSIZE=4096 %__base__% %__NO_HTTPD__% %__ZLIB__% %__SKIP__% %__EXCLASS__% %__DYNAMIC_STATUS_CHANGE__% %__AC_MAIL__%
set __include__=-I../common/

@echo ���R���p�C���I�v�V�����\����
@echo ������������������������������������������������������������������
@echo [%__define__%]
@echo ������������������������������������������������������������������

@rem Warning ��900��o�Ă��ĉ����Ȃ񂾂�������Ȃ��̂ŁA�S���}���B
@rem �C������C�͋N���Ȃ��̂ő��͖{�胂�[�h���ۂ��ł��B
@rem     W7035 : �����t���l�ƕ����Ȃ��l�̔�r
@rem     W8004 : **** �ɑ�������l�͎g���Ă��Ȃ�
@rem     W8012 : �����t���l�ƕ����Ȃ��l�̔�r
@rem     W8017 : �}�N�� '****' �̏d����`
@rem     W8057 : �p�����[�^ **** �͈�x���g�p����Ȃ�
@rem     W8060 : �����炭�s���ȑ��
@rem     W8064 : �v���g�^�C�v�錾�̂Ȃ��֐�
@rem     W8065 : �v���g�^�C�v�錾�̂Ȃ��֐�
@rem     W8066 : ���s����Ȃ��R�[�h
@rem     W8070 : �֐��͒l��Ԃ��ׂ�
@rem     W8075 : ���̂���|�C���^�̕ϊ�
@rem �抸�����R���p�C���o���Ȃ��Ȃ�[���Ȃ��̂ƃG���[�݂̂̕\��

set __warning__=-w-7035 -w-8004 -w-8012 -w-8017 -w-8057 -w-8060 -w-8064 -w-8065 -w-8066 -w-8070 -w-8075

@echo ���O�C���T�[�o�[�R���p�C��
cd src\login
bcc32 -j255 %__warning__% %__define__% %__include__% login.c libmysql.lib %__common__%
copy login.exe ..\..\login-server.exe > NUL

@echo �L�����N�^�[�T�[�o�[�R���p�C��
cd ..\char
bcc32 -j255 %__warning__% %__define__% %__include__% char.c libmysql.lib int_pet.c int_guild.c int_party.c int_storage.c inter.c %__common__%
copy char.exe ..\..\char-server.exe > NUL

@echo �R���o�[�^�[�R���p�C��
cd ..\converter
bcc32 -j255 %__warning__% %__define__% %__include__% char-converter.c libmysql.lib converter.c login-converter.c inter-converter.c map-converter.c %__common__%
copy converter.exe ..\..\converter.exe > NUL

cd ..\..\
@echo �I�u�W�F�N�g�t�@�C�����̃N���[���A�b�v
del src\char\*.obj > NUL
del src\char\char.exe > NUL
del src\login\*.obj > NUL
del src\login\login.exe > NUL
del src\map\*.obj > NUL
del src\map\map.exe > NUL
pause

