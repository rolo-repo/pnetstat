#include "GTI_Constants.h"

const ACE_TCHAR * const GTI_Constants::GTI_SERVER_VERSION = "12.04.10";
const int               GTI_Constants::GTI_ERROR = -1;
const int               GTI_Constants::GTI_SUCCESS = 0;
const int               GTI_Constants::GTI_NO_DATA_RCVD = -2;
const short             GTI_Constants::GTI_HANDLER_KEY_SIZE = 5;
const int               GTI_Constants::GTI_BUSY = -3;


const ACE_TCHAR * const GTI_Properties::LOGGER_SECTION_NAME = "logger";
const ACE_TCHAR * const GTI_Properties::GENERIC_SECTION_NAME = "global";
const ACE_TCHAR * const GTI_Properties::NETWORK_SECTION_NAME = "network";
const ACE_TCHAR * const GTI_Properties::SERVER_SECTION_NAME = "server";
const ACE_TCHAR * const GTI_Properties::ADDRESS_PUBLISHER_SECTION_NAME = "address_publisher";
const ACE_TCHAR * const GTI_Properties::PLUGINS_SECTION_NAME = "plugins";


 			
const ACE_TCHAR * const GTI_Properties::FILE_PATH_PARAM_NAME            = "file_path";
const ACE_TCHAR * const GTI_Properties::FILE_NAME_PARAM_NAME            = "file_name";
const ACE_TCHAR * const GTI_Properties::FILE_NAME_PREFIX_PARAM_NAME     = "file_name_prefix";

const ACE_TCHAR * const GTI_Properties::LOG_LEVEL_PARAM_NAME            = "level";
const ACE_TCHAR * const GTI_Properties::LOG_FILE_NAME_TIMESTAMP_ENABLED_PARAM_NAME = "file_timestamp_enabled";
const ACE_TCHAR * const GTI_Properties::LOG_MSG_QUEUE_HIGH_WATER_MARK_PARAM_NAME = "high_water_mark";

const ACE_TCHAR * const GTI_Properties::NUMBER_OF_THREADS_PARAM_NAME  = "number_of_threads";
const ACE_TCHAR * const GTI_Properties::MIN_LISTENED_PORT_PARAM_NAME  = "min_listened_port";
const ACE_TCHAR * const GTI_Properties::MAX_LISTENED_PORT_PARAM_NAME  = "max_listened_port";
const ACE_TCHAR * const GTI_Properties::SERVER_NAME                   = "server_name";

const ACE_TCHAR * const GTI_Properties::BROADCAST_PORT_PARAM_NAME     = "broadcast_port";
const ACE_TCHAR * const GTI_Properties::TIME_OUT_IN_SEC_PARAM_NAME    = "timeout_in_sec";


const ACE_TCHAR* GTI_Constants::printBuffer(const ACE_TCHAR* i_pBuf, ACE_UINT64 i_size, ACE_TCHAR* io_pBuf, ACE_UINT64 i_pBufSize, ACE_UINT64 & o_pBufSize)
{
  o_pBufSize = 0;
  const ACE_TCHAR* pInputBuf = i_pBuf;
  const ACE_TCHAR* pOutBuffer = io_pBuf;

  const ACE_TCHAR format[][60] =   {{"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x %02x %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x %02x %02x %02x %02x\n"},
  {"offset " ACE_UINT64_FORMAT_SPECIFIER ": %02x %02x %02x %02x %02x %02x %02x %02x\n"}};


  for ( ACE_UINT64 offset = 0; offset < i_size ; offset += 8, pInputBuf += 8 )
  {

    ACE_INT16 i = 0;
    ( offset < i_size - 1 ? i++ : 0);
    ( offset < i_size - 2 ? i++ : 0);
    ( offset < i_size - 3 ? i++ : 0);
    ( offset < i_size - 4 ? i++ : 0);
    ( offset < i_size - 5 ? i++ : 0);
    ( offset < i_size - 6 ? i++ : 0);
    ( offset < i_size - 7 ? i++ : 0);

    io_pBuf += ACE_OS::snprintf(
      io_pBuf,
      i_pBufSize - (io_pBuf - pOutBuffer),
      format[i],
      offset,
      (unsigned char)pInputBuf[0],
      (unsigned char)(offset < i_size - 1 ? pInputBuf[1] : 0),
      (unsigned char)(offset < i_size - 2 ? pInputBuf[2] : 0),
      (unsigned char)(offset < i_size - 3 ? pInputBuf[3] : 0),
      (unsigned char)(offset < i_size - 4 ? pInputBuf[4] : 0),
      (unsigned char)(offset < i_size - 5 ? pInputBuf[5] : 0),
      (unsigned char)(offset < i_size - 6 ? pInputBuf[6] : 0),
      (unsigned char)(offset < i_size - 7 ? pInputBuf[7] : 0)
      );

    if( i_pBufSize < (io_pBuf - pOutBuffer) )
      break;

  }

  o_pBufSize = io_pBuf - pOutBuffer;

  return pOutBuffer;
}
/*
// Convert a wide Unicode string to an UTF8 
string std::string GTI_Constants::utf8_encode(const std::wstring &wstr) 
{    
int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL); 
std::string strTo( size_needed, 0 ); 
WideCharToMultiByte  
(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL); 
return strTo; 
} 

// Convert an UTF8 string to a wide Unicode String 

std::wstring GTI_Constants::utf8_decode(const std::string &str) 
{
int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0); 
std::wstring wstrTo( size_needed, 0 );  
MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);  
return wstrTo;
} 
*/
