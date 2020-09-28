rem =============== BUILDING THE LIBRARY SDK AND THE DEMO ===============

md build-%COMPILER%-%BUILD_TYPE%-%PLATFORM%
cd build-%COMPILER%-%BUILD_TYPE%-%PLATFORM%

if NOT [%TOOLCHAIN_BIN%]==[] set PATH=%TOOLCHAIN_BIN%;%PATH:C:\Program Files\Git\usr\bin;=%

cmake -G "%GENERATOR%"^
 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%^
 -DCMAKE_PREFIX_PATH=%CMAKEPREFIXPATH%^
 -DCMAKE_INSTALL_PREFIX=libOPNMIDI^
 -DlibOPNMIDI_STATIC=%BUILD_STATIC%^
 -DlibOPNMIDI_SHARED=%BUILD_SHARED%^
 -DWITH_MIDIPLAY=%BUILD_MIDIPLAY%^
 -DWITH_VLC_PLUGIN=%VLC_PLUGIN%^
 -DVLC_PLUGIN_NOINSTALL=ON^
 -DUSE_VGM_FILE_DUMPER=ON^
 -DUSE_GX_EMULATOR=ON^
 -DWITH_MIDI2VGM=ON^
 ..
if %errorlevel% neq 0 exit /b %errorlevel%

if [%COMPILER_FAMILY%]==[MinGW] (
	cmake --build . --config %BUILD_TYPE% -- -j 2
	if %errorlevel% neq 0 exit /b %errorlevel%
)

if [%COMPILER_FAMILY%]==[MinGW] (
	mingw32-make install
	if %errorlevel% neq 0 exit /b %errorlevel%
)

if [%COMPILER_FAMILY%]==[MSVC] (
	cmake --build . --config %BUILD_TYPE% --target install
	if %errorlevel% neq 0 exit /b %errorlevel%
)

7z a -t7z -mx9 "libOPNMIDI-%COMPILER%-%BUILD_TYPE%-%PLATFORM%.7z" "libOPNMIDI"
if %errorlevel% neq 0 exit /b %errorlevel%

appveyor PushArtifact "libOPNMIDI-%COMPILER%-%BUILD_TYPE%-%PLATFORM%.7z"
if %errorlevel% neq 0 exit /b %errorlevel%

