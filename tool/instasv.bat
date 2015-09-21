@echo off
rem Windows�T�[�r�X�֓o�^���s���o�b�`�t�@�C��

rem ----------------------------------------------------------------
rem �T�[�r�X���ݒ�
set __LOGIN_SVC__=AurigaLoginServer
set __CHAR_SVC__=AurigaCharServer
set __MAP_SVC__=AurigaMapServer

rem �T�[�r�X�\�����E�����ݒ�
set __LOGIN_DESC__=Auriga Login Server
set __CHAR_DESC__=Auriga Char Server
set __MAP_DESC__=Auriga Map Server

rem �W���o�͂̃��_�C���N�g�惍�O�t�@�C�����ݒ�
set __LOGIN_LOGFILE__=./log/login-stdout.log
set __CHAR_LOGFILE__=./log/char-stdout.log
set __MAP_LOGFILE__=./log/map-stdout.log
rem ----------------------------------------------------------------

rem �J�����g�f�B���N�g���̕␳
cd %~dp0
cd ..
set __BASE_DIR__=%cd%

echo ��Auriga���T�[�r�X�Ƃ��ēo�^���܂��B��

if "%__LOGIN_SVC__%"=="" goto SKIP1
echo Installing Service "%__LOGIN_SVC__%" ...
sc.exe create %__LOGIN_SVC__% DisplayName= "%__LOGIN_DESC__%" binPath= "%__BASE_DIR__%\login-server.exe --service-name %__LOGIN_SVC__% --stdout %__LOGIN_LOGFILE__%"
sc.exe description %__LOGIN_SVC__% "%__LOGIN_DESC__%"
echo.
:SKIP1

if "%__CHAR_SVC__%"=="" goto SKIP2
echo Installing Service "%__CHAR_SVC__%" ...
sc.exe create %__CHAR_SVC__% DisplayName= "%__CHAR_DESC__%" binPath= "%__BASE_DIR__%\char-server.exe --service-name %__CHAR_SVC__% --stdout %__CHAR_LOGFILE__%"
sc.exe description %__CHAR_SVC__% "%__CHAR_DESC__%"
echo.
:SKIP2

if "%__MAP_SVC__%"=="" goto SKIP3
echo Installing Service "%__MAP_SVC__%" ...
sc.exe create %__MAP_SVC__% DisplayName= "%__MAP_DESC__%" binPath= "%__BASE_DIR__%\map-server.exe --service-name %__MAP_SVC__%  --stdout %__MAP_LOGFILE__%"
sc.exe description %__MAP_SVC__% "%__MAP_DESC__%"
echo.
:SKIP3

echo ��Login, Char, Map�T�[�o��Windows�̃T�[�r�X�Ƃ��ēo�^���܂����B
echo ����Q���̎����N����ON�ɂ���ꍇ��Windows�́u�Ǘ��c�[���v����u�T�[�r�X�v��I�����Ă��������B

pause
