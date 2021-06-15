/* 
 * CSV.H -- Comma-separated value parser and generator.
 * (C) 1989, Info Tech, Inc.
 */


typedef char *PSTR;


PSTR GetCSVField (int iField, PSTR pCSVString, PSTR sResult);
/* -- Gets the iField-th comma separated field from the string
      pCSVString and puts it in sResult.  Returns a pointer to the
      result.

      Field numbers start with 1.

      If there is no iField-th field, creates an empty string.

      Strips leading and trailing ", and collapses "" into ".
*/



PSTR MakeCSVField (PSTR pString, PSTR pResult);
/* -- Changes the string pString, putting the changed one in pResult, 
      so it can be in a 
      CSV string.  Generally, this means replacing " with "", and
      putting " around it if there are internal " or ,.
*/

