:: Manalink Launcher v0.1
:: written for the CCGHQ Manalink Forums
:: by CirothUngol July 2, 2015

@ECHO OFF
SETLOCAL EnableExtensions DisableDelayedExpansion
FOR %%A IN ("%~dp0") DO (
	SET "mlDir=%%~sAProgram"
	SET "modDir=%%~sAMods"
)
SET "modTmp=%modDir%\_tmp"
SET "modArt=%modDir%\Art"
SET "modSound=%modDir%\Sound"
SET "modRogues=%modDir%\Rogues"
SET "modPlayDeck=%modDir%\PlayDeck"
::SET "modPlayDeck=%mlDir%\PlayDeck"
SET "undoArt=%modArt%\_undo"
SET "undoPlayDeck=%modPlayDeck%\_backup"
SET "defaultArt=Default Sonic 2014"
SET "defaultSound=Default Microprose"
SET "defaultRogues=Classic CirothUngol Ridiculous"

SET "SELF=%~n0"
SET "SELFDIR=%~dps0"
SET "SELFDIR=%SELFDIR:~0,-1%"
SET "PATH=%modDir%\Util;%PATH%"
SET "PC=BG.EXE Print"
IF "%~1" NEQ "" (
	IF /I "%~1" EQU "/nc" (
		CALL :nameCorrect %2 %3 %4
	) ELSE IF EXIST "%~1\" CALL :Zip %1
	GOTO :Stop
)
TITLE %SELF%: Initializing
SET "dirDisplay="
SET "RoguesOnly="
SET "firstRun="
SET "ReadOnly="
SET "modCnt=0"
SET "resCnt=0"
SET "mlVer="
SET "kbd=0"
BG Color 0F
::check for old Sound_Toggler folders.
IF EXIST "%mlDir%\_Sound\" RENAME "%mlDir%\_Sound" "_Sound_OFF" >NUL 2>&1
IF EXIST "%mlDir%\_StatWin\" RENAME "%mlDir%\_StatWin" "_StatWin_OFF" >NUL 2>&1
IF EXIST "%mlDir%\_DuelSounds\" RENAME "%mlDir%\_DuelSounds" "_DuelSounds_OFF" >NUL 2>&1
:: is current folder writable?
REM.>Delete.Me
IF NOT EXIST Delete.Me SET "ReadOnly=1"
DEL /F /Q /A Delete.Me >NUL 2>&1
IF EXIST Delete.Me SET "ReadOnly=1"
IF DEFINED ReadOnly (
	%PC% F "\n     The current folder appears to be unwriteable.\n"
	%PC% F "     Please check permissions or move Manalink to a different folder.\n\n"
	%PC% A "     Press any key to continue..."
	BG Kbd
	GOTO :Stop
)
::check for firstRun and install if needed.
IF NOT EXIST "%mlDir%\Rogues.csv" (
	CALL :Install
	IF ERRORLEVEL 1 GOTO :Stop
)
FOR %%A IN ("%mlDir%\Magic.exe") DO FOR /F "tokens=1" %%B IN ("%%~tA") DO SET mlVer= -" A " %%B

:Menu - begin Main Display
TITLE %SELF%: Main Menu Options
CLS
::set Sound Toggler
SET "stFlag="
IF EXIST "%mlDir%\DuelSounds\Artifact.wav" (
	SET ts=Sounds/Animations are" A " ON" F ", Toggle them" C " OFF"
) ELSE IF EXIST "%mlDir%\_DuelSounds_OFF\Artifact.wav" (
	SET ts=Sounds/Animations are" C " OFF" F ", Toggle them" A " ON"
) ELSE SET stFlag=1 & SET ts=Sounds/Animations are" C " NOT" F " Installed"
::set counts for menu options
::SET ddCnt=0
::FOR /R "%mlDir%" %%A IN ("Draft - Human*.dck") DO SET /A ddCnt+=1
::FOR /R "%mlDir%" %%A IN ("Draft Opponent*.dck") DO SET /A ddCnt+=1
::display Menu
%PC% F "\n                               Manalink Launcher\n"
%PC% F "                               Main Menu Options\n\n"
%PC% A "     1)" F " Play Manalink 3.0%mlVer%\n"
%PC% A "     2)" F " %ts%\n"
%PC% A "     3)" F " Delete Draft Decks\n"
%PC% A "     4)" F " Start the PlayDeck Analyser\n"
%PC% A "     5)" F " View the ReadMe.txt\n"
%PC% A "     6)" F " Go to the Mods Menu\n"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "6" F "):"
:Input
CALL :bgInput kbd
IF %kbd% EQU 27 GOTO :Stop
SET /A kbd-=48
IF %kbd% LSS 1 GOTO :Input
IF %kbd% GTR 6 GOTO :Input
%PC% F "%kbd%"
IF %kbd% EQU 2 IF DEFINED stFlag SET "kbd=6"
IF %kbd% EQU 1 (
	CD /D "%mlDir%"
	START Magic.exe
	GOTO :Stop
) ELSE IF %kbd% EQU 2 (
	CALL :ToggleSounds
) ELSE IF %kbd% EQU 3 (
	DEL "%mlDir%\Draft - Human*.dck" /F /Q /A >NUL 2>&1
	DEL "%mlDir%\Draft Opponent*.dck" /F /Q /A >NUL 2>&1
	DEL "%mlDir%\PlayDeck\Draft - Human*.dck" /F /Q /A >NUL 2>&1
	DEL "%mlDir%\PlayDeck\Draft Opponent*.dck" /F /Q /A >NUL 2>&1
) ELSE IF %kbd% EQU 4 (
	IF EXIST "%SELFDIR%\PlayDeckAnalyser\PlayDeckAnalyser.exe" (
		START %SELFDIR%\PlayDeckAnalyser\PlayDeckAnalyser.exe
	)
) ELSE IF %kbd% EQU 5 (
	START %mlDir%\ReadMe.txt
) ELSE IF %kbd% EQU 6 (
	GOTO :mInit
) ELSE ECHO WTF?!
GOTO :Menu

