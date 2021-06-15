/* PrintC - File Print Utility
 * Craig Fitzgerald
 * (c) 1990,  Info Tech, Inc.
 */

#include "ostype.h"
#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>
#include "csv.h"
#include "arg2.h"

#define TIME __TIME__
#define DATE __DATE__

//---- in ostype.h ----
//#define  TRUE           1
//#define  FALSE          0
//#define  BOOL           int
//typedef  char  *PSZ;
//typedef  unsigned char CHAR;

#define  CONFIGFILE     "PRINTC."
#define  ENVIRONMENTVAR "PRINTC"
#define  STRLEN         512
#define  MSGLEN         120
#define  InitCodes      0
#define  HeaderCodes    1
#define  BodyCodes      2
#define  FooterCodes    3
#define  TermCodes      4
#define  LineNumberSize 6
#define  HeaderSize     2
#define  FooterSize     2

typedef  void  *PV;

/* globals set to default values */
int  PAGELENGTH   = 64;
int  HEADERWIDTH  = 80;
int  BODYWIDTH    = 80;
int  FOOTERWIDTH  = 80;
int  LEFTMARGIN   = 0;
int  RIGHTMARGIN  = 0;
int  TOPLINES     = 0;
int  ChopMark1    = 0;
int  ChopMark2    = 1;
int  ChopTotal    = 1;
int  STARTPAGE    = 1;
int  ENDPAGE      = 32767;

BOOL TRUNCATE     = FALSE;
BOOL EVEN         = TRUE;
BOOL ODD          = TRUE;
BOOL IGNOREFF     = FALSE;
BOOL LINENUMBERS  = FALSE;
BOOL HEADER       = FALSE;
BOOL FOOTER       = FALSE;
BOOL PAGEFF       = TRUE;

char  szOutFileName [STRLEN] = "PRN";
char  szMessage     [STRLEN] = "(c) 1992   Info Tech, Inc.";
char  szCodes    [5][STRLEN] = { { 0,0 }, { 0,0 }, { 0,0 }, { 0,0 }, { 0,0 }};
char  szFileName    [STRLEN];
char  szExtension   [40] = "CFG";

#undef toupper
#undef tolower

void Usage (void)
   {
   printf ("PRINTC          file printing utility  v1.1       %s  %s\n", TIME, DATE);   
   printf (" \n");
   printf ("USAGE: PRINTC  [options] [@ConfigFile] [#Extension] FileNames\n");
   printf (" \n");
   printf ("WHERE: FileNames .... The list of files to print (wildcards ok)\n");
   printf (" \n");
   printf ("       @ConfigFile .. The configuration file which, if not specified here,\n");
   printf ("                       must be named PRINTC.CFG and must be located in either\n");
   printf ("                       the current dir, the location specified by path variable\n");
   printf ("                       PRINTC=, or in the same path as the .exe file.\n");
   printf (" \n");
   printf ("       #Extension ... The config file extension override. The config file is\n");
   printf ("                       assumed to have a CFG extension unless overridden here.\n");
   printf ("                       The search order is still Current Dir, Env Dir, Exe Dir\n");
   printf (" \n");
   printf ("       options ...... These override settings in the config file \n");
   printf ("                                                               \n");
   printf ("          -BodyWidth   = #             | These options, and, others may also\n");
   printf ("          -Chop        = #,#,#         | be present in the config file. See\n");
   printf ("          -EndPage     = #             | the documentation in the config\n");
   printf ("          -Even        = True | False  | file PRINTC.CFG for thier use.\n");
   printf ("          -Footer      = True | False  | \n");
   printf ("          -FooterWidth = #             | \n");
   printf ("          -Header      = True | False  | These options may be minimally\n");
   printf ("          -HeaderWidth = #             | specified on the command line. I.E.\n");
   printf ("          -IgnoreFF    = True | False  | -le=5 is legal because le will\n");
   printf ("          -LeftMargin  = #             | match exactly one parameter.\n");
   printf ("          -LineNumbers = True | False  |                        \n");
   printf ("          -Odd         = True | False  |                        \n");
   printf ("          -OutFile     = filespec      |                        \n");
   printf ("          -PageFF      = True | False  |                        \n");
   printf ("          -PageLength  = #             |                        \n");
   printf ("          -RightMargin = #             |                        \n");
   printf ("          -StartPage   = #             |                        \n");
   printf ("          -TopLines    = #             |                        \n");
   printf ("          -Truncate    = True | False  |                        \n");
   printf ("                        \n");
   exit (1);
   }



