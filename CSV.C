/* 
 * CSV.C -- Comma-separated value parser and generator.
 * Charles Engelke
 * Modified by Mark Hampton to run under Windows
 * Copyright (C) 1989, Info Tech, Inc.
 */

#include <string.h>

#include "csv.h"


#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

static PSTR AdvanceAField (PSTR pszString);
static PSTR AdvanceQuotedField (PSTR pszString);
static void ConvertAField (PSTR pszString, PSTR pResult);
static void InsertChar (char c, PSTR pszString);



PSTR GetCSVField (int iField, PSTR pCSVString, PSTR sResult)
   {
   int   iFieldNumber;

   /* Grammar for pCSVString is:
      pCSVString     = Field {',' Field} .
      Field          = Empty | NonQuotedField | QuotedField .
      NonQuotedField = { NonQuoteComma } .
      QuotedField    = '"' { NonQuoteComma | ',' | '""' } '"' .
      Empty          = .
      NonQuoteComma  = anything other than '"' or ',' .
   */

   for (iFieldNumber = 1; iFieldNumber < iField; iFieldNumber++)
      {
      pCSVString = AdvanceAField (pCSVString);
      if (*pCSVString != '\0')
         pCSVString++;
      }

   ConvertAField (pCSVString, sResult);

   return sResult;
   }


static PSTR AdvanceAField (PSTR pszString)
   {
   switch (*pszString)
      {
      case ',':
      case '\0':
         return pszString;
         break;

      case '"':
         return AdvanceQuotedField (pszString);
         break;

      default:
         while (*pszString != ',' && *pszString != '\0')
            pszString++;
         return pszString;
         break;
      }

   return pszString;
   }



static PSTR AdvanceQuotedField (PSTR pszString)
   {
   pszString++;

   while (TRUE)
      {
      switch (*pszString)
         {
         case '\0':
            return pszString;
            break;

         case '"':
            if (*(pszString+1) == '"')
               pszString += 2;
            else
               {
               pszString++;
               return pszString;
               }
            break;

         default:
            pszString++;
            break;
         }
      }
   return pszString;
   }



static void ConvertAField (PSTR pszString, PSTR pResult)
   {
   switch (*pszString)
      {
      case ',':
      case '\0':
         *pResult = '\0';
         return;
         break;

      case '"':
         pszString++;

         while (TRUE)
            {
            switch (*pszString)
               {
               case '\0':
                  *pResult = '\0';
                  return;
                  break;

               case '"':
                  if (*(pszString+1) == '"')
                     {
                     pszString += 2;
                     *pResult = '"';
                     pResult++;
                     }
                  else
                     {
                     pszString++;
                     *pResult = '\0';
                     return;
                     }
                  break;

               default:
                  *pResult = *pszString;
                  pszString++;
                  pResult++;
                  break;
               }
            }
         break;

      default:
         while (*pszString != ',' && *pszString != '\0')
            {
            *pResult = *pszString;
            pszString++;
            pResult++;
            }
         *pResult = '\0';
         return;
         break;
      }
   }



PSTR MakeCSVField (PSTR pszString, PSTR pResult)
   {
   BOOL bNeedsQuotes;
   PSTR pChar;

   bNeedsQuotes = FALSE;

   strcpy (pResult, pszString);

   for (pChar = pResult; '\0' != *pChar; pChar++)
      {
      if ('"' == *pChar)
         {
         InsertChar ('"', pChar);
         pChar++;
         bNeedsQuotes = TRUE;
         }
      if (',' == *pChar)
         bNeedsQuotes = TRUE;
      }

   if (bNeedsQuotes)
      {
      *pChar = '"';
      *(pChar+1) = '\0';
      InsertChar ('"', pResult);
      }

   return pResult;
   }



static void InsertChar (char c, PSTR pszString)
   {
   PSTR pTemp;

   pTemp = pszString;

   while ('\0' != *pTemp)
      pTemp++;

   *(pTemp+1) = '\0';

   while (pTemp >= pszString)
      {
      *(pTemp+1) = *pTemp;
      pTemp--;
      }

   *pszString = c;
   }
