#ifndef LC_LOC_H
#define LC_LOC_H
#include "catalog.h"
typedef unsigned char Lc8;
typedef unsigned short Lc16;
#if defined(_WIN32) && defined(LC_HOST_DLL)
#define LC_API __declspec(dllexport)
#else
#define LC_API
#endif
#define LC_FORMAT_CAP 384
#define LC_WIDTH 34
#define LC_LINES 5
#define LC_MAX_PAGES 8
#define LC_NAME_MAX 8
#define LC_NEWLINE 252
/* Fixed storage; dimensions in the last three words describe the used region. */
typedef struct {Lc8 cells[LC_MAX_PAGES*LC_LINES*LC_WIDTH];Lc16 pages,width,lines;} LcLayout;
LC_API Lc16 lc_language(Lc16 language);
LC_API Lc16 lc_variant(Lc16 quantity);
LC_API Lc16 lc_format(Lc16 language,Lc16 message,Lc8 const *name,Lc16 name_length,Lc16 quantity,Lc8 *out,Lc16 capacity);
LC_API Lc16 lc_wrap(Lc8 const *text,Lc16 length,Lc16 width,Lc16 lines,LcLayout *out);
#endif