void PrintControlString(FILE *fpOut, int j)
   {
   int i = 1;

   for (; i <= szCodes[j][0]; i++)
      fprintf (fpOut,"%c",szCodes[j][i]);
   }


void Error (char *psz)
   {
   printf ("Error %s\n", psz);
   exit (1);
   }


void ReadCodes (PSZ pszStr, int j)
   {
   int    uCount = 0;
   CHAR   ch, chr;


   /*--- skip leading space ---*/
   while (*pszStr == ' ')
      pszStr++;

   while (1)
      {
      ch = *pszStr++;

      if (!ch || ch == '\n')
         break;

      if (ch == ' ')
         continue;

      uCount++;

      if (ch == '\\')       
         {
         if (!*pszStr)
            Error ("bad \\ format: unexpected eol.");

         if (*pszStr < '0' || *pszStr > '9')      /*--- \char ---*/
            chr = *pszStr++;

         else if (toupper (pszStr[2]) == 'H')     /*--- \00h ---*/
            {
            ch = (CHAR) toupper (*pszStr++);
            if (ch < '0' || ch > 'H' || (ch > '9' && ch < 'A'))
               Error ("Invalid char in \\00h format");
            chr = (CHAR) (16 * (ch - (ch > '9' ? 'A' + 10 : '0')));
            ch = (CHAR) toupper (*pszStr++);
            if (ch < '0' || ch > 'H' || (ch > '9' && ch < 'A'))
               Error ("Invalid char in \\00h format");
            chr += (CHAR) (ch - (ch > '9' ? 'A' - 10 : '0'));
            }
         else                                     /*--- \000 ---*/
            {
            if (pszStr[0] < '0' || pszStr[0] > '9'  ||
                pszStr[1] < '0' || pszStr[1] > '9'  ||
                pszStr[2] < '0' || pszStr[2] > '9')
               Error ("Invalid char in \000 format");

            chr  = (CHAR)(100 * ((*pszStr++ - '0') % 10));
            chr += (CHAR)(10  * ((*pszStr++ - '0') % 10));
            chr += (CHAR)(      ((*pszStr++ - '0') % 10));
            }
         }
      else if (ch == '~')
         chr = (CHAR) 27;

      else if (ch == '^')
         {
         ch = (CHAR) toupper (*pszStr++);
         if (ch < 'A' || ch > 'Z')
            Error ("Illegal char in ^<char> format");
         chr = (CHAR)(ch - 'A' + 1);
         }

      else
         chr = ch;

      szCodes[j][uCount] = chr;
      }
   szCodes[j][0] = (CHAR)(uCount);
   }



FILE *OpenCFGFile (PSZ pszArg0, BOOL bCmdName)
   {
   FILE *fpIn;
   char  szPathStr [STRLEN];
   PSZ   p, pszPathStr;
   char  szTmp [512];

   if (bCmdName)
      {
      if ((fpIn = fopen (++pszArg0, "r")) == NULL)
         {
         printf ("ERROR: Unable to load config file %s", pszArg0);
         exit (1);
         }
      else
         {
         printf ("Using %s as configuration file\n", pszArg0);
         return fpIn;
         }
      }

   /* first try cwd */
   strcat (strcpy (szTmp, CONFIGFILE), (*pszArg0=='#' ? pszArg0+1 : "CFG"));
   if ((fpIn = fopen (szTmp, "r")) != NULL)
      {
      printf ("Using current directory to obtain %s\n", szTmp);
      return fpIn;
      }

   /* next try environment variable */
   if ((pszPathStr = getenv (ENVIRONMENTVAR)) != NULL)
      {
      /* copy env var to str */
      pszPathStr = strcpy (szPathStr, pszPathStr);
      while (pszPathStr [strlen (pszPathStr) - 1] == ' ')
         pszPathStr [strlen (pszPathStr) - 1] = '\0';
      while (*pszPathStr == ' ')
         ++pszPathStr;
      if (pszPathStr [strlen (pszPathStr) - 1] == ';')
         pszPathStr [strlen (pszPathStr) - 1] = '\0';

      strcat (strcat (pszPathStr, "\\"), CONFIGFILE);
      strcat (pszPathStr, (*pszArg0=='#' ? pszArg0+1 : "CFG"));

      if ((fpIn = fopen (pszPathStr, "r")) != NULL)
         {
         printf ("Using environment var to obtain %s\n", pszPathStr);
         return fpIn;
         }
      else
         printf ("WARNING: config file not found using environment: %s\n",
                  pszPathStr);
      }
   /* no: next try path of exe file  */
   if (*pszArg0 != '#')
      {
      strcpy (szPathStr, pszArg0);
      p = strchr (szPathStr, '.');
      p[1] = '\0';
      strcat (szPathStr, (*pszArg0=='#' ? pszArg0+1 : "CFG"));
      if ((fpIn = fopen (szPathStr, "r")) != NULL)
         {
         printf ("Using path of executable to obtain %s\n", pszPathStr);
         return fpIn;
         }
      }
   printf ("WARNING: Unable to find config file %s. Using defaults.\n", CONFIGFILE);
   return NULL;
   }