:mInit
%PC% F "\n\n     Initializing, please wait..."
SET /A amCnt=0,smCnt=0,rmCnt=0,pdCnt=0,pdTot=0
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modArt%" /AD-S /B 2^>NUL`) DO IF /I "%%~nxA" NEQ "_undo" SET /A amCnt+=1
FOR %%A IN (7z zip) DO FOR /F "usebackq tokens=*" %%B IN (`DIR "%modArt%\*.%%A" /A-D-S /B 2^>NUL`) DO SET /A amCnt+=1
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modSound%" /AD-S /B 2^>NUL`) DO SET /A smCnt+=1
FOR %%A IN (7z zip) DO FOR /F "usebackq tokens=*" %%B IN (`DIR "%modSound%\*.%%A" /A-D-S /B 2^>NUL`) DO SET /A smCnt+=1
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modRogues%" /AD-S /B 2^>NUL`) DO SET /A rmCnt+=1
FOR %%A IN (7z zip) DO FOR /F "usebackq tokens=*" %%B IN (`DIR "%modRogues%\*.%%A" /A-D-S /B 2^>NUL`) DO SET /A rmCnt+=1
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modPlayDeck%" /AD-S /B /S 2^>NUL`) DO IF EXIST "%%A\*.dck" SET /A pdCnt+=1
FOR /F "usebackq tokens=1" %%A IN (`DIR "%modPlayDeck%\*.dck" /-C /S 2^>NUL ^| FIND /I "File(s)"`) DO SET pdTot=%%A
:mMenu - begin Mod Menu Display
TITLE %SELF%: Mod Menu Options
CLS
RD "%modTmp%" /S /Q >NUL 2>&1
DEL "%modTmp%.*" /F /Q /A >NUL 2>&1
FOR %%A IN (7z zip) DO IF EXIST "%modSelect%\%Mod%.%%A" CALL :killArc "%modSelect%\%Mod%"
SET "last=4"
::display mMenu
%PC% F "\n                               Manalink Launcher\n"
%PC% F "                               Mods Menu Options\n\n"
%PC% A "     1)" F " Install Art Mods   (" A "%amCnt%" F " Mods found)\n"
%PC% A "     2)" F " Install Sound Mods (" A "%smCnt%" F " Mods found)\n"
%PC% A "     3)" F " Install Rogues     (" A "%rmCnt%" F " Mods found)\n"
%PC% A "     4)" F " Install PlayDecks  (" A "%pdCnt%" F " Mods," A " %pdTot%" F " PlayDecks found)\n"
IF EXIST "%modDir%\Text.res" (
	%PC% A "     5)" F " Check/Install" A " .\\Mods\\Text.res\n"
	SET "last=5"
)
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "%last%" F "):"
:mInput
CALL :bgInput kbd
IF %kbd% EQU 27 GOTO :Menu
SET /A kbd-=48
IF %kbd% LSS 1 GOTO :mInput
IF %kbd% GTR %last% GOTO :mInput
%PC% F "%kbd%"
IF %kbd% EQU 1 (
	CALL :ArtMod
) ELSE IF %kbd% EQU 2 (
	CALL :SoundMod
) ELSE IF %kbd% EQU 3 (
	CALL :RoguesMod
) ELSE IF %kbd% EQU 4 (
	CALL :PlayDeckMod
	SET "dirDisplay="
) ELSE IF %kbd% EQU 5 (
	CALL :arCheck
) ELSE ECHO WTF?!
GOTO :mMenu

