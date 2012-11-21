#ifndef __GTI_Constants_H__
#define __GTI_Constants_H__

#include <boost/current_function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/variant.hpp>
#include <boost/noncopyable.hpp>

#include <ace/OS_NS_string.h>
#include <ace/OS_NS_unistd.h>
#include "ace/OS.h"
#include "ace/config-all.h"
#include "ace/Basic_Types.h"

#include "ace/Message_Block.h"

#include "GTI_Exports.h"

#include <string>

#ifdef WIN32
#pragma warning( disable : 4267 4355 18 )
#endif

#define GTI_ALIGN_TO_INT64(P) ACE_ptr_align_binary(P, sizeof(ACE_INT64))
#define GTI_ALIGN_TO_INT32(P) ACE_ptr_align_binary(P, sizeof(ACE_INT32))
#define GTI_ALIGN_TO_INT16(P) ACE_ptr_align_binary(P, sizeof(ACE_INT16))


#ifndef ACE_SWAP_LONG_LONG

# define ACE_SWAP_LONG_LONG(L) ((ACE_SWAP_LONG ((L) & 0xFFFFFFFF) << 32) \
  | ACE_SWAP_LONG(((L) >> 32) & 0xFFFFFFFF))


# if defined (ACE_LITTLE_ENDIAN)
#   define ACE_HTONLL(X) ACE_SWAP_LONG_LONG (X)
#   define ACE_NTOHLL(X) ACE_SWAP_LONG_LONG (X)
#   define ACE_IDL_NCTOHLL(X) (X)
#   define ACE_IDL_NSTOHLL(X) (X)
# else
#   define ACE_HTONLL(X) X
#   define ACE_NTOHLL(X) X
#   define ACE_IDL_NCTOHLL(X) (X << 24)
#   define ACE_IDL_NSTOHLL(X) ((X) << 16)
#endif //if defined (ACE_LITTLE_ENDIAN)

#endif //ACE_SWAP_LONG_LONG

typedef std::string GTI_String;
typedef std::pair< ACE_TCHAR* , size_t > GTI_BufferWrapper;


#ifdef WIN32
#define GTI_RESOURCE_BUSY 12
#else
#define GTI_RESOURCE_BUSY -1
#endif



class GTI_EXPORT GTI_Constants
{
public:
  static const int GTI_ERROR ;
  static const int GTI_SUCCESS;
  static const int GTI_NO_DATA_RCVD ;
  static const int GTI_BUSY;
  static const short GTI_HANDLER_KEY_SIZE;
  static const ACE_TCHAR * const GTI_SERVER_VERSION;
  
  static const ACE_TCHAR* printBuffer(const ACE_TCHAR* i_pBuf,ACE_UINT64 i_size, ACE_TCHAR* o_pBuf,ACE_UINT64 i_pBufSize, ACE_UINT64 & o_pBufSize);
//  string std::string utf8_encode(const std::wstring &wstr);
 // std::wstring GTI_Constants::utf8_decode(const std::string &str);
};

class GTI_EXPORT GTI_Properties
{
public :
  static const ACE_TCHAR * const GENERIC_SECTION_NAME;
  static const ACE_TCHAR * const LOGGER_SECTION_NAME;
  static const ACE_TCHAR * const NETWORK_SECTION_NAME;
  static const ACE_TCHAR * const SERVER_SECTION_NAME;
  static const ACE_TCHAR * const ADDRESS_PUBLISHER_SECTION_NAME;
  static const ACE_TCHAR * const PLUGINS_SECTION_NAME;

  static const ACE_TCHAR * const FILE_PATH_PARAM_NAME;
  static const ACE_TCHAR * const FILE_NAME_PREFIX_PARAM_NAME;
  static const ACE_TCHAR * const FILE_NAME_PARAM_NAME;

  static const ACE_TCHAR * const LOG_LEVEL_PARAM_NAME; 			
  static const ACE_TCHAR * const LOG_FILE_NAME_TIMESTAMP_ENABLED_PARAM_NAME;
  static const ACE_TCHAR * const LOG_MSG_QUEUE_HIGH_WATER_MARK_PARAM_NAME;
  
  static const ACE_TCHAR * const NUMBER_OF_THREADS_PARAM_NAME;
  static const ACE_TCHAR * const SERVER_NAME; 
  static const ACE_TCHAR * const BROADCAST_PORT_PARAM_NAME;
  static const ACE_TCHAR * const TIME_OUT_IN_SEC_PARAM_NAME;
  static const ACE_TCHAR * const MIN_LISTENED_PORT_PARAM_NAME;
  static const ACE_TCHAR * const MAX_LISTENED_PORT_PARAM_NAME;
};


#endif // __GTI_Constants_H__