void AddParam (PSZ pszParam, PSZ pszVal)
   {
   char CSVStr [STRLEN];
   int iStrVal;

   if (pszParam == NULL   ||
       pszParam[0] == '\0'||
       pszParam[0] == '#' ||
       pszParam[0] == '\n')
      return;

   iStrVal = atoi (pszVal);
   /********** strings **********/
   if (!strnicmp (pszParam,"message",7))
      strncpy (szMessage, pszVal, MSGLEN);
   else if (!strnicmp (pszParam,"outfile",7))
      strcpy  (szOutFileName, pszVal);
   else if (!strnicmp (pszParam,"extension",9))
      strcpy  (szExtension, pszVal);
   /********** integers **********/
   else if (!strnicmp (pszParam,"pagelength",9))
      PAGELENGTH  = iStrVal;
   else if (!strnicmp (pszParam,"headerwidth",11))
      HEADERWIDTH = iStrVal;
   else if (!strnicmp (pszParam,"bodywidth",9))
      BODYWIDTH = iStrVal;
   else if (!strnicmp (pszParam,"footerwidth",11))
      FOOTERWIDTH = iStrVal;
   else if (!strnicmp (pszParam,"leftmargin",10))
         LEFTMARGIN = iStrVal;
   else if (!strnicmp (pszParam,"rightmargin",11))
         RIGHTMARGIN = iStrVal;
   else if (!strnicmp (pszParam,"toplines",8))
         TOPLINES = iStrVal;
   else if (!strnicmp (pszParam,"startpage",9))
         STARTPAGE = iStrVal;
   else if (!strnicmp (pszParam,"endpage",7))
         ENDPAGE = iStrVal;

   /********** csv **********/
   else if (!strnicmp (pszParam,"initcodes",9))
      ReadCodes (pszVal, InitCodes);
   else if (!strnicmp (pszParam,"headercodes",11))
      ReadCodes (pszVal, HeaderCodes);
   else if (!strnicmp (pszParam,"bodycodes",9))
      ReadCodes (pszVal, BodyCodes);
   else if (!strnicmp (pszParam,"footercodes",11))
      ReadCodes (pszVal, FooterCodes);
   else if (!strnicmp (pszParam,"termcodes",9))
      ReadCodes (pszVal, TermCodes);
   else if (!strnicmp (pszParam,"chop",4))
      {
      ChopMark1 = (char) atoi (GetCSVField (1, pszVal, CSVStr)); 
      ChopMark2 = ChopMark1 + (char) atoi (GetCSVField (2, pszVal, CSVStr));
      ChopTotal = ChopMark2 + (char) atoi (GetCSVField (3, pszVal, CSVStr));
      }
   /********** boolean **********/
   else if (!strnicmp (pszParam,"truncate",8))
      TRUNCATE = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"even",4))
      EVEN = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"odd",3))
      ODD = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"ignoreff",8))
      IGNOREFF = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"linenumbers",11))
      LINENUMBERS = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"header",6))
      HEADER = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"footer",6))
      FOOTER = (toupper(pszVal[0]) == 'T');
   else if (!strnicmp (pszParam,"pageff",6))
      PAGEFF = (toupper(pszVal[0]) == 'T');
   else
      printf ("WARNING: param ignored: %s\n", pszParam);
   }





void LoadConfigFile (PSZ pszArg0,  BOOL bCmdName)
   {
   FILE  *fpIn;
   char  szStr [STRLEN];
   PSZ   pszStrPtr;

   if (!(fpIn = OpenCFGFile (pszArg0, bCmdName)))
      return;

   while (!feof (fpIn))
      {
      fgets (szStr, STRLEN, fpIn);
      if (szStr == NULL || szStr[0] == '#' || szStr[0] == '\n')
         continue;
      szStr [strlen (szStr) -1] = '\0';
      pszStrPtr = (strchr (szStr,'=')) + 1;
      while (*pszStrPtr == ' ')
         ++pszStrPtr;

      AddParam (szStr, pszStrPtr);
      }
   fclose (fpIn);
   }


