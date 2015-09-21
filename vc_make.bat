@echo off
rem VC++ �ł̃r���h�p�o�b�`�t�@�C��

rem �Ή�����R���p�C���̃o�[�W����
rem  - Visual C++ 6.0
rem  - Visual C++ .NET (Visual C++ .NET 2002)
rem  - Visual C++ .NET 2003
rem  - Visual C++ Toolkit 2003
rem  - Visual C++ 2005 (Express Edition �܂�)
rem  - Visual C++ 2008 Express Edition
rem  - Visual C++ 2008 64bit�R���p�C��
rem  - Visual C++ 2010 Express Edition

rem ----------------------------------------------------------------
rem ���ʐݒ�
echo vc_make.bat - Auriga makefile for Visual C++
set __VCVER__=7
set __BITTYPE__=x32

rem ----------------------------------------------------------------
rem �p�P�b�g��`
rem 2010-08-03aRagexeRE: 31
rem 2010-07-28aRagexeRE: 30
rem 2010-07-21aRagexeRE: 29
rem 2010-06-29aRagexeRE: 28
rem 2010-04-14aRagexeRE: 27
rem 2010-02-23aRagexeRE: 26
rem 2009-12-08aRagexeRE: 25
rem 2009-11-18cRagexeRE: 24
rem 2009-11-04aRagexeRE: 23
rem 2009-07-15aRagexeRE: 22
rem 2009-06-17aRagexeRE: 21
rem 2009-06-03aRagexeRE: 20
rem 2009-01-14aSakexe or 2009-01-14aRagexeRE: 19
rem 2008-11-26cSakexe or 2008-11-26aRagexeRE: 18
rem 2008-11-13aSakexe: 17
rem 2008-08-20aSakexe: 16
rem 2008-05-28aSakexe or 2008-08-27aRagexeRE: 15
rem 2008-01-02aSakexe: 14
rem 2007-11-28aSakexe: 13
rem 2007-11-06aSakexe: 12
rem 2007-09-04aSakexe: 11
rem 2007-07-11aSakexe: 10
rem 2007-05-21aSakexe:  9
rem 2007-02-12aSakexe:  8
set __PACKETDEF__=/D "PACKETVER=31" /D "NEW_006b" /D "NEW_006b_RE"

rem ----------------------------------------------------------------
rem �R���p�C�����Ƃ̐ݒ�i�����̊��ɂ������̂̃R�����g�A�E�g���͂����j

rem ---- VC++ Toolkit 2003 �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set PATH=C:\Program Files\Microsoft Visual C++ Toolkit 2003\bin;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem set INCLUDE=C:\Program Files\Microsoft Visual C++ Toolkit 2003\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem set LIB=C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%
rem ---- VC++ Toolkit 2003 �̐ݒ肱���܂�

rem ---- VC++ 2005 Express Edition �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set PATH=C:\Program Files\Microsoft Visual Studio 8\VC\bin;C:\Program Files\Microsoft Visual Studio 8\Common7\IDE;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem set INCLUDE=C:\Program Files\Microsoft Visual Studio 8\VC\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem set LIB=C:\Program Files\Microsoft Visual Studio 8\VC\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%
rem set __VCVER__=8
rem ---- VC++ 2005 Express Edition �̐ݒ肱���܂�

rem ---- VC++ 2005 (Express�ȊO) �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\VCVARS32.BAT"
rem set __VCVER__=8
rem ---- VC++ 2005 (Express�ȊO) �̐ݒ肱���܂�

rem ---- VC++ 2008 Express Edition �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set PATH=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin;C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem set INCLUDE=C:\Program Files\Microsoft Visual Studio 9.0\VC\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem set LIB=C:\Program Files\Microsoft Visual Studio 9.0\VC\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%
rem set __VCVER__=9
rem ---- VC++ 2008 Express Edition �̐ݒ肱���܂�

rem ---- VC++ 2008 64bit�R���p�C�� (Express�ȊO) �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set PATH=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\x86_amd64;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\bin;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE;%PATH%
rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\include;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\altmfc\include;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include;%INCLUDE%
rem set LIB=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\lib\amd64;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib\x64;C:\Program Files (x86)\Microsoft Visual Studio 9.0\SDK\v2.0\lib\amd64;%LIB%
rem set __VCVER__=9
rem set __BITTYPE__=x64
rem ---- VC++ 2008 64bit�R���p�C�� (Express�ȊO) �̐ݒ肱���܂�

