# Microsoft Developer Studio Project File - Name="map_sql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** �ҏW���Ȃ��ł������� **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=map_sql - Win32 Debug
!MESSAGE ����͗L����Ҳ�̧�قł͂���܂���B ������ۼު�Ă�����ނ��邽�߂ɂ� NMAKE ���g�p���Ă��������B
!MESSAGE [Ҳ�̧�ق̴���߰�] ����ނ��g�p���Ď��s���Ă�������
!MESSAGE 
!MESSAGE NMAKE /f "map_sql.mak".
!MESSAGE 
!MESSAGE NMAKE �̎��s���ɍ\�����w��ł��܂�
!MESSAGE ����� ײݏ��ϸۂ̐ݒ���`���܂��B��:
!MESSAGE 
!MESSAGE NMAKE /f "map_sql.mak" CFG="map_sql - Win32 Debug"
!MESSAGE 
!MESSAGE �I���\������� Ӱ��:
!MESSAGE 
!MESSAGE "map_sql - Win32 Release" ("Win32 (x86) Console Application" �p)
!MESSAGE "map_sql - Win32 Debug" ("Win32 (x86) Console Application" �p)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "map_sql - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WINDOWS" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gi /GX /O2 /I "../common/" /D PACKETVER=20120925 /D "NEW_006b" /D FD_SETSIZE=4096 /D "_CRT_SECURE_NO_DEPRECATE" /D "WINDOWS" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_HTTPD" /D "NO_HTTPD_CGI" /D "PRE_RENEWAL" /D "DYNAMIC_SC_DATA" /FD /c /wd4819
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /incremental:yes /map:"../../map-server.map" /machine:I386 /out:"../../map-server.exe"

!ELSEIF  "$(CFG)" == "map_sql - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WINDOWS" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /Gi /GX /ZI /Od /I "../common/" /D PACKETVER=20120925 /D "NEW_006b" /D FD_SETSIZE=4096 /D "_CRT_SECURE_NO_DEPRECATE" /D "WINDOWS" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_HTTPD" /D "NO_HTTPD_CGI" /D "PRE_RENEWAL" /D "DYNAMIC_SC_DATA" /FR /FD /GZ /c /wd4819
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:console /map:"../../map-server.map" /debug /machine:I386 /out:"../../map-server.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "map_sql - Win32 Release"
# Name "map_sql - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\map\atcommand.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\battle.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\chat.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\chrif.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\clif.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\core.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\date.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\db.c
# End Source File
# Begin Source File

SOURCE=.\friend.c
# End Source File
# Begin Source File

SOURCE=..\common\graph.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\grfio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\guild.c
# End Source File
# Begin Source File

SOURCE=..\common\httpd.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\intif.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\itemdb.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\lock.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\malloc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\map.c
# End Source File
# Begin Source File

SOURCE=..\common\md5calc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\mob.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\npc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\nullpo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\party.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\path.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\pc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\pet.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\ranking.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\script.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\skill.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\socket.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\status.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\storage.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\timer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\trade.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\unit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\vending.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\homun.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\mail.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\merc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\quest.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\booking.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\buyingstore.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\elem.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\memorial.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\msg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\bank.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\bonus.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\extra.c
# End Source File
# Begin Source File

SOURCE=..\..\src\map\sql\mapreg_sql.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\version.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\common\sqldbs.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\map\atcommand.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\battle.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\chat.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\chrif.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\clif.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\core.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\date.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\db.h
# End Source File
# Begin Source File

SOURCE=.\friend.h
# End Source File
# Begin Source File

SOURCE=..\common\graph.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\grfio.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\guild.h
# End Source File
# Begin Source File

SOURCE=..\common\httpd.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\intif.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\itemdb.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\lock.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\malloc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\map.h
# End Source File
# Begin Source File

SOURCE=..\common\md5calc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\mmo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\mob.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\npc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\nullpo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\party.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\pc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\pet.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\ranking.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\script.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\skill.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\socket.h
# End Source File
# Begin Source File

SOURCE=.\status.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\storage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\trade.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\unit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\vending.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\homun.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\mail.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\merc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\quest.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\booking.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\buyingstore.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\elem.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\memorial.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\msg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\bank.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\bonus.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\extra.h
# End Source File
# Begin Source File

SOURCE=..\..\src\map\sql\mapreg_sql.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\src\common\sqldbs.h
# End Source File
# End Group
# End Target
# End Project