void ReadCmdLine (void)
   {
   int   i;
   char  szArg[128], szVal[128];

   for (i=0; EnumArg (szArg, szVal, i); i++)
      if (szVal[0]) AddParam (szArg, szVal);
   }


/* returns last char read */
int SkipLine (FILE *fp)
   {
   int   c;

   while ((c = getc (fp)) != '\n' && c != EOF)
         ;
   return c;
   }


/* returns next line (i.e. returns 2 after reading 1st line) */
int GetNextLine (FILE *fp, PSZ pszLine, int iMaxWidth, int iLine)
   {
   int   c;

   for (; (c = getc (fp)) != '\n' && c != EOF && iMaxWidth > 0; iMaxWidth--)
      {
      if (c != '\f' || IGNOREFF)
         *pszLine++ = (char) c;
      }
   *pszLine = '\0';

   /*--- put back the char that we couldn't handle at the moment ---*/
   if (c != '\n' && c != EOF)
      ungetc (c, fp);

   if (TRUNCATE && c !='\n')
      while ((c = getc (fp)) != '\n' && c != EOF)
         ;
   return  iLine + (c == '\n');
   }



void PrintHeader (FILE *fpOut)
   {
   int   i;

   PrintControlString(fpOut, HeaderCodes);
   for (i = 0; i < TOPLINES; i++)
      fprintf (fpOut, "\n");
   fprintf (fpOut, "\r %s",szMessage);
   for ( i = strlen (szMessage) + strlen (szFileName); i < (HEADERWIDTH - 3); i++)
      fprintf (fpOut, " ");
   fprintf (fpOut, "%s\n",szFileName);
   for (i=0; i < (HEADERWIDTH - 1); i++)
      fprintf (fpOut, "=");
   fprintf (fpOut, "\n");
   }



int PrintBody (FILE *fpOut, FILE *fp, int iLine, int iBodyLength, BOOL bSkip)
   {
   char  szCurrentLine [STRLEN];
   int   i, iMaxWidth, j;

   if (!bSkip)
      PrintControlString(fpOut, BodyCodes);
   for (i = 0; i < iBodyLength; i++)
      if (!bSkip && feof (fp))
         fprintf (fpOut,"\n");
      else
         {
         iMaxWidth = BODYWIDTH - LEFTMARGIN - RIGHTMARGIN;
         iMaxWidth -= LINENUMBERS ? LineNumberSize : 0;
         /* chop lines if chop mode on and in range */
         while (((j = (iLine - 1) % ChopTotal) < ChopMark1 ||
                  j >= ChopMark2) && SkipLine (fp)  != EOF)
            ++iLine;
         if (!bSkip && LINENUMBERS)
            fprintf (fpOut, "<%4.4d>", iLine % 10000);
         iLine = GetNextLine (fp, szCurrentLine, iMaxWidth, iLine);
         for (j = 0; j < LEFTMARGIN; j++)
            if (!bSkip)
               fprintf (fpOut, " ");
         if (!bSkip)
            fprintf (fpOut, "%s\n", szCurrentLine);
         }
   return iLine;
   }


//
//int SkipBody (FILE *fp, int iLine, int iBodyLength)
//   {
//   char  szCurrentLine [STRLEN];
//   int   i, iMaxWidth, j;
//
//   for (i = 0; i < iBodyLength; i++)
//      if (!feof (fp))
//         {
//         iMaxWidth = BODYWIDTH - LEFTMARGIN - RIGHTMARGIN;
//         iMaxWidth -= LINENUMBERS ? LineNumberSize : 0;
//         /* chop lines if chop mode on and in range */
//         while (((j = (iLine - 1) % ChopTotal) < ChopMark1 ||
//                  j >= ChopMark2) && SkipLine (fp)  != EOF)
//            ++iLine;
//         iLine = GetNextLine (fp, szCurrentLine, iMaxWidth, iLine);
//         }
//   return iLine;
//   }
//


