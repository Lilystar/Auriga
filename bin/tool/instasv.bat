@echo off
@echo ��Auriga���T�[�r�X�Ƃ��ēo�^���܂��B

@rem -----�ݒ蕔---------
set aurigasrv="c:\auriga"
@rem Auriga�̃o�C�i���̂���ꏊ���w�肵�܂��B
@rem --------------------
if "%aurigasrv%"=="" goto error

sc.exe Create AurigaLogin binPath= %aurigasrv%\login-server.exe
sc.exe Create AurigaChar binPath= %aurigasrv%\char-server.exe
sc.exe Create AurigaMap binPath= %aurigasrv%\map-server.exe

@echo ��Login.Char.Map�T�[�o��Windows�̃T�[�r�X�Ƃ��ēo�^���܂����B
@echo ����Q���̎����N����ON�ɂ���ꍇ��Windows�́u�Ǘ��c�[���v����u�T�[�r�X�v��I�����Ă��������B
goto end

:error
@echo �����ϐ��@set aurigasrv=�@�ɃT�[�o�v���O�����̃p�X���ݒ肳��Ă��Ȃ���Ԃł��B
@echo ���ϐ��ݒ��@set aurigasrv="c:\auriga"�@�Ȃ�
:end
pause
