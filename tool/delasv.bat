@echo off
rem Windows�T�[�r�X����폜���s���o�b�`�t�@�C��

rem ----------------------------------------------------------------
rem �T�[�r�X���ݒ�
set __LOGIN_SVC__=AurigaLoginServer
set __CHAR_SVC__=AurigaCharServer
set __MAP_SVC__=AurigaMapServer
rem ----------------------------------------------------------------

echo ��Auriga���T�[�r�X����폜���܂���

if "%__LOGIN_SVC__%"=="" goto SKIP1
echo UnInstalling Service "%__LOGIN_SVC__%" ...
sc.exe delete %__LOGIN_SVC__%
echo.
:SKIP1

if "%__CHAR_SVC__%"=="" goto SKIP2
echo UnInstalling Service "%__CHAR_SVC__%" ...
sc.exe delete %__CHAR_SVC__%
echo.
:SKIP2

if "%__MAP_SVC__%"=="" goto SKIP3
echo UnInstalling Service "%__MAP_SVC__%" ...
sc.exe delete %__MAP_SVC__%
echo.
:SKIP3

echo ��Login, Char, Map�T�[�o��Windows�̃T�[�r�X������������݂܂����B
echo ���G���[���������Ă���ꍇ�́A���炩���߃T�[�r�X���~���Ă���ēx���s���Ă��������B

pause