void PrintFooter (FILE *fpOut, struct stat psFileBuff, int iPage)
   {
   int   i;
   PSZ   pszTimeStr;

   PrintControlString(fpOut, FooterCodes);
   for (i=0; i < (HEADERWIDTH - 1); i++)
      fprintf (fpOut, "=");
   fprintf (fpOut, "\n FileSize: %ld", psFileBuff.st_size);
   pszTimeStr = ctime (&psFileBuff.st_atime);
   pszTimeStr [24] = '\0';
   for (i = 52; i <= FOOTERWIDTH; i+=2)
      fprintf (fpOut, " ");
   fprintf (fpOut, "%s", pszTimeStr);
   for (i = 52; i <= FOOTERWIDTH; i+=2)
      fprintf (fpOut, " ");
   fprintf (fpOut, "Page: %d", iPage);
   }



/*    iSides truth table
 * = 0  print all pages.
 * = 1  print odd pages.
 * = 2  print even pages.
 */
void PrintFile (FILE *fpOut, FILE *fp)
   {
   struct stat  psFileBuff;
   int   i, iLinesPP;
   int   iLine = 1, iPage = 0;
   BOOL  bSkip;

   fstat (fileno (fp), &psFileBuff);
   psFileBuff.st_size = filelength (fileno (fp));
   printf ("Printing  %s", szFileName);
   for ( i = strlen (szFileName); i < 40; i++)
      printf (" ");
   printf ("Size = %6ld   Page =   0",  psFileBuff.st_size);
   iLinesPP = PAGELENGTH - TOPLINES;
   iLinesPP -= (FOOTER? FooterSize: 0) + (HEADER? HeaderSize: 0);

   for (iPage =1; !feof (fp); iPage++)
      {
      if (ENDPAGE < iPage)
         break;

      printf ("\b\b\b%3d",iPage);

      bSkip = (STARTPAGE > iPage || !EVEN && !(iPage % 2) || !ODD && (iPage %2));

      if (!bSkip && HEADER)
         PrintHeader (fpOut);
//
//      if (!bSkip)
//         {
//         if (HEADER)
//            PrintHeader (fpOut);
//         else
//            fprintf (fpOut, "%c", PAGEFF ? '\r' : '\n');
//         }
//
//

      iLine = PrintBody (fpOut, fp, iLine, iLinesPP, bSkip);
      if (!bSkip && FOOTER)
         PrintFooter (fpOut, psFileBuff, iPage);
      fprintf (fpOut, "%s", PAGEFF ? "\f\r" : "\n");
      }
   printf ("\015          \n");
   }




int cdecl main (int argc, PSZ argv[])
   {
   FILE  *fpIn;
   FILE  *fpOut;
   int   iCount;
   char  *psz;

   if (argc == 1)
      Usage ();

   BuildArgBlk ("*^BodyWidth% "
                "*^Chop% "
                "*^EndPage% "
                "*^Even% "
                "*^Footer%"
                "*^FooterWidth% "
                "*^Header%"
                "*^HeaderWidth% "
                "*^IgnoreFF%"
                "*^LeftMargin% "
                "*^LineNumbers% "
                "*^Odd% "
                "*^OutFile% "
                "*^PageFF% "
                "*^PageLength% "
                "*^RightMargin% "
                "*^StartPage% "
                "*^TopLines% "
                "*^Truncate% "
                "*^Extension% ");


   if (FillArgBlk (argv))
      {
      printf ("Error: %s\n", GetArgErr());
      exit (1);
      }

   for (iCount = 0; psz = GetArg (NULL, iCount); iCount++)
      if (*psz == '@' || *psz == '#' ) break;

   LoadConfigFile ((psz ? psz : argv[0]), !!(*psz == '@'));

   ReadCmdLine ();

   if (stricmp (szOutFileName, "PRN") == 0)
      {
      for (iCount = 0; iCount < 500; iCount++)
         if ((fpOut = fopen (szOutFileName, "w")) != NULL)
            break;
      }
   else
      fpOut = fopen (szOutFileName, "w");
   if (fpOut == NULL)
         {
         printf ("ERROR: Unable to open %s for output\n", szOutFileName);
         exit (1);
         }
   printf ("Output being sent to %s\n", szOutFileName);
   PrintControlString(fpOut, InitCodes);
   printf ("\n          FileName                                Size            Page\n");
   printf ("===============================================================================\n");

   for (iCount = 0; psz = GetArg (NULL, iCount); iCount++)
      {
      if (*psz == '@' || *psz == '#') continue;
      if ((fpIn = fopen (psz, "r")) == NULL)
         printf ("WARNING: Unable to open %s for input\n", psz);
      else
         {
         strcpy (szFileName, psz);
         PrintFile (fpOut, fpIn);
         fclose (fpIn);
         }
      }
   PrintControlString(fpOut, TermCodes);
   fclose (fpOut);
   return 0;
   }

