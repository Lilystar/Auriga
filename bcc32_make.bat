@ECHO OFF

@echo �R���p�C���J�n
@rem �p�X�̒ǉ��ł������ϐ��Ɋ��ɑg�ݍ���ł���l�͕s�v�ł��B
set PATH=C:\borland\bcc55\bin;C:\borland\bcc55\Include;C:\borland\bcc55\lib;%PATH%

@rem SQL�ŕK�v�Ȃ�R�����g�A�E�g���͂����Đݒ肷��
:set __sqlinclude__=-I"C:\Program Files\MySQL\MySQL Server 5.0\Include"
:set __sqllib__=-L"C:\Program Files\MySQL\MySQL Server 5.0\Lib\Opt" libmysql.lib

@rem �p�P�b�g��`
@rem 2012-09-25aRagexeRE: 20120925
@rem 2012-06-18aRagexeRE: 20120618
@rem 2012-04-04aRagexeRE: 20120404
@rem 2012-03-28aRagexeRE: 20120328
@rem 2012-03-07aRagexeRE: 20120307
@rem 2012-02-22aRagexeRE: 20120222
@rem 2012-02-01aRagexeRE: 20120201
@rem 2011-11-02aRagexeRE: 20111102
@rem 2011-10-25aRagexeRE: 20111025
@rem 2011-09-28aRagexeRE: 20110928
@rem 2011-07-19aRagexeRE: 20110719
@rem 2011-03-09aRagexeRE: 20110309
@rem 2011-01-11aRagexeRE: 20110111
@rem 2010-10-20aRagexeRE: 20101020
@rem 2010-08-03aRagexeRE: 20100803
@rem 2010-07-28aRagexeRE: 20100728
@rem 2010-07-21aRagexeRE: 20100721
@rem 2010-06-29aRagexeRE: 20100629
@rem 2010-06-15aRagexeRE: 20100615
@rem 2010-04-14aRagexeRE: 20100414
@rem 2010-02-23aRagexeRE: 20100223
@rem 2009-12-08aRagexeRE: 20091208
@rem 2009-11-18cRagexeRE: 20091118
@rem 2009-11-04aRagexeRE: 20091104
@rem 2009-07-15aRagexeRE: 20090715
@rem 2009-06-17aRagexeRE: 20090617
@rem 2009-06-03aRagexeRE: 20090603
@rem 2008-11-26cSakexe or 2008-11-26aRagexeRE: 20081126
@rem 2008-11-13aSakexe: 20081113
@rem 2008-08-20aSakexe: 20080820
@rem 2008-05-28aSakexe or 2008-08-27aRagexeRE: 20080827
@rem 2008-01-02aSakexe: 20080102
@rem 2007-11-28aSakexe: 20071128
@rem 2007-11-06aSakexe: 20071106
@rem 2007-09-04aSakexe: 20070904
@rem 2007-07-11aSakexe: 20070711
@rem 2007-05-21aSakexe: 20070521
@rem 2007-02-12aSakexe: 20070212
@rem 2006-10-23aSakexe: 20061023
set __PACKETDEF__=-DPACKETVER=20120925 -DNEW_006b

@rem �R���p�C���I�v�V����
@rem SQL��TEXT�̐؂�ւ��ASQL�łɂ���ꍇ�͈ȉ��̃R���p�C���I�v�V�������R�����g�A�E�g���Ă�������
set __base__=-DTXT_ONLY

@rem txt ���[�h�ŃW���[�i�����g���Ȃ�R�����g�A�E�g���͂���
:set __base__=-DTXT_ONLY -DTXT_JOURNAL

@rem �f�[�^�ۑ����@�� SQL �̎��Atxt-converter ���s�v�Ȃ�R�����g�A�E�g���͂���
:set __TXTCONVERTER__=SKIP

@rem zlib.dll���R���p�C������(�ʏ�̓R�����g�A�E�g)
:set __ZLIB__=-DLOCALZLIB

@rem Login_ID2�ŁAIP������悤�ɂ��܂�(�ʏ�̓R�����g�A�E�g)
@rem Password�̑����IP�����Đڑ����܂��A�ŐV�̓������g����悤�ɂȂ�܂���
@rem �P�[�u��TV���O���[�o��IP���O���ɏo�Ȃ��v���o�C�_����͐ڑ��s�ɂȂ�܂�
:set __SKIP__=-DCMP_AUTHFIFO_IP -DCMP_AUTHFIFO_LOGIN2

@rem httpd �����S�ɖ����ɂ���ꍇ�R�����g�A�E�g���͂���
set __NO_HTTPD__=-DNO_HTTPD

@rem httpd �ŊO�� CGI ���g���ꍇ�̓R�����g�A�E�g����
set __NO_HTTPD_CGI__=-DNO_HTTPD_CGI

@rem csvdb �̃X�N���v�g����̗��p�𖳌��ɂ���ꍇ�R�����g�A�E�g���͂���
:set __NO_CSVDB_SCRIPT__=-DNO_CSVDB_SCRIPT

rem R���O�̃V�X�e�����g���ꍇ�̓R�����g�A�E�g���͂���
set __PRE_RENEWAL__=-DPRE_RENEWAL

@rem MB�ł��ɂ傲�ɂ� (�ʏ�̓R�����g�A�E�g)
:set __EXCLASS__=-DCLASS_MB

@rem ���I��MOB��sc_data���m�ۂ������l�̓R�����g�A�E�g���͂���
set __DYNAMIC_STATUS_CHANGE__=-DDYNAMIC_SC_DATA

@rem �L�����̍폜�Ƀ��[���A�h���X���g���ꍇ�̓R�����g�A�E�g���͂���
:set __AC_MAIL__=-DAC_MAIL

@rem �L�����̍폜�ɒa�������g���ꍇ�̓R�����g�A�E�g���͂���
:set __AC_BIRTHDATE__=-DAC_BIRTHDATE

@rem �X�e�[�^�X�ُ�f�[�^�̕ۑ��𖳌��ɂ���ꍇ�R�����g�A�E�g���͂���
:set __NO_SCDATA_SAVING__=-DNO_SCDATA_SAVING

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
if "%_model_%"=="DCORE" set __cpu__=-6 -a16 -C -d -f -ff -Hc -i133 -Jgd -k- -Oc -Oxt -Ve -VF -xf -xp

set __define__=%__cpu__% %__PACKETDEF__% -DFD_SETSIZE=4096 -D_WIN32 -DWINDOWS %__base__% %__NO_HTTPD__% %__NO_HTTPD_CGI__% %__NO_CSVDB_SCRIPT__% %__ZLIB__% %__SKIP__% %__PRE_RENEWAL__% %__EXCLASS__% %__DYNAMIC_STATUS_CHANGE__% %__AC_MAIL__% %__AC_BIRTHDATE__% %__NO_SCDATA_SAVING__%
set __include__=-I../common/ -I../common/zlib/ %__sqlinclude__%

if "%__ZLIB__%"=="" goto NOZLIB
set __common__=..\common\zlib\*.c
:NOZLIB
set __common__=..\common\*.c %__common__%

if "%__base__%"=="" (set __dbmode__=sql) else (set __dbmode__=txt)


@echo ���R���p�C���I�v�V�����\����
@echo ������������������������������������������������������������������
@echo [%__define__%]
@echo ������������������������������������������������������������������

@rem Warning ��900��o�Ă��ĉ����Ȃ񂾂�������Ȃ��̂ŁA�S���}���B
@rem �C������C�͋N���Ȃ��̂ő��͖{�胂�[�h���ۂ��ł��B
@rem     W8004 : **** �ɑ�������l�͎g���Ă��Ȃ�
@rem     W8008 : ��������ɐ^
@rem     W8012 : �����t���l�ƕ����Ȃ��l�̔�r
@rem     W8057 : �p�����[�^ **** �͈�x���g�p����Ȃ�
@rem     W8060 : �����炭�s���ȑ��
@rem     W8066 : ���s����Ȃ��R�[�h
@rem �抸�����R���p�C���o���Ȃ��Ȃ�[���Ȃ��̂ƃG���[�݂̂̕\��

set __warning__=-w-8004 -w-8008 -w-8012 -w-8057 -w-8060 -w-8066

@echo ���O�C���T�[�o�[�R���p�C��
cd src\login
bcc32 -j255 -M -e..\..\login-server.exe %__warning__% %__define__% %__include__% *.c .\%__dbmode__%\*.c %__common__% %__sqllib__%

@echo �L�����N�^�[�T�[�o�[�R���p�C��
cd ..\char
bcc32 -j255 -M -e..\..\char-server.exe %__warning__% %__define__% %__include__% *.c .\%__dbmode__%\*.c %__common__% %__sqllib__%

@echo �}�b�v�T�[�o�[�R���p�C��
cd ..\map
bcc32 -j255 -M -e..\..\map-server.exe %__warning__% %__define__% %__include__% *.c .\%__dbmode__%\*.c %__common__% %__sqllib__%

@rem �K�v�Ȃ� txt-converter ���R���p�C��
if NOT "%__base__%"=="" goto NOCONVERTER1
if "%__TXTCONVERTER__%"=="SKIP" goto NOCONVERTER1
@echo �R���o�[�^�[�R���p�C��
cd ..\converter
bcc32 -j255 -M -e..\..\txt-converter.exe %__warning__% %__define__% %__include__% *.c %__common__% %__sqllib__%
:NOCONVERTER1

cd ..\..\
@echo �I�u�W�F�N�g�t�@�C�����̃N���[���A�b�v
del src\char\*.obj > NUL
del src\login\*.obj > NUL
del src\map\*.obj > NUL
if NOT "%__base__%"=="" goto NOCONVERTER2
if "%__TXTCONVERTER__%"=="SKIP" goto NOCONVERTER2
del src\converter\*.obj > NUL
:NOCONVERTER2

pause
