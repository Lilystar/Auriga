# Microsoft Developer Studio Project File - Name="login_sql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** �ҏW���Ȃ��ł������� **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=login_sql - Win32 Debug
!MESSAGE ����͗L����Ҳ�̧�قł͂���܂���B ������ۼު�Ă�����ނ��邽�߂ɂ� NMAKE ���g�p���Ă��������B
!MESSAGE [Ҳ�̧�ق̴���߰�] ����ނ��g�p���Ď��s���Ă�������
!MESSAGE 
!MESSAGE NMAKE /f "login_sql.mak".
!MESSAGE 
!MESSAGE NMAKE �̎��s���ɍ\�����w��ł��܂�
!MESSAGE ����� ײݏ��ϸۂ̐ݒ���`���܂��B��:
!MESSAGE 
!MESSAGE NMAKE /f "login_sql.mak" CFG="login_sql - Win32 Debug"
!MESSAGE 
!MESSAGE �I���\������� Ӱ��:
!MESSAGE 
!MESSAGE "login_sql - Win32 Release" ("Win32 (x86) Console Application" �p)
!MESSAGE "login_sql - Win32 Debug" ("Win32 (x86) Console Application" �p)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "login_sql - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_sql"
# PROP Intermediate_Dir "Release_sql"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gi /GX /O2 /I "../common/" /D PACKETVER=8 /D "NEW_006b" /D FD_SETSIZE=4096 /D "_CRT_SECURE_NO_DEPRECATE" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /incremental:yes /map:"../../login-server.map" /machine:I386 /out:"../../login-server.exe"

!ELSEIF  "$(CFG)" == "login_sql - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_sql"
# PROP Intermediate_Dir "Debug_sql"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /Gi /GX /ZI /Od /I "../common/" /D PACKETVER=8 /D "NEW_006b" /D FD_SETSIZE=4096 /D "_CRT_SECURE_NO_DEPRECATE" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /map:"../../login-server.map" /debug /machine:I386 /out:"../../login-server.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "login_sql - Win32 Release"
# Name "login_sql - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\common\core.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\db.c
# End Source File
# Begin Source File

SOURCE=..\common\graph.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\grfio.c
# End Source File
# Begin Source File

SOURCE=..\common\httpd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\lock.c
# End Source File
# Begin Source File

SOURCE=..\..\src\login\login.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\malloc.c
# End Source File
# Begin Source File

SOURCE=..\common\md5calc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\nullpo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\socket.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\timer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\utils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\common\core.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\db.h
# End Source File
# Begin Source File

SOURCE=..\common\graph.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\grfio.h
# End Source File
# Begin Source File

SOURCE=..\common\httpd.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\lock.h
# End Source File
# Begin Source File

SOURCE=..\..\src\login\login.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\malloc.h
# End Source File
# Begin Source File

SOURCE=..\common\md5calc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\mmo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\nullpo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\utils.h
# End Source File
# End Group
# End Target
# End Project
