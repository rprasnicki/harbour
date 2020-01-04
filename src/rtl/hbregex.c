/*
 * Regex functions
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 * Copyright 2015 Viktor Szakats (vszakats.net/harbour) (PCRE2 support)
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

#define _HB_REGEX_INTERNAL_
#include "hbapi.h"
#include "hbregex.h"
#include "hbapicdp.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbinit.h"
#if defined( HB_HAS_PCRE2 )
#include "hbvm.h"
#endif

#if defined( HB_HAS_PCRE ) || defined( HB_HAS_PCRE2 )
   static int s_iUTF8Enabled;
#endif

#if defined( HB_HAS_PCRE2 )
   static pcre2_general_context * s_re_ctxg;
   static pcre2_compile_context * s_re_ctxc;
   static pcre2_match_context *   s_re_ctxm;
#endif

static void hb_regfree( PHB_REGEX pRegEx )
{
#if defined( HB_HAS_PCRE2 )
   pcre2_code_free( pRegEx->re_pcre );
#elif defined( HB_HAS_PCRE )
   ( pcre_free )( pRegEx->re_pcre );
#elif defined( HB_POSIX_REGEX )
   regfree( &pRegEx->reg );
#else
   HB_SYMBOL_UNUSED( pRegEx );
#endif
}

static int hb_regcomp( PHB_REGEX pRegEx, const char * szRegEx )
{
#if defined( HB_HAS_PCRE2 )
   int iError = 0;
   PCRE2_SIZE iErrOffset = 0;
   HB_U32 uiCFlags = ( ( pRegEx->iFlags & HBREG_ICASE   ) ? PCRE2_CASELESS  : 0 ) |
                     ( ( pRegEx->iFlags & HBREG_NEWLINE ) ? PCRE2_MULTILINE : 0 ) |
                     ( ( pRegEx->iFlags & HBREG_DOTALL  ) ? PCRE2_DOTALL    : 0 );

   pRegEx->iEFlags = ( ( pRegEx->iFlags & HBREG_NOTBOL ) ? PCRE2_NOTBOL : 0 ) |
                     ( ( pRegEx->iFlags & HBREG_NOTEOL ) ? PCRE2_NOTEOL : 0 );

   /* use UTF-8 in PCRE2 when available and HVM CP is also UTF-8. */
   if( s_iUTF8Enabled && hb_cdpIsUTF8( NULL ) )
      uiCFlags |= PCRE2_UTF;

   pRegEx->re_pcre = pcre2_compile( ( PCRE2_SPTR ) szRegEx,
                                    ( PCRE2_SIZE ) strlen( szRegEx ),
                                    uiCFlags,
                                    &iError,
                                    &iErrOffset, s_re_ctxc );
   return pRegEx->re_pcre ? 0 : -1;
#elif defined( HB_HAS_PCRE )
   const unsigned char * pCharTable = NULL;
   const char * szError = NULL;
   int iErrOffset = 0;
   int iCFlags = ( ( pRegEx->iFlags & HBREG_ICASE   ) ? PCRE_CASELESS  : 0 ) |
                 ( ( pRegEx->iFlags & HBREG_NEWLINE ) ? PCRE_MULTILINE : 0 ) |
                 ( ( pRegEx->iFlags & HBREG_DOTALL  ) ? PCRE_DOTALL    : 0 );

   pRegEx->iEFlags = ( ( pRegEx->iFlags & HBREG_NOTBOL ) ? PCRE_NOTBOL : 0 ) |
                     ( ( pRegEx->iFlags & HBREG_NOTEOL ) ? PCRE_NOTEOL : 0 );

   /* use UTF-8 in PCRE1 when available and HVM CP is also UTF-8. */
   if( s_iUTF8Enabled && hb_cdpIsUTF8( NULL ) )
      iCFlags |= PCRE_UTF8;

   pRegEx->re_pcre = pcre_compile( szRegEx, iCFlags, &szError,
                                   &iErrOffset, pCharTable );
   return pRegEx->re_pcre ? 0 : -1;
