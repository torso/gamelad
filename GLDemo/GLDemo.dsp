# Microsoft Developer Studio Project File - Name="GLDemo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=GLDemo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GLDemo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GLDemo.mak" CFG="GLDemo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GLDemo - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "GLDemo - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "GLDemo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "GLDemo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds="C:\TV-Spel\Spec\Gb\Rgbds\xLink95.exe" -tg "C:\Torbjrn\Cpp\Game Lad\GLDemo\GLDemo.lnk"	"C:\TV-Spel\Spec\Gb\Rgbds\Rgbfix95.exe" -v "C:\Torbjrn\Cpp\Game Lad\GLDemo\Debug\GLDemo.gb"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "GLDemo - Win32 Release"
# Name "GLDemo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "asm"
# Begin Source File

SOURCE=.\GLDemo.asm

!IF  "$(CFG)" == "GLDemo - Win32 Release"

# Begin Custom Build
WkspDir=.
InputPath=.\GLDemo.asm
InputName=GLDemo

"$(WkspDir)\GLDemo\Release\$(InputName).gb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(WkspDir)\Glasm\Release\Glasm.exe" "$(WkspDir)\GLDemo\$(InputName).asm" "$(WkspDir)\GLDemo\Release\$(InputName).gb"

# End Custom Build

!ELSEIF  "$(CFG)" == "GLDemo - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
WkspDir=.
InputPath=.\GLDemo.asm
InputName=GLDemo

"$(WkspDir)\GLDemo\Debug\$(InputName).o" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"C:\TV-Spel\Spec\Gb\Rgbds\Rgbasm95.exe" -o"$(WkspDir)\GLDemo\Debug\$(InputName).o" "$(WkspDir)\GLDemo\$(InputName).asm"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GLDemo.lnk
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "inc"
# Begin Source File

SOURCE=.\Hardware.inc
# End Source File
# End Group
# End Target
# End Project
