/*
 * hb_fsLink*(), hb_FLink*() functions
 *
 * Copyright 2010 Viktor Szakats (vszakats.net/harbour)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.txt.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA (or visit https://www.gnu.org/licenses/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "hbapi.h"
#include "hbapifs.h"
#include "hbvm.h"

#if defined( HB_OS_WIN ) && ! defined( HB_OS_WIN_CE )
   #include <windows.h>
   #if ! defined( INVALID_FILE_ATTRIBUTES )
      #define INVALID_FILE_ATTRIBUTES  ( ( DWORD ) -1 )
   #endif
   #include "hbwinuni.h"
#elif defined( HB_OS_UNIX )
   #include <unistd.h>
#endif

HB_BOOL hb_fsLink( const char * pszExisting, const char * pszNewFile )
{
   HB_BOOL fResult;

   if( pszExisting && pszNewFile )
   {
      hb_vmUnlock();

#if defined( HB_OS_WIN ) && ! defined( HB_OS_WIN_CE )
      {
         typedef BOOL ( WINAPI * _HB_CREATEHARDLINK )( LPCTSTR, LPCTSTR, LPSECURITY_ATTRIBUTES );

         static _HB_CREATEHARDLINK s_pCreateHardLink = ( _HB_CREATEHARDLINK ) -1;

         if( s_pCreateHardLink == ( _HB_CREATEHARDLINK ) -1 )
         {
            HMODULE hModule = GetModuleHandle( TEXT( "kernel32.dll" ) );
            if( hModule )
               s_pCreateHardLink = ( _HB_CREATEHARDLINK )
                  HB_WINAPI_GETPROCADDRESST( hModule, "CreateHardLink" );
            else
               s_pCreateHardLink = NULL;
         }

         if( s_pCreateHardLink )
         {
            LPCTSTR lpFileName, lpExistingFileName;
            LPTSTR lpFileNameFree, lpExistingFileNameFree;

            lpFileName = HB_FSNAMECONV( pszNewFile, &lpFileNameFree );
            lpExistingFileName = HB_FSNAMECONV( pszExisting, &lpExistingFileNameFree );

            fResult = s_pCreateHardLink( lpFileName, lpExistingFileName, NULL ) != 0;
            hb_fsSetIOError( fResult, 0 );

            if( lpFileNameFree )
               hb_xfree( lpFileNameFree );
            if( lpExistingFileNameFree )
               hb_xfree( lpExistingFileNameFree );
         }
         else
         {
            hb_fsSetError( 1 );
            fResult = HB_FALSE;
         }
      }
#elif defined( HB_OS_UNIX )
      {
         char * pszExistingFree;
         char * pszNewFileFree;

         pszExisting = hb_fsNameConv( pszExisting, &pszExistingFree );
         pszNewFile = hb_fsNameConv( pszNewFile, &pszNewFileFree );

         fResult = ( link( pszExisting, pszNewFile ) == 0 );
         hb_fsSetIOError( fResult, 0 );

         if( pszExistingFree )
            hb_xfree( pszExistingFree );
         if( pszNewFileFree )
            hb_xfree( pszNewFileFree );
      }
#else
      {
         hb_fsSetError( 1 );
         fResult = HB_FALSE;
      }
#endif

      hb_vmLock();
   }
   else
   {
      hb_fsSetError( 2 );
      fResult = HB_FALSE;
   }

   return fResult;
}

HB_BOOL hb_fsLinkSym( const char * pszTarget, const char * pszNewFile )
{
   HB_BOOL fResult;

   if( pszTarget && pszNewFile )
   {
      hb_vmUnlock();

#if defined( HB_OS_WIN ) && ! defined( HB_OS_WIN_CE )
      {
         typedef BOOL ( WINAPI * _HB_CREATESYMBOLICLINK )( LPCTSTR, LPCTSTR, DWORD );

         static _HB_CREATESYMBOLICLINK s_pCreateSymbolicLink = ( _HB_CREATESYMBOLICLINK ) -1;

         #ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
         #define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
         #endif
         /* Requires Windows 10 Insiders Build 14972 or newer
            https://blogs.windows.com/buildingapps/2016/12/02/symlinks-windows-10/ */
         #ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
         #define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
         #endif

         if( s_pCreateSymbolicLink == ( _HB_CREATESYMBOLICLINK ) -1 )
         {
            HMODULE hModule = GetModuleHandle( TEXT( "kernel32.dll" ) );
            if( hModule )
               s_pCreateSymbolicLink = ( _HB_CREATESYMBOLICLINK )
                  HB_WINAPI_GETPROCADDRESST( hModule, "CreateSymbolicLink" );
            else
               s_pCreateSymbolicLink = NULL;
         }

         if( s_pCreateSymbolicLink )
         {
            LPCTSTR lpSymlinkFileName, lpTargetFileName;
            LPTSTR lpSymlinkFileNameFree, lpTargetFileNameFree;
            DWORD dwAttr;
            HB_BOOL fDir;

            lpSymlinkFileName = HB_FSNAMECONV( pszNewFile, &lpSymlinkFileNameFree );
            lpTargetFileName = HB_FSNAMECONV( pszTarget, &lpTargetFileNameFree );

            dwAttr = GetFileAttributes( lpTargetFileName );
            fDir = ( dwAttr != INVALID_FILE_ATTRIBUTES ) &&
                   ( dwAttr & FILE_ATTRIBUTE_DIRECTORY );

            fResult = s_pCreateSymbolicLink( lpSymlinkFileName, lpTargetFileName,
               ( fDir ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0 ) | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE ) != 0;
            hb_fsSetIOError( fResult, 0 );

            if( lpSymlinkFileNameFree )
               hb_xfree( lpSymlinkFileNameFree );
            if( lpTargetFileNameFree )
               hb_xfree( lpTargetFileNameFree );
         }
         else
         {
            hb_fsSetError( 1 );
            fResult = HB_FALSE;
         }
      }