#elif defined( HB_POSIX_REGEX )
   int iCFlags = REG_EXTENDED |
                 ( ( pRegEx->iFlags & HBREG_ICASE   ) ? REG_ICASE   : 0 ) |
                 ( ( pRegEx->iFlags & HBREG_NEWLINE ) ? REG_NEWLINE : 0 ) |
                 ( ( pRegEx->iFlags & HBREG_NOSUB   ) ? REG_NOSUB   : 0 );
   pRegEx->iEFlags = ( ( pRegEx->iFlags & HBREG_NOTBOL ) ? REG_NOTBOL : 0 ) |
                     ( ( pRegEx->iFlags & HBREG_NOTEOL ) ? REG_NOTEOL : 0 );
   return regcomp( &pRegEx->reg, szRegEx, iCFlags );
#else
   HB_SYMBOL_UNUSED( pRegEx );
   HB_SYMBOL_UNUSED( szRegEx );
   return -1;
#endif
}

static int hb_regexec( PHB_REGEX pRegEx, const char * szString, HB_SIZE nLen,
                       int iMatches, HB_REGMATCH * aMatches )
{
#if defined( HB_HAS_PCRE2 )
   PCRE2_SIZE iResult = pcre2_match( pRegEx->re_pcre,
                                     ( PCRE2_SPTR ) szString,
                                     ( PCRE2_SIZE ) nLen,
                                     ( PCRE2_SIZE ) 0 /* startoffset */,
                                     ( HB_U32 ) pRegEx->iEFlags,
                                     aMatches, s_re_ctxm );
   if( iResult == 0 )
   {
      PCRE2_SIZE i;
      for( i = 0; i < ( PCRE2_SIZE ) iMatches; i++ )
      {
         if( HB_REGMATCH_EO( aMatches, i ) != HB_REGMATCH_UNSET )
            iResult = i + 1;
      }
   }
   return ( int ) iResult;
#elif defined( HB_HAS_PCRE )
   int iResult = pcre_exec( pRegEx->re_pcre, NULL /* pcre_extra */,
                            szString, ( int ) nLen, 0 /* startoffset */,
                            pRegEx->iEFlags, aMatches, HB_REGMATCH_SIZE( iMatches ) );
   if( iResult == 0 )
   {
      int i;
      for( i = 0; i < iMatches; i++ )
      {
         if( HB_REGMATCH_EO( aMatches, i ) != HB_REGMATCH_UNSET )
            iResult = i + 1;
      }
   }
   return iResult;
#elif defined( HB_POSIX_REGEX )
   char * szBuffer = NULL;
   int iResult, i;

   if( szString[ nLen ] != 0 )
   {
      szBuffer = hb_strndup( szString, nLen );
      szString = szBuffer;
   }
   for( i = 0; i < iMatches; i++ )
      HB_REGMATCH_EO( aMatches, i ) = HB_REGMATCH_UNSET;
   iResult = regexec( &pRegEx->reg, szString, iMatches, aMatches, pRegEx->iEFlags );
   if( iResult == 0 )
   {
      for( i = 0; i < iMatches; i++ )
      {
         if( HB_REGMATCH_EO( aMatches, i ) != HB_REGMATCH_UNSET )
            iResult = i + 1;
      }
   }
   else
      iResult = -1;
   if( szBuffer )
      hb_xfree( szBuffer );
   return iResult;
#else
   HB_SYMBOL_UNUSED( pRegEx );
   HB_SYMBOL_UNUSED( szString );
   HB_SYMBOL_UNUSED( nLen );
   HB_SYMBOL_UNUSED( iMatches );
   HB_SYMBOL_UNUSED( aMatches );
   return -1;
#endif
}


HB_FUNC( HB_REGEXCOMP )
{
   HB_SIZE nLen = hb_parclen( 1 );

   if( nLen == 0 )
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
   else
   {
      int iFlags = HBREG_EXTENDED;
      PHB_REGEX pRegEx;

      if( ! hb_parldef( 2, HB_TRUE ) )
         iFlags |= HBREG_ICASE;
      if( hb_parl( 3 ) )
         iFlags |= HBREG_NEWLINE;

      pRegEx = hb_regexCompile( hb_parc( 1 ), nLen, iFlags );
      if( pRegEx )
      {
         pRegEx->fFree = HB_FALSE;
         hb_retptrGC( pRegEx );
      }
   }
}

HB_FUNC( HB_ISREGEX )
{
   hb_retl( hb_regexIs( hb_param( 1, HB_IT_ANY ) ) );
}

