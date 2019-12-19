set WINSDK_VER=10.0.18362.0
set LIBRARY=openssl
set LIB_SOURCE_DIR=OpenSSL_1_1_0l
set VCBUILDTOOLS=x86
set BUILD_ARCH=x86
set OPENSSL_TARGET=VC-WIN32
set ROOT_DIR=\users\%USERNAME%\harbour
set NASM_DIR=NASM32

IF NOT DEFINED OPENSSL_BUILD set INCLUDE=
IF NOT DEFINED OPENSSL_BUILD set LIBPATH=
IF NOT DEFINED OPENSSL_BUILD set PATH=c:\windows;c:\windows\system32
IF NOT DEFINED OPENSSL_BUILD call "C:\Program Files (x86)\Microsoft Visual C++ Build Tools\vcbuildtools.bat"  %VCBUILDTOOLS%
IF NOT DEFINED OPENSSL_BUILD set PATH=%PATH%;C:\Program Files\Git\cmd
IF NOT DEFINED OPENSSL_BUILD set PATH=%PATH%;C:\Users\%USERNAME%\AppData\Local\Programs\Microsoft VS Code\bin
IF NOT DEFINED OPENSSL_BUILD set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\bin\%WINSDK_VER%\%BUILD_ARCH%
IF NOT DEFINED OPENSSL_BUILD SET PATH=%PATH%;C:\Strawberry\c\bin
IF NOT DEFINED OPENSSL_BUILD SET PATH=%PATH%;C:\Strawberry\perl\bin
IF NOT DEFINED OPENSSL_BUILD SET PATH=%PATH%;C:\Strawberry\perl\site\bin
IF NOT DEFINED OPENSSL_BUILD SET PATH=%PATH%;%ROOT_DIR%\tools\win32\%NASM_DIR%
SET OPENSSL_BUILD=1


set LIB_TARGET=%ROOT_DIR%\3rd\%BUILD_ARCH%\%LIBRARY%
set HB_INSTALL_PREFIX=%ROOT_DIR%\build\%BUILD_ARCH%\harbour

cd %ROOT_DIR%\3rd\%LIBRARY%\%LIB_SOURCE_DIR%

call "cpan install Text:Template"

perl Configure --prefix=%LIB_TARGET% --openssldir=%LIB_TARGET% %OPENSSL_TARGET%
nmake -f makefile clean install

cd %ROOT_DIR%\3rd\%LIBRARY%

dir /s %LIB_TARGET%
