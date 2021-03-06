@echo off
cd %GRISBISRC%
::DEFINITION DES VERSIONS
:: Change the next lines to choose which gtk+ version you download.
:: Choose runtime version posterior to dev version
::    Get this file name from http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.24/gtk+-bundle_2.24.10-20120208_win32.zip
::    Specify the BUNDLE file
::    Don't include the extension
SET GTK_DEV_FILE_BASENAME=gtk+-bundle_2.24.10-20120208_win32
SET GTK_DEV_BUNDLE_VERSION=2.24
SET ZLIB_DEV_FILE_BASENAME%=zlib_1.2.5-2_win32
SET LIBXML_FILE_BASENAME=libxml2_2.9.0-1_win32
SET LIBXML_FILE_DEV_NAME=libxml2-dev_2.9.0-1_win32
SET ICONV_FILE_BASENAME=iconv-1.9.2.win32
SET OPENSSL_FILE_BASENAME=Win32OpenSSL-1_0_0j
SET LIBGSF_FILE_BASENAME=libgsf_1.14.17-1_win32
SET LIBGSF_FILE_DEV_NAME=libgsf-dev_1.14.17-1_win32
SET LIBPTHREAD_FILE_BASENAME=libpthread-2.8.0-3-mingw32-dll-2

:: The rest of the script should do the rest
:: on met chcp 1252 pour les wget car ils affiche en francais
SET CURRENT_DIR=%CD%
IF NOT EXIST target MKDIR target
IF NOT EXIST target\Win32 MKDIR target\Win32
SET TARGET_DIR=%CURRENT_DIR%\target\Win32
IF NOT EXIST target\Win32\plugins-dev MKDIR target\Win32\plugins-dev
IF NOT EXIST downloads MKDIR downloads
SET DOWNLOADS_DIR=%CURRENT_DIR%\downloads
IF NOT EXIST target\Win32/package MKDIR target\Win32\package

echo Downloads directory : %DOWNLOADS_DIR%
echo Target directory : %TARGET_DIR%

:: Download and unzip libxml2 dev and bin files

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/%LIBXML_FILE_BASENAME%.zip
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/%LIBXML_FILE_DEV_NAME%.zip
cd %TARGET_DIR%
unzip -uo "%DOWNLOADS_DIR%\%LIBXML_FILE_BASENAME%.zip" -d plugins-dev\libxml2
unzip -uo "%DOWNLOADS_DIR%\%LIBXML_FILE_DEV_NAME%.zip" -d plugins-dev\libxml2
echo libxml ok

:: Download and unzip libgsf dev and bin files

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/libgsf/1.14/%LIBGSF_FILE_BASENAME%.zip
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/libgsf/1.14/%LIBGSF_FILE_DEV_NAME%.zip
cd %TARGET_DIR%
unzip -uo "%DOWNLOADS_DIR%\%LIBGSF_FILE_BASENAME%.zip" -d plugins-dev\libgsf-1
unzip -uo "%DOWNLOADS_DIR%\%LIBGSF_FILE_DEV_NAME%.zip" -d plugins-dev\libgsf-1
echo libgsf ok

:: Download and unzip iconv dev and bin files

cd "%DOWNLOADS_DIR%"
chcp 1252 && wget -nc ftp://ftp.zlatkovic.com/libxml/%ICONV_FILE_BASENAME%.zip
cd "%TARGET_DIR%"
unzip -uo "%DOWNLOADS_DIR%\%ICONV_FILE_BASENAME%.zip" -d plugins-dev
IF EXIST plugins-dev\iconv RMDIR /S /Q plugins-dev\iconv
MOVE plugins-dev\%ICONV_FILE_BASENAME% plugins-dev\iconv
echo iconv ok
PAUSE

:: Download and install openssl, copy the required files in the right place

cd "%DOWNLOADS_DIR%"
chcp 1252 && wget -nc http://www.slproweb.com/download/%OPENSSL_FILE_BASENAME%.exe
SET SSLDIR=%SystemDrive%\OpenSSL-Win32
IF NOT EXIST "%SSLDIR%\readme.txt" (
	IF EXIST %OPENSSL_FILE_BASENAME%.exe (
		ECHO ***** ATTENTION: installer openssl sur le disque systeme generalement C: dans le repertoire par defaut *****
		PAUSE
		START /WAIT %OPENSSL_FILE_BASENAME%.exe 
		echo installation faite
	) ELSE (
		ECHO OpenSSL package not downloaded. Please edit the script with correct filename and re-run.
		CD "%CURRENT_DIR%"
		EXIT /B 1)
) ELSE (
	ECHO OpenSSL already installed. Please check version.)
cd "%TARGET_DIR%"
if not exist plugins-dev\openssl mkdir plugins-dev\openssl
if not exist plugins-dev\openssl\lib mkdir plugins-dev\openssl\lib
xcopy /Y %SSLDIR%\lib\libeay32.lib plugins-dev\openssl\lib\.
xcopy /Y %SSLDIR%\lib\ssleay32.lib plugins-dev\openssl\lib\.
IF NOT EXIST plugins-dev\openssl\include mkdir plugins-dev\openssl\include
xcopy /YICD %SSLDIR%\include\openssl\* plugins-dev\openssl\include\openssl
IF NOT EXIST plugins-dev\openssl\bin mkdir plugins-dev\openssl\bin
xcopy /YICD %SSLDIR%\*.dll plugins-dev\openssl\bin
echo openssl ok