HB_FUNC( HB_ATX )
{
   PHB_ITEM pString = hb_param( 2, HB_IT_STRING );

   if( pString )
   {
      PHB_REGEX pRegEx = hb_regexGet( hb_param( 1, HB_IT_ANY ),
                                      ! hb_parldef( 3, HB_TRUE ) ? HBREG_ICASE : 0 );

      if( pRegEx )
      {
         HB_SIZE nLen = hb_itemGetCLen( pString );
         HB_SIZE nStart = hb_parns( 4 );
         HB_SIZE nEnd = hb_parnsdef( 5, nLen );

         if( nLen && nStart <= nLen && nStart <= nEnd )
         {
#if defined( HB_HAS_PCRE2 )
            HB_REGMATCH * aMatches = pcre2_match_data_create( 1, NULL );

            if( aMatches )
            {
#else
            HB_REGMATCH aMatches[ HB_REGMATCH_SIZE( 1 ) ];
#endif
               const char * pszString = hb_itemGetCPtr( pString );

               if( nEnd < nLen )
                  nLen = nEnd;
               if( nStart )
               {
                  --nStart;
                  nLen -= nStart;
               }

               if( hb_regexec( pRegEx, pszString + nStart, nLen, 1, aMatches ) > 0 )
               {
                  nStart += ( HB_SIZE ) HB_REGMATCH_SO( aMatches, 0 ) + 1;
                  nLen = ( HB_SIZE ) HB_REGMATCH_EO( aMatches, 0 ) -
                         ( HB_SIZE ) HB_REGMATCH_SO( aMatches, 0 );
                  hb_retclen( pszString + nStart - 1, nLen );
               }
               else
                  nStart = nLen = 0;

#if defined( HB_HAS_PCRE2 )
               pcre2_match_data_free( aMatches );
            }
            else
               nStart = nLen = 0;
#endif
         }
         else
            nStart = nLen = 0;

         hb_regexFree( pRegEx );

         hb_storns( nStart, 4 );
         hb_storns( nLen, 5 );
      }
      else
      {
         hb_storns( 0, 4 );
         hb_storns( 0, 5 );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3013, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

static HB_BOOL hb_regex( int iRequest )
{
#if defined( HB_HAS_PCRE2 )
   HB_REGMATCH * aMatches;
#else
   HB_REGMATCH aMatches[ HB_REGMATCH_SIZE( REGEX_MAX_GROUPS ) ];
#endif
   PHB_ITEM pRetArray, pString;
   int iMatches, iMaxMatch;
   HB_BOOL fResult = HB_FALSE;
   PHB_REGEX pRegEx;
   const char * pszString;
   HB_SIZE nLen;

   pString = hb_param( 2, HB_IT_STRING );
   if( ! pString )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3014, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
      return HB_FALSE;
   }
   pRegEx = hb_regexGet( hb_param( 1, HB_IT_ANY ),
                         ( ! hb_parldef( 3, HB_TRUE ) ? HBREG_ICASE : 0 ) |
                         ( hb_parl( 4 ) ? HBREG_NEWLINE : 0 ) );
   if( ! pRegEx )
      return HB_FALSE;

#if defined( HB_HAS_PCRE2 )
   aMatches = pcre2_match_data_create( REGEX_MAX_GROUPS, NULL );
   if( ! aMatches )
      return HB_FALSE;
#endif

   pszString = hb_itemGetCPtr( pString );
   nLen      = hb_itemGetCLen( pString );
   iMaxMatch = iRequest == 0 || iRequest == 4 || iRequest == 5 ?
               REGEX_MAX_GROUPS : 1;
   iMatches = hb_regexec( pRegEx, pszString, nLen, iMaxMatch, aMatches );
   if( iMatches > 0 )
   {
      PHB_ITEM pMatch;
      int i;

      switch( iRequest )
      {
         case 0:
            pRetArray = hb_itemArrayNew( iMatches );
            for( i = 0; i < iMatches; i++ )
            {
               if( HB_REGMATCH_EO( aMatches, i ) != HB_REGMATCH_UNSET )
                  hb_arraySetCL( pRetArray, i + 1,
                                 pszString + ( HB_SIZE ) HB_REGMATCH_SO( aMatches, i ),
                                 ( HB_SIZE ) HB_REGMATCH_EO( aMatches, i ) -
                                 ( HB_SIZE ) HB_REGMATCH_SO( aMatches, i ) );
               else
                  hb_arraySetCL( pRetArray, i + 1, NULL, 0 );
            }
            hb_itemReturnRelease( pRetArray );
            fResult = HB_TRUE;
            break;

         case 1: /* LIKE */
            fResult = HB_REGMATCH_SO( aMatches, 0 ) == 0 &&
                      ( HB_SIZE ) HB_REGMATCH_EO( aMatches, 0 ) == nLen;
            break;

         case 2: /* HAS */
            fResult = HB_TRUE;
            break;

         case 3: /* SPLIT */
            iMaxMatch = hb_parni( 5 );
            pRetArray = hb_itemArrayNew( 0 );
            pMatch = hb_itemNew( NULL );
            iMatches = 0;
            do
            {
               hb_itemPutCL( pMatch, pszString, ( HB_SIZE ) HB_REGMATCH_SO( aMatches, 0 ) );
               hb_arrayAddForward( pRetArray, pMatch );
               nLen -= ( HB_SIZE ) HB_REGMATCH_EO( aMatches, 0 );
               pszString += ( HB_SIZE ) HB_REGMATCH_EO( aMatches, 0 );
               iMatches++;
            }
            while( HB_REGMATCH_EO( aMatches, 0 ) > 0 && nLen &&
                   ( iMaxMatch == 0 || iMatches < iMaxMatch ) &&
                   hb_regexec( pRegEx, pszString, nLen, 1, aMatches ) > 0 );

            /* last match must be done also in case that pszString is empty;
               this would mean an empty split field at the end of the string */
#if 0
            if( nLen )
#endif
            {
               hb_itemPutCL( pMatch, pszString, nLen );
               hb_arrayAddForward( pRetArray, pMatch );
            }
            hb_itemRelease( pMatch );

            hb_itemReturnRelease( pRetArray );
            fResult = HB_TRUE;
            break;

         case 4: /* results AND positions */
            pRetArray = hb_itemArrayNew( iMatches );

            for( i = 0; i < iMatches; i++ )
            {
               HB_SIZE nSO = ( HB_SIZE ) HB_REGMATCH_SO( aMatches, i ),
                       nEO = ( HB_SIZE ) HB_REGMATCH_EO( aMatches, i );
               pMatch = hb_arrayGetItemPtr( pRetArray, i + 1 );
               hb_arrayNew( pMatch, 3 );
               if( nEO != ( HB_SIZE ) HB_REGMATCH_UNSET )
               {
                  hb_arraySetCL( pMatch, 1, pszString + nSO, nEO - nSO );  /* matched string */
                  hb_arraySetNS( pMatch, 2, nSO + 1 );  /* begin of match */
                  hb_arraySetNS( pMatch, 3, nEO );  /* End of match */
               }
               else
               {
                  hb_arraySetCL( pMatch, 1, NULL, 0 );
                  hb_arraySetNS( pMatch, 2, 0 );
                  hb_arraySetNS( pMatch, 3, 0 );
               }
            }
            hb_itemReturnRelease( pRetArray );
            fResult = HB_TRUE;
            break;

         case 5: /* _ALL_ results AND positions */
         {
            PHB_ITEM pAtxArray;
            int      iMax       = hb_parni( 5 );  /* Max number of matches I want, 0 = unlimited */
            int      iGetMatch  = hb_parni( 6 );  /* Gets if want only one single match or a sub-match */
            HB_BOOL  fOnlyMatch = hb_parldef( 7, HB_TRUE );  /* If HB_TRUE, returns only matches and sub-matches, not positions */
            HB_SIZE  nOffset    = 0;
            int      iCount     = 0;
            HB_SIZE  nSO, nEO;

            /* Set new array */
            pRetArray = hb_itemArrayNew( 0 );
            do
            {
               /* If I want all matches */
               if( iGetMatch == 0 || /* Check boundaries */
                   ( iGetMatch < 0 || iGetMatch > iMatches ) )
               {
                  pAtxArray = hb_itemArrayNew( iMatches );
                  for( i = 0; i < iMatches; i++ )
                  {
                     nSO = ( HB_SIZE ) HB_REGMATCH_SO( aMatches, i );
                     nEO = ( HB_SIZE ) HB_REGMATCH_EO( aMatches, i );
                     pMatch = hb_arrayGetItemPtr( pAtxArray, i + 1 );
                     if( ! fOnlyMatch )
                     {
                        hb_arrayNew( pMatch, 3 );
                        if( nEO != ( HB_SIZE ) HB_REGMATCH_UNSET )
                        {
                           hb_arraySetCL( pMatch, 1, pszString + nSO, nEO - nSO );  /* matched string */
                           hb_arraySetNS( pMatch, 2, nOffset + nSO + 1 );  /* begin of match */
                           hb_arraySetNS( pMatch, 3, nOffset + nEO );  /* End of match */
                        }
                        else
                        {
                           hb_arraySetCL( pMatch, 1, NULL, 0 );
                           hb_arraySetNS( pMatch, 2, 0 );
                           hb_arraySetNS( pMatch, 3, 0 );
                        }
                     }
                     else
                     {
                        if( nEO != ( HB_SIZE ) HB_REGMATCH_UNSET )
                           hb_itemPutCL( pMatch, pszString + nSO, nEO - nSO );  /* matched string */
                        else
                           hb_itemPutC( pMatch, NULL );
                     }
                  }
                  hb_arrayAddForward( pRetArray, pAtxArray );
                  hb_itemRelease( pAtxArray );
               }
               else /* Here I get only single matches */
               {
                  i = iGetMatch - 1;
                  nSO = ( HB_SIZE ) HB_REGMATCH_SO( aMatches, i );
                  nEO = ( HB_SIZE ) HB_REGMATCH_EO( aMatches, i );
                  pMatch = hb_itemNew( NULL );
                  if( ! fOnlyMatch )
                  {
                     hb_arrayNew( pMatch, 3 );
                     if( nEO != ( HB_SIZE ) HB_REGMATCH_UNSET )
                     {
                        hb_arraySetCL( pMatch, 1, pszString + nSO, nEO - nSO );  /* matched string */
                        hb_arraySetNS( pMatch, 2, nOffset + nSO + 1 );  /* begin of match */
                        hb_arraySetNS( pMatch, 3, nOffset + nEO );  /* End of match */
                     }
                     else
                     {
                        hb_arraySetCL( pMatch, 1, NULL, 0 );
                        hb_arraySetNS( pMatch, 2, 0 );
                        hb_arraySetNS( pMatch, 3, 0 );
                     }
                  }
                  else
                  {
                     if( nEO != ( HB_SIZE ) HB_REGMATCH_UNSET )
                        hb_itemPutCL( pMatch, pszString + nSO, nEO - nSO );  /* matched string */
                     else
                        hb_itemPutC( pMatch, NULL );
                  }
                  hb_arrayAddForward( pRetArray, pMatch );
                  hb_itemRelease( pMatch );
               }

               nEO = ( HB_SIZE ) HB_REGMATCH_EO( aMatches, 0 );
               if( nEO == ( HB_SIZE ) HB_REGMATCH_UNSET )
                  break;
               nLen -= nEO;
               pszString += nEO;
               nOffset += nEO;
               iCount++;
            }
            while( nEO && nLen && ( iMax == 0 || iCount < iMax ) &&
                   ( iMatches = hb_regexec( pRegEx, pszString, nLen, iMaxMatch, aMatches ) ) > 0 );
            hb_itemReturnRelease( pRetArray );
            fResult = HB_TRUE;
            break;
         }
      }
   }
   else if( iRequest == 3 )
   {
      pRetArray = hb_itemArrayNew( 1 );
      hb_arraySet( pRetArray, 1, pString );
      hb_itemReturnRelease( pRetArray );
      fResult = HB_TRUE;
   }

#if defined( HB_HAS_PCRE2 )
   pcre2_match_data_free( aMatches );
#endif

   hb_regexFree( pRegEx );
   return fResult;
}

/* Returns array of Match + Sub-Matches. */
HB_FUNC( HB_REGEX )
{
   if( ! hb_regex( 0 ) )
      hb_reta( 0 );
}

/* Returns just .T. if match found or .F. otherwise. */
/* NOTE: Deprecated compatibility function.
         Please use hb_regexLike() and hb_regexHas() instead. */


HB_FUNC( HB_REGEXLIKE )
{
   hb_retl( hb_regex( 1 ) );
}

HB_FUNC( HB_REGEXHAS )
{
   hb_retl( hb_regex( 2 ) );
}

/* Splits the string in an array of matched expressions */
HB_FUNC( HB_REGEXSPLIT )
{
   if( ! hb_regex( 3 ) )
      hb_reta( 0 );
}

/* Returns array of { Match, start, end }, { Sub-Matches, start, end } */
HB_FUNC( HB_REGEXATX )
{
   if( ! hb_regex( 4 ) )
      hb_reta( 0 );
}

/* 2005-12-16 - Francesco Saverio Giudice
   hb_regexAll( cRegex, cString, lCaseSensitive, lNewLine, nMaxMatches, nGetMatch, lOnlyMatch ) --> aAllRegexMatches

   This function return all matches from a Regex search.
   It is a mix from hb_regex() and hb_regexAtX()

   PARAMETERS:
    cRegex         - Regex pattern string or precompiled Regex
    cString        - The string you want to search
    lCaseSensitive - default = FALSE
    lNewLine       - default = FALSE
    nMaxMatches    - default = unlimited, this limit number of matches that have to return
    nGetMatch      - default = unlimited, this returns only one from Match + Sub-Matches
    lOnlyMatch     - default = TRUE, if TRUE returns Matches, otherwise it returns also start and end positions
 */

HB_FUNC( HB_REGEXALL )
{
   if( ! hb_regex( 5 ) )
      hb_reta( 0 );
}

#if defined( HB_HAS_PCRE2 )
static void * hb_pcre2_grab( PCRE2_SIZE size, void * data )
{
   HB_SYMBOL_UNUSED( data );
   return size > 0 ? hb_xgrab( size ) : NULL;
}
static void hb_pcre2_free( void * ptr, void * data )
{
   HB_SYMBOL_UNUSED( data );
   if( ptr )
      hb_xfree( ptr );
}
static void hb_pcre2_exit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   pcre2_match_context_free( s_re_ctxm );
   pcre2_compile_context_free( s_re_ctxc );
   pcre2_general_context_free( s_re_ctxg );
}
#elif defined( HB_HAS_PCRE )
static void * hb_pcre_grab( size_t size )
{
   return size > 0 ? hb_xgrab( size ) : NULL;
}
static void hb_pcre_free( void * ptr )
{
   if( ptr )
      hb_xfree( ptr );
}
#endif

HB_CALL_ON_STARTUP_BEGIN( _hb_regex_init_ )
#if defined( HB_HAS_PCRE2 )
   /* detect UTF-8 support. */
   if( pcre2_config( PCRE2_CONFIG_UNICODE, &s_iUTF8Enabled ) != 0 )
      s_iUTF8Enabled = 0;

   s_re_ctxg = pcre2_general_context_create( hb_pcre2_grab, hb_pcre2_free, NULL );
   s_re_ctxc = pcre2_compile_context_create( s_re_ctxg );
   s_re_ctxm = pcre2_match_context_create( s_re_ctxg );

   hb_vmAtExit( hb_pcre2_exit, NULL );
#elif defined( HB_HAS_PCRE )
   /* detect UTF-8 support.
    * In BCC builds this code also forces linking newer PCRE versions
    * then the one included in BCC RTL.
    */
   if( pcre_config( PCRE_CONFIG_UTF8, &s_iUTF8Enabled ) != 0 )
      s_iUTF8Enabled = 0;

   pcre_malloc = hb_pcre_grab;
   pcre_free = hb_pcre_free;
   pcre_stack_malloc = hb_pcre_grab;
   pcre_stack_free = hb_pcre_free;
#endif
   hb_regexInit( hb_regfree, hb_regcomp, hb_regexec );
HB_CALL_ON_STARTUP_END( _hb_regex_init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hb_regex_init_
#elif defined( HB_DATASEG_STARTUP )
   #define HB_DATASEG_BODY    HB_DATASEG_FUNC( _hb_regex_init_ )
   #include "hbiniseg.h"
#endif
