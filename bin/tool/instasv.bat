@echo off
@echo ��Athena���T�[�r�X�Ƃ��ēo�^���܂��B

@rem -----�ݒ蕔---------
set athenasrv="c:\athena"
@rem athena�̃o�C�i���̂���ꏊ���w�肵�܂��B
@rem --------------------
if "%athenasrv%"=="" goto error

sc.exe Create AthenaLogin binPath= %athenasrv%\login-server.exe
sc.exe Create AthenaChar binPath= %athenasrv%\char-server.exe
sc.exe Create AthenaMap binPath= %athenasrv%\map-server.exe

@echo ��Login.Char.Map�T�[�o��Windows�̃T�[�r�X�Ƃ��ēo�^���܂����B
@echo ����Q���̎����N����ON�ɂ���ꍇ��Windows�́u�Ǘ��c�[���v����u�T�[�r�X�v��I�����Ă��������B
goto end

:error
@echo �����ϐ��@set athenasrv=�@�ɃT�[�o�v���O�����̃p�X���ݒ肳��Ă��Ȃ���Ԃł��B
@echo ���ϐ��ݒ��@set athenasrv="c:\athena"�@�Ȃ�
:end
pause