rem ---- VC++ 2010 Express Edition �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set PATH=C:\Program Files\Microsoft Visual Studio 10.0\VC\bin;C:\Program Files\Microsoft Visual Studio 10.0\Common7\IDE;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem set INCLUDE=C:\Program Files\Microsoft Visual Studio 10.0\VC\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem set LIB=C:\Program Files\Microsoft Visual Studio 10.0\VC\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%
rem set __VCVER__=10
rem ---- VC++ 2010 Express Edition �̐ݒ肱���܂�

rem ---- VC++ .NET 2003 �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
rem ---- VC++ .NET 2003 �̐ݒ肱���܂�

rem ---- VC++ .NET (2002) / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio .NET\Vc7\bin\vcvars32.bat"
rem ---- VC++ .NET (2002) �̐ݒ� �����܂�

rem ---- VC++ 6.0 / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"
rem set __VCVER__=6
rem ---- VC++ 6.0 �̐ݒ肱���܂�

rem ----------------------------------------------------------------
rem SQL �̐ݒ� / �K�v�Ȃ�R�����g�A�E�g���͂���
rem set INCLUDE=C:\Program Files\MySQL\MySQL Server 5.0\Include;%INCLUDE%
rem set LIB=C:\Program Files\MySQL\MySQL Server 5.0\Lib\Opt;%LIB%

rem ----------------------------------------------------------------
rem VC8�ȏ� �Ŏw��ł��Ȃ��I�v�V�����̉������
if "%__VCVER__%"=="8" goto SKIPVC8
if "%__VCVER__%"=="9" goto SKIPVC8
if "%__VCVER__%"=="10" goto SKIPVC8
set __OPT_OP__=/Op
:SKIPVC8

rem ----------------------------------------------------------------
rem VC6 �Ŏw��ł��Ȃ��I�v�V�����̉������
if "%__VCVER__%"=="6" goto SKIPVC6
set __FIXOPT2__=/MAP /nologo
:SKIPVC6

rem ----------------------------------------------------------------
rem �r���h�I�v�V�����̑I��

