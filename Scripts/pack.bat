@echo This script should be executed from the Visual Studio Developer Command Prompt.
@echo Otherwise, the SvcWatchDog binary won't be recompiled - though this may not be an issue if you're aware of the implications.

@SET OUTPUTFOLDER=..\SvcWatchDogDist
@echo Folder %OUTPUTFOLDER% will be deleted and recreated. If you don't want this, press ctrl-c.

pause

msbuild ..\SvcWatchDog.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64


if exist %OUTPUTFOLDER% rd /s /q %OUTPUTFOLDER%

mkdir %OUTPUTFOLDER%\SvcWatchDog
mkdir %OUTPUTFOLDER%\TestService
mkdir %OUTPUTFOLDER%\Doc

copy /y ..\x64\Release\SvcWatchDog.exe %OUTPUTFOLDER%\SvcWatchDog
copy /y ..\SvcWatchDog.json %OUTPUTFOLDER%\SvcWatchDog
copy TestService.ps1 %OUTPUTFOLDER%\TestService

copy /y ..\LICENSE* %OUTPUTFOLDER%\Doc
copy /y ..\*.md %OUTPUTFOLDER%\Doc

pause