:: Download and unzip libofx dev and bin files

cd "%DOWNLOADS_DIR%"
chcp 1252 && wget -nc http://sourceforge.net/projects/grisbi/files/dependancies/0.7/libofx_mingw.zip/download
cd "%TARGET_DIR%"
unzip -uo "%DOWNLOADS_DIR%\libofx_mingw.zip" -d plugins-dev
echo ofx ok

:: Download and unzip libgoffice dev and bin files and other dll (provisoire)

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://sourceforge.net/projects/grisbi/files/dependancies/0.9/libgoffice-0.8-8.zip/download
cd %TARGET_DIR%
unzip -uo "%DOWNLOADS_DIR%\libgoffice-0.8-8.zip" -d plugins-dev
echo libgoffice ok

:: Download and unzip other files (provisoire)

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://sourceforge.net/projects/grisbi/files/dependancies/0.9/gtkrc/download
chcp 1252 && wget -nc http://sourceforge.net/projects/grisbi/files/dependancies/0.9/libxml2.dll/download
chcp 1252 && wget -nc http://sourceforge.net/projects/grisbi/files/dependancies/0.9/IEShims.dll/download
cd %TARGET_DIR%
copy /Y %DOWNLOADS_DIR%\gtkrc gtk-dev\etc\gtk-2.0
copy /Y %DOWNLOADS_DIR%\libxml2.dll package
copy /Y %DOWNLOADS_DIR%\IEShims.dll package
echo other files ok

:: Download and unzip pcre dev and bin files

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://sourceforge.net/projects/gnuwin32/files/pcre/7.0/pcre-7.0-bin.zip/download
chcp 1252 && wget -nc http://sourceforge.net/projects/gnuwin32/files/pcre/7.0/pcre-7.0-lib.zip/download
cd %TARGET_DIR%
unzip -uo "%DOWNLOADS_DIR%\pcre-7.0-bin.zip" -d plugins-dev\pcre
unzip -uo "%DOWNLOADS_DIR%\pcre-7.0-lib.zip" -d plugins-dev\pcre
echo pcre ok

:: Download and unzip libpthread-2.dll file

cd %DOWNLOADS_DIR%
chcp 1252 && wget -nc http://sourceforge.net/projects/mingw/files/MinGW/Base/pthreads-w32/pthreads-w32-2.8.0-3/%LIBPTHREAD_FILE_BASENAME%.tar.lzma/download
:: cd %TARGET_DIR%
7z e -y "%LIBPTHREAD_FILE_BASENAME%.tar.lzma"
7z e -y "%LIBPTHREAD_FILE_BASENAME%.tar"
IF NOT EXIST %TARGET_DIR%\plugins-dev\libpthread mkdir %TARGET_DIR%\plugins-dev\libpthread
MOVE libpthread-2.dll %TARGET_DIR%\plugins-dev\libpthread
RM -f -v %LIBPTHREAD_FILE_BASENAME%.tar
echo libpthread ok
PAUSE

:: Download and unzip gtk dev and bin files

cd "%DOWNLOADS_DIR%"
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/%GTK_DEV_BUNDLE_VERSION%/%GTK_DEV_FILE_BASENAME%.zip
chcp 1252 && wget -nc http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/%ZLIB_DEV_FILE_BASENAME%.zip

:: Checking if already unzipped
:: If the readme file exists for this version, update the unzip
cd "%TARGET_DIR%"
IF NOT EXIST gtk-dev (
	echo GTK+ development files not present.
	echo Unzipping the archive now...
	unzip "%DOWNLOADS_DIR%\%GTK_DEV_FILE_BASENAME%" -d gtk-dev
	unzip -uo "%DOWNLOADS_DIR%\%ZLIB_DEV_FILE_BASENAME%" -d gtk-dev
	echo Done unzipping archive!
)
IF EXIST gtk-dev\%GTK_DEV_FILE_BASENAME%.README.txt (
	echo Found gtk-dev directory with the same gtk+ version.
	echo Updating the files...
	unzip -uo "%DOWNLOADS_DIR%\%GTK_DEV_FILE_BASENAME%" -d gtk-dev
	echo Done updating the files in gtk-dev!
)
:: If the readme file for the current version does not exist, remove the dir, and unzip again
IF NOT EXIST gtk-dev\%GTK_DEV_FILE_BASENAME%.README.txt (
	echo Found gtk-dev directory with a different gtk+ version.
	echo Deleting gtk-dev for compatibility reasons...
	rmdir /S /Q gtk-dev
	echo Done deleting gtk-dev directory!
	echo Unzipping downloaded archive...
	unzip "%DOWNLOADS_DIR%\%GTK_DEV_FILE_BASENAME%" -d gtk-dev
	echo Done unzipping archive!
)
echo gtk ok
SET GTK_DEV_FILE_BASENAME=
cd "%CURRENT_DIR%"

call generate.cmd
PAUSE

call build.cmd