rem �f�[�^�ۑ����@�̑I�� �F SQL �ɂ���Ȃ�R�����g�A�E�g����
set __TXT_MODE__=/D "TXT_ONLY"

rem �f�[�^�ۑ����@�� TXT �̎��A�W���[�i�����g���Ȃ�R�����g�A�E�g���͂���
rem set __TXT_MODE__=/D "TXT_ONLY" /D "TXT_JOURNAL"

rem �f�[�^�ۑ����@�� SQL �̎��Atxt-converter ���s�v�Ȃ�R�����g�A�E�g���͂���
rem set __TXTCONVERTER__=SKIP

rem zlib.dll���R���p�C������Ȃ�R�����g�A�E�g���͂���
rem set __ZLIB__=/D "LOCALZLIB"

rem login_id2 �� IP �� AUTHFIFO ���r����ꍇ�̓R�����g�A�E�g���͂���
rem set __CMP_AFL2__=/D "CMP_AUTHFIFO_LOGIN2"
rem set __CMP_AFIP__=/D "CMP_AUTHFIFO_IP"

rem httpd �����S�ɖ����ɂ���ꍇ�R�����g�A�E�g���͂���
rem set __NO_HTTPD__=/D "NO_HTTPD"

rem httpd �ŊO�� CGI ���g���ꍇ�̓R�����g�A�E�g����
set __NO_HTTPD_CGI__=/D "NO_HTTPD_CGI"

rem csvdb �̃X�N���v�g����̗��p�𖳌��ɂ���ꍇ�R�����g�A�E�g���͂���
rem set __NO_CSVDB_SCRIPT__=/D "NO_CSVDB_SCRIPT"

rem MB ���g���ꍇ�̓R�����g�A�E�g���͂���
rem set __EXCLASS__=/D "CLASS_MB"

rem ���I��MOB�� sc_data ���m�ۂ���ꍇ�̓R�����g�A�E�g���͂���
rem set __DYNAMIC_STATUS_CHANGE__=/D "DYNAMIC_SC_DATA"

rem �L�����̍폜�Ƀ��[���A�h���X���g���ꍇ�̓R�����g�A�E�g���͂���
rem set __AC_MAIL__=/D "AC_MAIL"

rem �L�����̍폜�ɒa�������g���ꍇ�̓R�����g�A�E�g���͂���
rem set __AC_BIRTHDATE__=/D "AC_BIRTHDATE"

rem �X�e�[�^�X�ُ�f�[�^�̕ۑ��𖳌��ɂ���ꍇ�̓R�����g�A�E�g���͂���
rem set __NO_SCDATA_SAVING__=/D "NO_SCDATA_SAVING"

rem ---------------------------
rem �R���p�C���I�v�V�����ݒ�

@rem CPU�œK���X�C�b�`(By Nameless)
@rem �ȉ��̗���Q�l�ɃX�C�b�`�����L�����Ă��������B
set _model_=x32

@rem �œK���Ȃ�
if "%_model_%"=="NOOPTIMIZE" set __cpu__=/c /W3 /Od /Zi

@rem CPU�A�[�L�e�N�`��32BitCPU/64BitCPU
if "%_model_%"=="x32" set __cpu__=/c /W3 /O2 %__OPT_OP__% /GA /TC /Zi
if "%_model_%"=="x64" set __cpu__=/c /arch:SSE2 /W3 /O2 %__OPT_OP__% /GA /TC /Zi

@rem �������[1024�ȏ㓋�ڂ�32bitCPU/64bitCPU
if "%_model_%"=="HiMemL" set __cpu__=/c /bigobj /W3 /O2 %__OPT_OP__% /GA /TC /Zi
if "%_model_%"=="HiMemH" set __cpu__=/c /bigobj /arch:SSE2 /W3 /O2 %__OPT_OP__% /GA /TC /Zi

@rem �X�^�b�N������R���p�C���ōs���ꍇ
if "%_model_%"=="Stac32" set __cpu__=/c /F4096 /W3 /O2 %__OPT_OP__% /GA /TC /Zi
if "%_model_%"=="Stac64" set __cpu__=/c /F4096 /arch:SSE2 /W3 /O2 %__OPT_OP__% /GA /TC /Zi
@rem AMD�n64bitCPU�p
if "%_model_%"=="A64x2" set __cpu__=/c /favor:blend /W3 /O2 %__OPT_OP__% /GA /TC /Zi
if "%_model_%"=="A64x1" set __cpu__=/c /favor:AMD64 /W3 /O2 %__OPT_OP__% /GA /TC /Zi

@rem Intel�n64bitCPU�p
if "%_model_%"=="EM64T" set __cpu__=/c /favor:EM64T /W3 /O2 %__OPT_OP__% /GA /TC /Zi


@rem �ȉ������i�K(�l������ by Nameless)
@rem �\�������c32BitCPU�ō������[�h
if "%_model_%"=="mode01" set __cpu__=/c /fp:fast /F4096 /bigobj /W3 /Ox /GA /TC /Zi
@rem �\�������c64BitCPU�ō������[�h
if "%_model_%"=="mode02" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /W3 /Ox /Gr /GA /TC /Zi
@rem �\�������cAMD 64x2 & FX�n�œK���E�ō���
if "%_model_%"=="mode03" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:AMD64 /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode04" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:blend /W3 /Ox /Gr /GA /TC /Zi
@rem �\�������cIntel 64bitCPU�p�œK���E�ō���
if "%_model_%"=="mode05" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:EM64T /W3 /Ox /Gr /GA /TC /Zi
@rem �ȉ����U�[�u
if "%_model_%"=="mode06" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode07" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode08" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode09" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi



rem ----------------------------------------------------------------
rem �ŏI�I�ȃr���h�I�v�V�����𐶐�
if "%__ZLIB__%"=="" goto NOZLIB1
set __LINKZLIB__=../common/zlib/*.obj
:NOZLIB1

if "%__BITTYPE__%"=="x32" set __BITOPTION__=/D "WIN32" /D "_WIN32" /D "_WIN32_WINDOWS"
if "%__BITTYPE__%"=="x64" set __BITOPTION__=/D "WIN64" /D "_WIN64"

set __opt1__=/D "FD_SETSIZE=4096" /D "NDEBUG" /D "_CONSOLE" /D "_CRT_SECURE_NO_DEPRECATE" /D "WINDOWS" %__BITOPTION__% %__PACKETDEF__% %__TXT_MODE__% %__ZLIB__% %__CMP_AFL2__% %__CMP_AFIP__% %__NO_HTTPD__% %__NO_HTTPD_CGI__% %__NO_CSVDB_SCRIPT__% %__EXCLASS__% %__DYNAMIC_STATUS_CHANGE__% %__AC_MAIL__% %__AC_BIRTHDATE__% %__NO_SCDATA_SAVING__%
set __opt2__=/DEBUG %__FIXOPT2__% user32.lib %__LINKZLIB__% ../common/*.obj *.obj
set __include__=/I "../common/zlib/" /I "../common/"

if "%__TXT_MODE__%"=="" (set __filter__=txt) else (set __filter__=sql)

rem ----------------------------------------------------------------
rem �x���̗}��
rem   C4819 : �\���ł��Ȃ��������܂�ł��܂�
set __warning__=/wd4819

rem ----------------------------------------------------------------
rem �R���p�C���I�v�V�����̕\��

echo ���R���p�C�����\����
echo ������������������������������������������������������������������
echo [VCVER = %__VCVER__%]
echo [BITTYPE = %__BITTYPE__%]
echo [model = %_model_%]
echo [CompileOption = %__opt1__%]
echo ������������������������������������������������������������������

rem �r���h��Ɩ{��

rem ���ʃR���|�[�l���g�̃R���p�C��
cd src\common\zlib
if "%__ZLIB__%"=="" goto NOZLIB2
echo zlib�̃R���p�C��
cl %__warning__% %__cpu__% %__opt1__% %__include__% *.c
:NOZLIB2
echo ���ʃR���|�[�l���g�̃R���p�C��
cd ..\
cl %__warning__% %__cpu__% %__opt1__% %__include__% *.c 

rem �T�[�o�[�{�̂̃r���h
echo ���O�C���T�[�o�[�R���p�C��
cd ..\login
set __filelist__=
for %%a in (*.c) do call :FILEFILTER %%a
cl %__warning__% %__cpu__% %__opt1__% %__include__% %__filelist__% 
link %__opt2__% /out:"../../login-server.exe"
echo �L�����N�^�[�T�[�o�[�R���p�C��
cd ..\char
set __filelist__=
for %%a in (*.c) do call :FILEFILTER %%a
cl %__warning__% %__cpu__% %__opt1__% %__include__% %__filelist__% 
link %__opt2__% /out:"../../char-server.exe"
echo �}�b�v�T�[�o�[�R���p�C��
cd ..\map
cl %__warning__% %__cpu__% %__opt1__% %__include__% *.c 
link %__opt2__% /out:"../../map-server.exe"

rem �K�v�Ȃ� txt-converter ���r���h
if NOT "%__TXT_MODE__%"=="" goto NOCONVERTER1
if "%__TXTCONVERTER__%"=="SKIP" goto NOCONVERTER1
echo �R���o�[�^�[�R���p�C��
cd ..\converter
cl %__warning__% %__cpu__% %__opt1__% %__include__% *.c 
link %__opt2__% /out:"../../txt-converter.exe"
:NOCONVERTER1

cd ..\..\

rem �s�K�v�ȃt�@�C�����폜
echo �I�u�W�F�N�g�t�@�C�����̃N���[���A�b�v
if "%__ZLIB__%"=="" goto NOZLIB3
del src\common\zlib\*.obj
:NOZLIB3
del src\common\*.obj
del src\char\*.obj
del src\login\*.obj
del src\map\*.obj
if NOT "%__TXT_MODE__%"=="" goto NOCONVERTER2
if "%__TXTCONVERTER__%"=="SKIP" goto NOCONVERTER2
del src\converter\*.obj
:NOCONVERTER2

rem ���ʊm�F�p�̈ꎞ��~
pause

goto :EOF

:FILEFILTER
set ARG=%1
if not "%ARG:~-6%" == "_%__filter__%.c" (
	set __filelist__=%__filelist__% %1
)
