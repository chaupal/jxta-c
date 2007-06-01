
The accompanying C source code file "ConvertUTF.c" and the associated header
file "ConvertUTF.h" provide for conversion between various transformation
formats of Unicode characters.  The following conversions are supported:

	UCS4 to UTF16
	UCS4 to UTF8
	UTF16 to UCS4
	UTF16 to UTF8
	UTF8 to UTF16
	UTF8 to UCS4


The files "CVTUTF7.C" and "CVTUTF7.H" are for archival and historical purposes
only.  They have not been updated to Unicode 3.0 and should be considered
obsolescent.  "CVTUTF7.C" contains two functions that can convert between
UCS2 (i.e., the BMP characters only) and UTF-7.  Surrogates are not supported,
the code has not been tested, and should be considered unsuitable for general
purpose use.

Please address any bug reports about these programs to info@unicode.org.

Last update: July 12, 2001

