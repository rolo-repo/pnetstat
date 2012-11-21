#ifndef __GTI_Exports_H__
#define __GTI_Exports_H__

#include "ace/config-all.h"

#  if defined (GTI_EXPORTS)
#    define GTI_EXPORT ACE_Proper_Export_Flag
#  else /* GTI_EXPORTS */
#    define GTI_EXPORT ACE_Proper_Import_Flag
#endif

#endif // __GTI_Exports_H__