!include "MUI2.nsh"
icon "application.ico"
Name SafejumperPlus
OutFile ..\safejumperplus.exe ; NsiDecompiler: generated value!
InstallColors 00FF00 000000
InstallDir 'C:\Program Files\Safejumper Plus'
Page directory "" "" ""
Page instfiles "" "" ""
Page custom ""  ""

!define VERSION         "5.1"
!define BUILD             "106"

Section main

SectionIn RO
       CreateDirectory $INSTDIR
       SetOutPath $INSTDIR
       # Stop and uninstall service in case it's running
	   nsExec::Exec 'taskkill /f /im "Safejumper Plus.exe"'
	   Pop $0
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -t'
       Pop $0
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -u'
       Pop $0
       nsExec::Exec 'taskkill /f /im openvpn.exe'
       Pop $0
       nsExec::Exec 'taskkill /f /im safejumperplusservice.exe'
       Pop $0
       
       File  libeay32.dll
       File  vcredist_x86.exe
       File  ssleay32.dll
       File  application.ico
       File  "Safejumper Plus.exe"
       File  safejumperplusservice.exe
       File  Qt5Core.dll
       File  Qt5Gui.dll
       File  Qt5Network.dll
       File  Qt5Svg.dll
       File  Qt5Widgets.dll
       File  Qt5Xml.dll
	   File  Qt5Qml.dll
	   File  Qt5Quick.dll
	   File  Qt5QuickControls2.dll
	   File  Qt5QuickTemplates2.dll
	   File  ..\src\languages\gui_zh.qm
	   File  ..\src\languages\gui_en.qm
       SetOutPath $INSTDIR\platforms
       File  platforms\qwindows.dll
	   SetOutPath $INSTDIR\imageformats
	   File imageformats\qsvg.dll
	   SetOutPath $INSTDIR\QtGraphicalEffects
	   File  /r QtGraphicalEffects\*
	   SetOutPath $INSTDIR\QtQuick
	   File  /r QtQuick\*
	   SetOutPath $INSTDIR\QtQuick.2
	   File  /r QtQuick.2\*
       SetOutPath $TEMP
       File  openvpn-proxysh.exe
       Push $0
       ExecWait '$OUTDIR\openvpn-proxysh.exe /S /SELECT_PATH=0 /SELECT_OPENVPNGUI=0 /SELECT_SHORTCUTS=0 /D=$INSTDIR\OpenVPN' $0
       IfErrors Label_0x19 Label_0x1A

  Label_0x19:
       MessageBox  MB_OK 'OpenVPN network card driver installation fails. Reinstall, please.' /SD IDOK

  Label_0x1A:
       Delete  $OUTDIR\openvpn-proxysh.exe
       File  python-2.7.11.msi
       Push $0
       ExecWait 'msiexec /qn /i $OUTDIR\python-2.7.11.msi ALLUSERS=1 TARGETDIR=c:\python27' $0
       IfErrors Label_0x1F Label_0x20

  Label_0x1F:
       MessageBox  MB_OK 'Cannot install Python 2.7.11' /SD IDOK

  Label_0x20:
       Delete  $OUTDIR\python-2.7.11.msi
       File  wheel-pip\pip-8.1.2-py2.py3-none-any.whl
       Push $0
       ExecWait 'c:\python27\pythonw.exe -m pip install $OUTDIR\pip-8.1.2-py2.py3-none-any.whl' $0
       IfErrors Label_0x25 Label_0x26

  Label_0x25:
       MessageBox  MB_OK 'Cannot install pip-8.1.2' /SD IDOK

  Label_0x26:
       Delete  $OUTDIR\pip-8.1.2-py2.py3-none-any.whl

       File  wheel\argparse-1.4.0-py2.py3-none-any.whl
       File  wheel\obfsproxy-0.2.13-py2-none-any.whl
       File  wheel\pycrypto-2.6.1-cp27-cp27m-win32.whl
       File  wheel\pyptlib-0.0.6-py2-none-any.whl
       File  wheel\PyYAML-3.11-cp27-cp27m-win32.whl
       File  wheel\setuptools-23.1.0-py2.py3-none-any.whl
       File  wheel\Twisted-16.2.0-cp27-cp27m-win32.whl
       File  wheel\zope.interface-4.2.0-cp27-cp27m-win32.whl
       Push $0
       ExecWait 'c:\python27\pythonw.exe -m pip install $OUTDIR\argparse-1.4.0-py2.py3-none-any.whl $OUTDIR\obfsproxy-0.2.13-py2-none-any.whl $OUTDIR\pycrypto-2.6.1-cp27-cp27m-win32.whl $OUTDIR\pyptlib-0.0.6-py2-none-any.whl $OUTDIR\PyYAML-3.11-cp27-cp27m-win32.whl $OUTDIR\setuptools-23.1.0-py2.py3-none-any.whl $OUTDIR\Twisted-16.2.0-cp27-cp27m-win32.whl $OUTDIR\zope.interface-4.2.0-cp27-cp27m-win32.whl' $0
       IfErrors Label_0x29 Label_0x30

  Label_0x29:
       MessageBox  MB_OK 'Cannot install pip-8.1.2' /SD IDOK

  Label_0x30:
        ExecWait '$INSTDIR\vcredist_x86.exe /install /quiet /norestart'

       ; Stop and unnistall in case a previous build is installed
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -t'
       Pop $0
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -u'
       Pop $0
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -i'
       Pop $0
       nsExec::Exec '$INSTDIR\safejumperplusservice.exe -s'
       Pop $0

    # set DACL for todylserver
    nsExec::Exec 'sc sdset "SafejumperPlus" D:(A;;CCLCSWRPWPDTLOCRRC;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)(A;;CCLCSWLOCRRCRPWP;;;IU)(A;;CCLCSWLOCRRC;;;SU)'

    # create the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    # create a shortcut named "new shortcut" in the start menu programs directory
    # point the new shortcut at the program uninstaller

	SetOutPath $INSTDIR
    CreateShortCut  "$DESKTOP\Safejumper Plus.lnk" "$INSTDIR\Safejumper Plus.exe"
    CreateDirectory "$SMPROGRAMS\Safejumper Plus"
    CreateShortCut  "$SMPROGRAMS\Safejumper Plus\Safejumper Plus.lnk" "$INSTDIR\Safejumper Plus.exe"
    CreateShortCut  "$SMPROGRAMS\Safejumper Plus\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    # Add uninstaller to registry for easy uninstallation
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "DisplayName" "SafejumperPlus"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "DisplayIcon" "$INSTDIR\application.ico"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "Publisher" "Proxy.sh"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "DisplayVersion" "${VERSION} build ${BUILD}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus" \
            "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
SectionEnd

# uninstaller section start
Section "uninstall"

    Delete  "$DESKTOP\Safejumper Plus.lnk"
    Delete  "$SMPROGRAMS\Safejumper Plus\Safejumper Plus.lnk"
    Delete  "$SMPROGRAMS\Safejumper Plus\Uninstall.lnk"
    ExecWait '$INSTDIR\OpenVPN\Uninstall.exe /S'  $0
    RMDir /r $INSTDIR\*.*
    RMDir $INSTDIR
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Safejumper Plus"

# uninstaller section end
SectionEnd
