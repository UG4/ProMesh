;--------------------------------

; The name of the installer
Name ProMesh4

; The file to write
OutFile "ProMesh4-Installer.exe"

; The default installation directory
InstallDir $PROGRAMFILES64\ProMesh4

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "SOFTWARE\G-CSC\ProMesh4" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

Icon "..\..\deployment\data\ProMeshIcon.ico"

LicenseBkColor /windows
LicenseData "..\..\LICENSE"

XPStyle on
ManifestSupportedOS all

;--------------------------------

; Pages

Page license
Page directory
Page components
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section ProMesh4 (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\..\win64-artifacts\ProMesh4.exe"
  File "..\..\win64-artifacts\\*.dll"

  SetOutPath $INSTDIR\platforms
  File "win64-artifacts\platforms\*.dll"

  SetOutPath $INSTDIR\tools
  File "win64-artifacts\tools\*.exe"
  File "win64-artifacts\tools\*LICENSE"

  SetOutPath $INSTDIR\translations
  File "win64-artifacts\translations\*"

  SetOutPath $INSTDIR\imageformats
  File "win64-artifacts\imageformats\*"

  SetOutPath $INSTDIR\iconengines
  File "win64-artifacts\iconengines\*"

  SetOutPath $INSTDIR
  

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\G-CSC\ProMesh4 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\G-CSC\ProMesh4" "DisplayName" "ProMesh4"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\G-CSC\ProMesh4" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\G-CSC\ProMesh4" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\G-CSC\ProMesh4" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Programs Shortcut"
  CreateDirectory "$SMPROGRAMS\ProMesh4"
  CreateShortcut "$SMPROGRAMS\ProMesh4\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$SMPROGRAMS\ProMesh4\ProMesh4.lnk" "$INSTDIR\ProMesh4.exe" "" "$INSTDIR\ProMesh4.exe" 0
SectionEnd

Section "Start Menu Shortcut"
  CreateShortcut "$STARTMENU\ProMesh4.lnk" "$INSTDIR\ProMesh4.exe" "" "$INSTDIR\ProMesh4.exe" 0
SectionEnd

Section "Desktop Shortcut"
  CreateShortcut "$DESKTOP\ProMesh4.lnk" "$INSTDIR\ProMesh4.exe" "" "$INSTDIR\ProMesh4.exe" 0
SectionEnd


;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\G-CSC\ProMesh4"
  DeleteRegKey HKLM SOFTWARE\G-CSC\ProMesh4

  ; Remove files and uninstaller
  Delete $INSTDIR\ProMesh4.exe
  Delete $INSTDIR\*.dll
  RMDir /r /REBOOTOK $INSTDIR\platforms
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\ProMesh4\*.*"
  Delete "$STARTMENU\ProMesh4.lnk"
  Delete "$DESKTOP\ProMesh4.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\ProMesh4"
  RMDir "$INSTDIR"

SectionEnd