#elif defined( HB_OS_UNIX )
      {
         char * pszTargetFree;
         char * pszNewFileFree;

         pszTarget = hb_fsNameConv( pszTarget, &pszTargetFree );
         pszNewFile = hb_fsNameConv( pszNewFile, &pszNewFileFree );

         fResult = ( symlink( pszTarget, pszNewFile ) == 0 );
         hb_fsSetIOError( fResult, 0 );

         if( pszTargetFree )
            hb_xfree( pszTargetFree );
         if( pszNewFileFree )
            hb_xfree( pszNewFileFree );
      }
#else
      {
         hb_fsSetError( 1 );
         fResult = HB_FALSE;
      }
#endif

      hb_vmLock();
   }
   else
   {
      hb_fsSetError( 2 );
      fResult = HB_FALSE;
   }

   return fResult;
}

/* NOTE: Caller must free the pointer, if not NULL */
char * hb_fsLinkRead( const char * pszFile )
{
   char * pszLink = NULL;

   if( pszFile )
   {
      hb_vmUnlock();

#if defined( HB_OS_WIN ) && ! defined( HB_OS_WIN_CE )
      {
         typedef DWORD ( WINAPI * _HB_GETFINALPATHNAMEBYHANDLE )( HANDLE, LPTSTR, DWORD, DWORD );

         static _HB_GETFINALPATHNAMEBYHANDLE s_pGetFinalPathNameByHandle = ( _HB_GETFINALPATHNAMEBYHANDLE ) -1;

         #ifndef VOLUME_NAME_DOS
         #define VOLUME_NAME_DOS       0x0
         #endif
         #ifndef VOLUME_NAME_GUID
         #define VOLUME_NAME_GUID      0x1
         #endif
         #ifndef VOLUME_NAME_NT
         #define VOLUME_NAME_NT        0x2
         #endif
         #ifndef VOLUME_NAME_NONE
         #define VOLUME_NAME_NONE      0x4
         #endif
         #ifndef FILE_NAME_NORMALIZED
         #define FILE_NAME_NORMALIZED  0x0
         #endif
         #ifndef FILE_NAME_OPENED
         #define FILE_NAME_OPENED      0x8
         #endif

         if( s_pGetFinalPathNameByHandle == ( _HB_GETFINALPATHNAMEBYHANDLE ) -1 )
         {
            HMODULE hModule = GetModuleHandle( TEXT( "kernel32.dll" ) );
            if( hModule )
               s_pGetFinalPathNameByHandle = ( _HB_GETFINALPATHNAMEBYHANDLE )
                  HB_WINAPI_GETPROCADDRESST( hModule, "GetFinalPathNameByHandle" );
            else
               s_pGetFinalPathNameByHandle = NULL;
         }

         if( s_pGetFinalPathNameByHandle )
         {
            LPCTSTR lpFileName;
            LPTSTR lpFileNameFree;
            HANDLE hFile;
            DWORD dwAttr;
            HB_BOOL fDir;

            lpFileName = HB_FSNAMECONV( pszFile, &lpFileNameFree );

            dwAttr = GetFileAttributes( lpFileName );
            fDir = ( dwAttr != INVALID_FILE_ATTRIBUTES ) &&
                   ( dwAttr & FILE_ATTRIBUTE_DIRECTORY );

            hFile = CreateFile( lpFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                fDir ? ( FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS ) : FILE_ATTRIBUTE_NORMAL,
                                NULL );

            if( hFile == INVALID_HANDLE_VALUE )
               hb_fsSetIOError( HB_FALSE, 0 );
            else
            {
               DWORD size;
               TCHAR lpLink[ HB_PATH_MAX ];
               size = s_pGetFinalPathNameByHandle( hFile, lpLink, HB_PATH_MAX, VOLUME_NAME_DOS );
               if( size < HB_PATH_MAX )
               {
                  if( size > 0 )
                  {
                     lpLink[ size ] = TEXT( '\0' );
                     pszLink = HB_OSSTRDUP( lpLink );
                  }

                  hb_fsSetIOError( HB_TRUE, 0 );
               }
               else
                  hb_fsSetError( 9 );
            }

            if( lpFileNameFree )
               hb_xfree( lpFileNameFree );
         }
         else
            hb_fsSetError( 1 );
      }
#elif defined( HB_OS_UNIX )
      {
         char * pszFileFree;
         size_t size;

         pszFile = hb_fsNameConv( pszFile, &pszFileFree );

         pszLink = ( char * ) hb_xgrab( HB_PATH_MAX + 1 );
         size = readlink( pszFile, pszLink, HB_PATH_MAX );
         hb_fsSetIOError( size != ( size_t ) -1, 0 );
         if( size == ( size_t ) -1 )
         {
            hb_xfree( pszLink );
            pszLink = NULL;
         }
         else
         {
            pszLink[ size ] = '\0';
            /* Convert from OS codepage */
            pszLink = ( char * ) HB_UNCONST( hb_osDecodeCP( pszLink, NULL, NULL ) );
         }

         if( pszFileFree )
            hb_xfree( pszFileFree );
      }
#else
      {
         hb_fsSetError( 1 );
      }
#endif

      hb_vmLock();
   }
   else
      hb_fsSetError( 2 );

   return pszLink;
}