:Stop - clean up and exit
RD "%undoArt%" >NUL 2>&1
RD "%undoPlayDeck%" >NUL 2>&1
RD "%modTmp%" /S /Q >NUL 2>&1
DEL "%modTmp%.*" /F /Q /A >NUL 2>&1
ENDLOCAL
COLOR 07
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Install - setup default Manalink resources
CLS
TITLE %SELF%: Installing Manalink
%PC% F "\n                               Manalink Launcher\n"
%PC% F "                             Automatic Installation\n\n"
%PC% D "     Manalink needs the following Mods installed:\n\n
%PC% F "        Art Mod:" A "%defaultArt%\n"
%PC% F "      Sound Mod:" A "%defaultSound%\n"
%PC% F "     Rogues Mod:" A "%defaultRogues%\n\n"
%PC% F "     Press any key to continue setup, or" C " Esc" F " to exit."
CALL :bgInput kbd
IF %kbd% EQU 27 EXIT /B 1
SET "firstRun= - Installing Manalink"
::install default Mods
SET "Mod=%defaultArt%"
CALL :ArtMod
FOR %%A IN (7z zip) DO IF EXIST "%modSelect%\%Mod%.%%A" CALL :killArc "%modSelect%\%Mod%"
SET "Mod=%defaultSound%"
CALL :SoundMod
FOR %%A IN (7z zip) DO IF EXIST "%modSelect%\%Mod%.%%A" CALL :killArc "%modSelect%\%Mod%"
SET "Mod=%defaultRogues%"
CALL :RoguesMod
FOR %%A IN (7z zip) DO IF EXIST "%modSelect%\%Mod%.%%A" CALL :killArc "%modSelect%\%Mod%"
SET "firstRun="
FOR /R "%modRogues%" %%A IN (CirothUngol.pic) DO COPY /Y "%%~dpsA%%~nxA" "%mlDir%\PlayFace\" >NUL 2>&1
::unlock challenge cards
MD "%mlDir%\Faces" >NUL 2>&1
FOR %%A IN (annoying begin bondage buuurp christine clean diane dylan) DO REM.>"%mlDir%\Faces\%%A.jat"
FOR %%A IN (goodwork greg hardy headache jabber james katie keeks magic) DO REM.>"%mlDir%\Faces\%%A.jat"
FOR %%A IN (master me monna mtg nerds nuts oath onna owen patch poeia) DO REM.>"%mlDir%\Faces\%%A.jat"
FOR %%A IN (quitter roskopp routish ryan shardy shiny tricia willy wired wonka) DO REM.>"%mlDir%\Faces\%%A.jat"
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:arCheck
SETLOCAL EnableDelayedExpansion
SET /A resCnt=0,atCnt=0
::check for minimum Text.res totals
FOR /F "usebackq" %%A IN (`FIND /C "@" ^<"!modDir!\Text.res"`) DO SET "atCnt=%%A"
FOR /F "usebackq" %%A IN (`FIND /V /C "" ^<"!modDir!\Text.res"`) DO SET "resCnt=%%A"
IF %resCnt% LSS 3 GOTO :arStop
IF %atCnt% LSS 1 GOTO :arStop
BG Locate 22 0
%PC% F "     ...checking for missing content in Text.res"
::read Mod\Text.res into pseudo-array and check for missing contents
<"!modDir!\Text.res" (FOR /L %%A IN (1,1,%resCnt%) DO SET /P "res[%%A]=")
CALL :ar1
IF ERRORLEVEL 1 GOTO :arStop
TITLE %~n0: Update Text.res?
BG Locate 22 0
%PC% A "     .\\Mods\\Text.res" F " contains new content for" A " .\\Program\\Text.res.\n"
%PC% F "     Would you like to update the file? [" A "Y" F "/" A "N" F "]:"
CALL :bgInput kbd
IF %kbd% NEQ 89 IF %kbd% NEQ 121 GOTO :arStop
%PC% A "Y"
FOR /L %%A IN (1,1,%resCnt%) DO SET "res[%%A]="
CALL :addRes
GOTO :arStop
:addRes
SETLOCAL EnableDelayedExpansion
::create res.tmp, count lines, read into pseudo-array
MD "!modTmp!" >NUL 2>&1
TYPE "!mlDir!\Text.res" "!modDir!\Text.res" 1>"!modTmp!\res.tmp" 2>NUL
FOR /F "usebackq" %%A IN (`FIND /V /C "" ^<"!modTmp!\res.tmp"`) DO SET "resCnt=%%A"
<"!modTmp!\res.tmp" (FOR /L %%A IN (1,1,%resCnt%) DO SET /P "res[%%A]=")
DEL /F /Q /A "!modTmp!\res.tmp" >NUL 2>&1
::split res.tmp into sections
SET "atFlag=-1"
FOR /L %%A IN (1,1,%resCnt%) DO (
	IF !atFlag! GTR 0 (
		IF %%A GTR !atFlag! (
			SET "atFlag=-1"
		) ELSE ECHO(!res[%%A]!>>"!modTmp!\!resName!"
	)
	IF !atFlag! EQU 0 (
		SET /A atFlag=%%A + !res[%%A]!
		ECHO(!res[%%A]!>>"!modTmp!\!resName!"
	)
	IF "!res[%%A]:~0,1!" EQU "@" (
		SET "atFlag=0"
		SET "resName=!res[%%A]!"
		TITLE %~n0: !resName!
		ECHO(!res[%%A]!>"!modTmp!\!resName!"
	)
)
::rebuild .\Program\Text.res and remove temp files
SET "atFlag="
REM.>"!mlDir!\Text.res"
::this line will order the sections exactly as in the original Text.res
::FOR /F "usebackq tokens=*" %%A IN (`FIND "@" ^<"!mlDir!\Text.res" 2^>NUL`) DO (
FOR /F "usebackq tokens=*" %%A IN (`DIR /A-D /B /ON "!modTmp!\@*" 2^>NUL`) DO (
	IF DEFINED atFlag ECHO(>>"!mlDir!\Text.res"
	TYPE "!modTmp!\%%A">>"!mlDir!\Text.res"
	SET "atFlag=1"
)
RD "!modTmp!" /S /Q >NUL 2>&1
:arStop
ENDLOCAL
EXIT /B 0
:ar1
::test Text.res
SET atFlag=-1
FOR /L %%A IN (1,1,%resCnt%) DO (
	FOR /F "usebackq" %%B IN (`FIND /I /C "!res[%%A]!" ^<"!mlDir!\Text.res" 2^>NUL`) DO (
		IF %%B LSS 1 IF DEFINED res[%%A] IF !atFlag! NEQ 0 EXIT /B 0
	)
	IF !atFlag! GTR 0 IF "%%A" GTR "!atFlag!" SET "atFlag=-1"
	IF !atFlag! EQU 0 SET /A atFlag=%%A + !res[%%A]!
	IF "!res[%%A]:~0,1!" EQU "@" SET "atFlag=0" & TITLE %~n0: !res[%%A]!
)
EXIT /B 1

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:ToggleSounds - allows switching Sounds and Animations ON and OFF
PUSHD "%mlDir%"
IF EXIST ".\_DuelSounds_OFF\Artifact.wav" (
	REM Toggle Sounds/Animations ON
	RD Sound /S /Q >NUL 2>&1
	RD DuelSounds /S /Q >NUL 2>&1
	RENAME _Sound_OFF Sound >NUL 2>&1
	RENAME _DuelSounds_OFF DuelSounds >NUL 2>&1
	MOVE /Y .\_StatWin_OFF\*.AVI .\StatWin\ >NUL 2>&1
	RD _StatWin_OFF /S /Q >NUL 2>&1
) ELSE (
	REM Toggle Sounds/Animations OFF
	RENAME DuelSounds _DuelSounds_OFF >NUL 2>&1
	RENAME Sound _Sound_OFF >NUL 2>&1
	IF EXIST .\_Sound_OFF\DAMBLOOP.WAV (
		MD Sound >NUL 2>&1
		COPY .\_Sound_OFF\*WALK*.WAV .\Sound\ /Y  >NUL 2>&1
		COPY .\_Sound_OFF\DAMBLOOP.WAV .\Sound\ /Y  >NUL 2>&1
	)
	IF EXIST .\StatWin\B_AMG.AVI (
		MD _StatWin_OFF
		MOVE /Y .\StatWin\*.AVI .\_StatWin_OFF\  >NUL 2>&1
	)
)
POPD
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:ArtMod - select and install Art Mods
SET "modSelect=%modArt%"
SET "modDisplay=                                 Art Mod Menu"
:am1
TITLE %SELF%: Art Mod Menu%firstRun%
CALL :Mod
IF ERRORLEVEL 1 EXIT /B 0
IF /I "%modSelect%\%Mod%" EQU "%undoArt%" GOTO :amUndo
SET "modSrc=%modSelect%\%Mod%"
IF EXIST "%modSrc%.cmd" (
	CALL "%modSrc%.cmd"
	IF ERRORLEVEL 2 GOTO :am1
	IF ERRORLEVEL 1 EXIT /B 0
)
IF DEFINED firstRun GOTO :am3
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
IF EXIST "%modSrc%.txt" TYPE "%modSrc%.txt"
BG Locate 20 0
%PC% F "     Create a backup before copying files? [" C "Esc" F ", " A "Y" F "es, " A "N" F "o]:"
:am2
CALL :bgInput kbd
IF %kbd% EQU 27 GOTO :am1
IF %kbd% EQU 110 SET kbd=78
IF %kbd% EQU 78 GOTO :am3
IF %kbd% NEQ 89 IF %kbd% NEQ 121 GOTO :am2
%PC% A "Yes"
CALL :timeName
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modSrc%\*.*" /A-D /B /S /ON 2^>NUL`) DO (
	SET "file=%%A"
	SETLOCAL EnableDelayedExpansion
	SET "file=!file:%modSrc:!=^!%\=!"
	IF EXIST "!mlDir!\!file!" (
		TITLE %SELF%: Moving !file!
		FOR %%B IN ("!undoArt!\!timeName!\!file!") DO MD "%%~dpB" >NUL 2>&1
		MOVE /Y "!mlDir!\!file!" "!undoArt!\!timeName!\!file!" >NUL 2>&1
	)
	ENDLOCAL
)
FOR /F "tokens=*" %%A IN ('DIR "%undoArt%" /AD /B /S ^| SORT /R') DO RD "%%A" >NUL 2>&1
IF EXIST "%undoArt%\%timeName%\" (
	ECHO      Created on %DateTime% when installing>"%undoArt%\%timeName%.txt"
	ECHO      %Mod%>>"%undoArt%\%timeName%.txt"
)
:am3
TITLE %SELF%: Copying %Mod%
IF %kbd% EQU 78 %PC% A "No"
XCOPY "%modSrc%\*.*" "%mlDir%\" /Q /S /Y /I >NUL 2>&1
EXIT /B 0
:amUndo
IF NOT EXIST "%undoArt%\" GOTO :am1
TITLE %SELF%: Undo Art Mod Menu
FOR /F "usebackq tokens=*" %%A IN (`DIR "%undoArt%" /AD-S /B /ON 2^>NUL`) DO SET "Mod=%%A"
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "                               Undo Art Mod Menu\n\n"
%PC% F "     Latest undo is dated" A " %Mod%\n\n"
IF EXIST "%undoArt%\%Mod%.txt" TYPE "%undoArt%\%Mod%.txt"
%PC% F "\n\n     Do you wish to restore this backup? [" C "Esc" F ", " A "Y" F "es, " A "N" F "o]:"
:amInput
CALL :bgInput kbd
IF %kbd% EQU 27 GOTO :am1
IF %kbd% EQU 110 EXIT /B 0
IF %kbd% EQU 78 EXIT /B 0
IF %kbd% NEQ 89 IF %kbd% NEQ 121 GOTO :amInput
%PC% A "Yes"
XCOPY "%undoArt%\%Mod%\*.*" "%mlDir%\" /Q /S /Y /I >NUL 2>&1
RD "%undoArt%\%Mod%" /S /Q >NUL 2>&1
RD "%undoArt%" >NUL 2>&1
DEL "%undoArt%\%Mod%.*" /F /Q /A >NUL 2>&1
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:SoundMod - select and install Sound Mods
TITLE %SELF%: Sound Mod Menu%firstRun%
SET "modSelect=%modSound%"
SET "modDisplay=                                Sound Mod Menu"
:smStart
CALL :Mod
IF ERRORLEVEL 1 EXIT /B 0
SET "modSrc=%modSelect%\%Mod%"
IF EXIST "%modSrc%.cmd" (
	CALL "%modSrc%.cmd"
	IF ERRORLEVEL 2 GOTO :smStart
	IF ERRORLEVEL 1 EXIT /B 0
)
SET "index=1"
SET "dB=5"
IF DEFINED firstRun GOTO :smStart1
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
IF EXIST "%modSrc%.txt" TYPE "%modSrc%.txt"
%PC% F "\n\n     Most of the Manalink sound files\n"
%PC% F "     have exceedingly high volumes.\n"
%PC% F "     You may choose to lower the volume\n"
%PC% F "     by making a selection below.\n\n"
%PC% F "     volume attenuation in decibels (" A "0" F "-" A "99" F "): " A "-5"
:smInput
CALL :bgInput kbd
::Esc
IF %kbd% EQU 27 GOTO :smStart
::Backspace
IF %kbd% EQU 8 (
	IF %index% GTR 0 (
		%PC% "\8 \8"
		SET /A "index-=1,dB/=10"
		GOTO :smInput
	)
)
::Enter
IF %kbd% EQU 13 (
	IF %dB% GEQ 0 IF %dB% LEQ 99 GOTO :smStart1
	GOTO :smInput
)
::0 thru 9
SET /A kbd-=48
IF %kbd% LSS 0 GOTO :smInput
IF %kbd% GTR 9 GOTO :smInput
IF %index% GEQ 2 GOTO :smInput
SET /A tempdB=(%dB% * 10) + %kbd%
IF %tempdB% GEQ 0 IF %tempdB% LEQ 99 (
	%PC% A "%kbd%"
	SET /A "index+=1,dB=%tempdB%"
)
GOTO :smInput
:smStart1
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
%PC% F "     Extracting files and lowering volume by %dB% decibels...\n"
IF EXIST "%mlDir%\_DuelSounds_OFF\Artifact.wav" CALL :ToggleSounds
XCOPY "%modSrc%\*.ogg" "%mlDir%\" /Q /S /Y /I >NUL 2>&1
BG Color 0A
BG Cursor 0
PUSHD "%mlDir%"
FOR /R %%A IN (*.ogg) DO (
    BG Locate 6 0
	OggDec "%%~fA"
	IF %dB% GTR 0 Normalize -q -a -%dB% "%%~dpnA.wav"
)
DEL /F /Q /A /S "*.ogg" >NUL 2>&1
POPD
BG Cursor 1
BG Color 0F
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:RoguesMod - select and install Rogues Mods
TITLE %SELF%: Rogues Mod Menu%firstRun%
SET "modSelect=%modRogues%"
SET "modDisplay=                                Rogues Mod Menu"
:rmStart
CALL :Mod
IF ERRORLEVEL 1 EXIT /B 0
SET "modSrc=%modSelect%\%Mod%"
SET "rogueSrc=%modSrc%\Rogues"
SET "pfaceSrc=%modSrc%\PlayFace"
SET RoguesOnly=
SET Display1=
SET Display2=
SET Display3=
SET kbd1=0
SET kbd2=0
SET kbd3=0

:: Check if Rogues Gallery is already installed
SET "rmCnt=0"
IF NOT EXIST "%mlDir%\Rogues.csv" ECHO Screen Name,Difficulty,Quote,Face Art>"%mlDir%\Rogues.csv"
FOR /F "usebackq skip=1 tokens=*" %%A IN ("%modSrc%\Rogues.csv") DO (
	SET "CheckWord=%%A"
	SET /A rmCnt+=1
)
SET Display3=%rmCnt%" F " more Rogues/PlayFaces"
SET "CheckWord=%CheckWord:"= %"
SET "CheckWord=%CheckWord:,= %"
FOR %%A IN ("%CheckWord: =" "%") DO SET "CheckWord=%%~A"
FOR /F "tokens=*" %%A IN ('FINDSTR /E /I "%CheckWord%" "%mlDir%\Rogues.csv"') DO SET Display2=This Rogues Gallery is Already Installed
IF EXIST "%modSrc%.cmd" (
	CALL "%modSrc%.cmd"
	IF ERRORLEVEL 2 GOTO :rmStart
	IF ERRORLEVEL 1 EXIT /B 0
)
IF DEFINED firstRun GOTO :rmRogues
IF DEFINED RoguesOnly (
	SET kbd1=1
	SET Display1=Install as Rogues Gallery Only
	GOTO :rmStart2
)
:rmStart1
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
%PC% A "     1)" F " Install as Rogues (for use by computer)\n"
%PC% A "     2)" F " Install as PlayFaces (for use by user)\n"
%PC% A "     3)" F " Install as Both Rogues and PlayFaces\n\n"
%PC% A "     %Display3%\n\n"
IF EXIST "%modSrc%.txt" TYPE "%modSrc%.txt"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "3" F "):"

:rmInput1
CALL :bgInput kbd1
IF %kbd1% EQU 27 GOTO :rmStart
SET /A kbd1-=48
:rmInput1a
IF %kbd1% LSS 1 GOTO :rmInput1
IF %kbd1% GTR 3 GOTO :rmInput1
IF %kbd1% EQU 1 SET Display1=Install as Rogues Gallery Only
IF %kbd1% EQU 2 (
	SET Display1=Install as PlayFaces Only
	GOTO :rmStart3
)
IF %kbd1% EQU 3 SET Display1=Install as Both Rogues and PlayFaces

:rmStart2
CLS
SET rm1=A "     3)" F " Add New Rogues to the Current Gallery\n\n"
IF DEFINED Display2 SET rm1=8 "     3)" 8 " Add New Rogues to the Current Gallery\n\n"
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
%PC% A "     %Display1%\n\n"
%PC% A "     1)" F " Replace Current Rogues (remove old files)\n"
%PC% A "     2)" F " Replace Current Rogues (retain old files)\n"
%PC% %rm1%
%PC% D "     %Display2%\n\n"
IF DEFINED RoguesOnly IF EXIST "%modSrc%.txt" TYPE "%modSrc%.txt"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "3" F "):"

:rmInput2
CALL :bgInput kbd2
IF %kbd2% EQU 27 (
	IF DEFINED RoguesOnly (
		GOTO :rmStart
	) ELSE GOTO :rmStart1
)
SET /A kbd2-=48
IF %kbd2% LSS 1 GOTO :rmInput2
IF %kbd2% GTR 3 GOTO :rmInput2
IF %kbd2% EQU 3 IF DEFINED Display2 GOTO :rmInput2
IF %kbd1% EQU 1 GOTO :rmRogues

:rmStart3
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %Mod%\n\n"
%PC% A "     %Display1%\n\n"
%PC% A "     1)" F " Replace All PlayFaces (remove old files)\n"
%PC% A "     2)" F " Add New PlayFaces     (retain old files)\n\n"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "2" F "):"

:rmInput3
CALL :bgInput kbd3
IF %kbd3% EQU 27 (
	IF %kbd1% EQU 50 (
		GOTO :rmStart1
	) ELSE GOTO :rmStart2
)
SET /A kbd3-=48
IF %kbd3% LSS 1 GOTO :rmInput3
IF %kbd3% GTR 2 GOTO :rmInput3
IF %kbd1% EQU 2 GOTO :rmPlayFaces

:rmRogues
IF %kbd2% EQU 1 (
	DEL /F /Q /A "%mlDir%\Exp1Art\Rogues\*.*" >NUL 2>&1
	ECHO Screen Name,Difficulty,Quote,Face Art>"%mlDir%\Rogues.csv"
)
IF %kbd2% EQU 2 ECHO Screen Name,Difficulty,Quote,Face Art>"%mlDir%\Rogues.csv"
FOR /F "usebackq skip=1 tokens=*" %%A IN ("%modSrc%\Rogues.csv") DO ECHO %%A>>"%mlDir%\Rogues.csv"
MD "%mlDir%\Exp1Art\Rogues\" >NUL 2>&1
COPY /Y "%modSrc%\*.*" "%mlDir%\Exp1Art\Rogues\" >NUL 2>&1
DEL /F /Q "%mlDir%\Exp1Art\Rogues\*.csv" >NUL 2>&1
IF %kbd1% EQU 1 EXIT /B 0

:rmPlayFaces
MD "%mlDir%\PlayFace\" >NUL 2>&1
IF %kbd3% EQU 1 DEL /F /Q /A "%mlDir%\PlayFace\*.*" >NUL 2>&1
COPY /Y "%modSrc%\*.*" "%mlDir%\PlayFace\" >NUL 2>&1
DEL /F /Q "%mlDir%\PlayFace\*.csv" >NUL 2>&1
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:PlayDeckMod - select and install PlayDeck Folders
TITLE %SELF%: PlayDeck Mod Menu
SET "pdLevel=0"
SET "modSrc=%modPlayDeck%"
SET "modSelect=%modPlayDeck%"
SET "modDisplay=                               PlayDeck Mod Menu"
:pdMod
FOR %%A IN ("%modSelect%") DO SET "Mod=%%~nA"
CALL SET "dirDisplay=.%%modSelect:%modDir%=%%\"
CALL :Mod
IF ERRORLEVEL 2 GOTO :pdStart
IF ERRORLEVEL 1 (
	IF %pdLevel% LSS 1 EXIT /B 0
	CALL SET "modSelect=%%msBak[%pdLevel%]%%"
	SET /A pdLevel-=1
	GOTO :pdMod
)
SET "modSrc=%modSelect%\%Mod%"
SET "flag="
FOR /F "usebackq tokens=*" %%A IN (`DIR "%modSrc%" /AD-S /B /ON 2^>NUL`) DO SET "flag=1"
SET /A pdLevel+=1
IF DEFINED flag (
	SET "msBak[%pdLevel%]=%modSelect%"
	SET "modSelect=%modSrc%"
	GOTO :pdMod
)
SET /A pdLevel-=1
IF EXIST "%modSrc%.cmd" (
	CALL "%modSrc%.cmd"
	IF ERRORLEVEL 2 GOTO :pdMod
	IF ERRORLEVEL 1 EXIT /B 0
)
:pdStart
CLS
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
%PC% C "     %dirDisplay:\=\\%%Mod%\n\n"
%PC% A "     1)" F " Add PlayDecks," A " Ignoring" F " duplicates.\n"
%PC% A "     2)" F " Add PlayDecks," C " Overwriting" F " duplicates.\n"
%PC% A "     3)" F " Replace PlayDecks," A " Backup" F " current contents.\n"
%PC% A "     4)" F " Replace Playdecks," C " Delete" F " current contents.\n\n"
IF EXIST "%modSrc%.txt" TYPE "%modSrc%.txt"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "1" F "-" A "4" F "):"

:pdInput
CALL :bgInput kbd
IF %kbd% EQU 27 GOTO :pdMod
SET /A kbd-=48
IF %kbd% LSS 1 GOTO :pdInput
IF %kbd% GTR 4 GOTO :pdInput
%PC% A "%kbd%"
IF %kbd% EQU 1 (
	FOR /F "usebackq tokens=*" %%A IN (`DIR "%modSrc%\*.dck" /A-D-S /B /ON 2^>NUL`) DO (
		IF NOT EXIST "%mlDir%\PlayDeck\%%~nxA" COPY /Y "%modSrc%\%%~nxA" "%mlDir%\PlayDeck\" >NUL 2>&1
	)
	EXIT /B 0
)
IF %kbd% EQU 2 (
	COPY /Y "%modSrc%\*.dck" "%mlDir%\PlayDeck\" >NUL 2>&1
	EXIT /B 0
)
IF %kbd% EQU 4 (
	DEL /F /Q /A "%mlDir%\PlayDeck\*.dck" >NUL 2>&1
	COPY /Y "%modSrc%\*.dck" "%mlDir%\PlayDeck\" >NUL 2>&1
	EXIT /B 0
)
CALL :timeName
MD "%undoPlayDeck%\%timeName%\" >NUL 2>&1
MOVE /Y "%mlDir%\PlayDeck\*.*" "%undoPlayDeck%\%timeName%\" >NUL 2>&1
COPY /Y "%modSrc%\*.dck" "%mlDir%\PlayDeck\" >NUL 2>&1
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Mod - displays folders for the Mods Menus
IF DEFINED firstRun (
	IF NOT EXIST "%modSelect%\%Mod%\" CALL :unDo "%modSelect%\%Mod%"
	EXIT /B 0
)
IF EXIST "%modSelect%\%Mod%.7z" CALL :killArc "%modSelect%\%Mod%"
IF EXIST "%modSelect%\%Mod%.zip" CALL :killArc "%modSelect%\%Mod%"
FOR /L %%A IN (0,1,%modCnt%) DO SET "mod[%%A]="
SET "page="
SET "mFlag="
SET "modCnt=-1"
IF DEFINED dirDisplay (
	IF /I "%modSelect%" NEQ "%mlDir%\PlayDeck" (
		IF EXIST "%modSelect%\*.dck" (
			SET "mFlag=1"
			SET "modCnt=0"
			SET "mod[0]=%Mod%"
		)
	)
)
RD "%modTmp%" /S /Q >NUL 2>&1
MD "%modTmp%" >NUL 2>&1
DIR "%modSelect%" /AD-S /B /ON 1>"%modTmp%\sort.tmp" 2>NUL
FOR %%A IN (7z zip) DO (
	FOR /F "tokens=*" %%B IN ('DIR "%modSelect%\*.%%A" /A-D-S /B /ON 2^>NUL') DO (
		IF NOT EXIST "%modSelect%\%%~nB\" %PC% "%%~nB\n">>"%modTmp%\sort.tmp"
	)
)
FOR /F "usebackq tokens=*" %%A IN (`SORT "%modTmp%\sort.tmp"`) DO (
	SET /A modCnt+=1
	CALL SET "mod[%%modCnt%%]=%%A"
)
DEL /F /Q "%modTmp%\sort.tmp" >NUL 2>&1
:ModMenu
CLS
IF "%page%" EQU "0" SET "page="
%PC% F "\n                               Manalink Launcher\n"
%PC% F "%modDisplay%\n\n"
IF DEFINED dirDisplay %PC% C "     %dirDisplay:\=\\%\n\n"
IF %modCnt% LSS 0 (
	%PC% 7 "     There are no Mods available in this folder. Press any key to return..."
	BG Kbd
	EXIT /B 1
)
SETLOCAL EnableDelayedExpansion
SET "mmCnt=-1"
FOR /L %%A IN (%page%0,1,%page%9) DO (
	IF DEFINED mod[%%A] (
		%PC% A "     %%A)" F " !mod[%%A]!\n"
		SET /A mmCnt+=1
	)
)
ENDLOCAL & SET "mmCnt=%mmCnt%"
ECHO(
IF EXIST "%modSelect%\folder.txt" TYPE "%modSelect%\folder.txt"
SET "mm1="
SET "mm2="
SET "mm3="
IF %mmCnt% GTR 0 SET mm1= F "-" A "%mmCnt%"
IF DEFINED page SET mm2= F "," A " <" F "-" A "B" F "ack a page"
IF %page%9 LSS %modCnt% SET mm3= F "," A " N" F "ext Page-" A ">"
BG Locate 20 0
%PC% F "     Please select (" C "Esc" F ", " A "0"%mm1%%mm2%%mm3% F "):"
:ModInput
CALL :bgInput kbd
::Esc
IF %kbd% EQU 27 EXIT /B 1
::Back
IF %kbd% EQU 331 SET "kbd=66"
IF %kbd% EQU 98  SET "kbd=66"
IF %kbd% EQU 66 (
	IF NOT DEFINED page GOTO :ModInput
	%PC% A "Back"
	SET /A page-=1
	GOTO :ModMenu
)
::Next
IF %kbd% EQU 333 SET "kbd=78"
IF %kbd% EQU 110 SET "kbd=78"
IF %kbd% EQU 78 (
	IF %page%9 GEQ %modCnt% GOTO :ModInput
	%PC% A "Next"
	SET /A page+=1
	GOTO :ModMenu
)
SET /A kbd-=48
IF NOT DEFINED mod[%page%%kbd%] GOTO :ModInput
%PC% A "%page%%kbd%"
CALL SET "Mod=%%mod[%page%%kbd%]%%"
IF NOT EXIST "%modSelect%\%Mod%\" CALL :unDo "%modSelect%\%Mod%"
::if PlayDeck and Self contains *.dck
IF DEFINED mFlag IF %kbd% EQU 0 EXIT /B 2
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:killArc
RD %1 /S /Q >NUL 2>&1
DEL /F /Q /A "%~1.txt" >NUL 2>&1
DEL /F /Q /A "%~1.cmd" >NUL 2>&1
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:unDo - extract archive
%PC% F "\n\n     ...extracting" A " %~nx1"
PUSHD "%~dp1"
IF EXIST "%~nx1.7z" (
	7ZA.EXE x -y "%~nx1.7z" >NUL 2>&1
) ELSE 7ZA.EXE x -y "%~nx1.zip" >NUL 2>&1
POPD
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:Zip
TITLE %~n0: "%~nx1"
SET "zTmp="
IF EXIST "%~1.7z" SET "zTmp=7z"
IF EXIST "%~1.zip" SET "zTmp=zip"
IF NOT DEFINED zTmp GOTO :ZipIt
%PC% A "\n\n     %~nx1.%zTmp%" F " already exists.\n"
%PC% F "     Would you like to replace the file? [" A "Y" F "/" A "N" F "]:"
CALL :bgInput kbd
IF %kbd% NEQ 89 IF %kbd% NEQ 121 (
	%PC% A "No"
	EXIT /B 0
)
%PC% A "Yes"
DEL /F /Q /A "%~1.%zTmp%" >NUL 2>&1
:ZipIt
PUSHD "%~dp1"
7ZA.EXE a -t7z -mx9 -y "%~nx1.7z" "%~nx1" "%~nx1.cmd" "%~nx1.txt"
POPD
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:timeName - returns Year_Month_Date_Hour_Min_Sec eg. 2015_05_23_10_15_31
SET "DateTime=%DATE% at %TIME: =0%"
SETLOCAL EnableDelayedExpansion
FOR /F "tokens=2-7" %%A IN ('BG.EXE Time') DO (
	IF %%B LSS 10 (
		SET timeName=%%A_0%%B
	) ELSE SET timeName=%%A_%%B
	IF %%C LSS 10 (
		SET timeName=!timeName!_0%%C
	) ELSE SET timeName=!timeName!_%%C
	IF %%D LSS 10 (
		SET timeName=!timeName!_0%%D
	) ELSE SET timeName=!timeName!_%%D
	IF %%E LSS 10 (
		SET timeName=!timeName!_0%%E
	) ELSE SET timeName=!timeName!_%%E
	IF %%F LSS 10 (
		SET timeName=!timeName!_0%%F
	) ELSE SET timeName=!timeName!_%%F
)
ENDLOCAL & SET "timeName=%timeName%"
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:bgInput - waits for a keystroke, then returns the decimal ASCII value
BG LastKbd
IF ERRORLEVEL 1 GOTO :bgInput
BG Kbd
SET "%~1=%ERRORLEVEL%"
EXIT /B 0

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:nameCorrect [/i] DirToCorrect [DirOfSource]
SET "ncCnt=0"
SET "ncJump=nc1"
SET "ncSrc=%SELFDIR%"
IF /I "%~1" EQU "/i" SET "ncJump=nc2" & SHIFT
IF "%~1" EQU "" EXIT /B 0
IF "%~2" NEQ "" SET "ncSrc=%~2"
FOR /F "tokens=*" %%A IN ('DIR %1 /A-D-S /B /S /ON 2^>NUL') DO (
	TITLE %~n0: "%%~nxA"
	CALL :%ncJump% "%%A"
	IF ERRORLEVEL 1 (
		ECHO Can't match "%%A"
	) ELSE SET /A cnt+=1
)
TITLE %~n0: Matched %cnt% files
ECHO Matched %cnt% files.
EXIT /B 0
:nc1
FOR /R "%ncSrc%" %%B IN (*) DO (
	IF /I "%~nx1" EQU "%%~nxB" (
		RENAME "%~1" "%%~nxB"
		EXIT /B 0
	)
)
EXIT /B 1
:nc2
FOR /R "%ncSrc%" %%B IN (*) DO (
	IF /I "%~n1" EQU "%%~nB" (
		RENAME "%~1" "%%~nB%~x1"
		EXIT /B 0
	)
)
EXIT /B 1
