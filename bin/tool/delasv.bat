@echo off
@echo ��Athena���T�[�r�X���疕�����܂�

set reskit=
set svname=
if "%svname%"=="" goto error3
if "%reskit%"=="" goto error2
if not exist %reskit%sc.exe goto error1

%reskit%sc.exe %svname% Delete AthenaLogin
%reskit%sc.exe %svname% Delete AthenaChar
%reskit%sc.exe %svname% Delete AthenaMap

@echo ��Login.Char.Map�T�[�o��Windows�̃T�[�r�X����̉��������݂܂����B
@echo ���G���[���������Ă���ꍇ�́A���炩���߃T�[�r�X���~���Ă���ēx���s���Ă��������B
goto end

:error3
@echo ���T�[�o�������ݒ�ł��AWindows�̏ꍇ�̓R���s���[�^�[�����T�[�o�[���ɂȂ�܂�
@echo �R���g���[���p�l�����V�X�e���̃l�b�g���[�NID�ɕ\�������R���s���[�^����
@echo ���ϐ��ݒ�� set svname="\\myserver"�̂悤�ɐݒ肵�Ă��������A\\�͕K�{�ł��B
goto end

:error2
@echo ���G���[���������܂����B���\�[�X�L�b�g�̃p�X���ݒ肳��Ă��܂���
@echo ���ϐ��ݒ�� set reskit="C:\Program Files\Resource Kit\"
goto end

:error1
@echo ���G���[���������܂����B���\�[�X�L�b�h�񓱓��ɂ��uSC.EXE�v�̌������邢��
@echo ���\�[�X�L�b�g�Ƀp�X���ʂ��Ă��Ȃ��\��������܂��B
@echo ���\�[�X�L�b�g�ݒ�̊m�F�������̓��\�[�X�L�b�g�𓱓����Ă�������
goto end

:error
:end
pause