HB_FUNC( HB_FLINK )
{
   HB_ERRCODE uiError = 2;
   HB_BOOL fResult = HB_FALSE;
   const char * pszExisting = hb_parc( 1 ), * pszNewFile = hb_parc( 2 );

   if( pszExisting && pszNewFile )
   {
      fResult = hb_fsLink( pszExisting, pszNewFile );
      uiError = hb_fsError();
   }
   hb_retni( fResult ? 0 : F_ERROR );
   hb_fsSetFError( uiError );
}

HB_FUNC( HB_FLINKSYM )
{
   HB_ERRCODE uiError = 2;
   HB_BOOL fResult = HB_FALSE;
   const char * pszTarget = hb_parc( 1 ), * pszNewFile = hb_parc( 2 );

   if( pszTarget && pszNewFile )
   {
      fResult = hb_fsLinkSym( pszTarget, pszNewFile );
      uiError = hb_fsError();
   }
   hb_retni( fResult ? 0 : F_ERROR );
   hb_fsSetFError( uiError );
}

HB_FUNC( HB_FLINKREAD )
{
   HB_ERRCODE uiError = 2;
   char * pszResult = NULL;
   const char * pszFile = hb_parc( 1 );

   if( pszFile )
   {
      pszResult = hb_fsLinkRead( pszFile );
      uiError = hb_fsError();
   }
   hb_retc_buffer( pszResult );
   hb_fsSetFError( uiError );
}
