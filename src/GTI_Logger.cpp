#include "GTI_Logger.h"

#include "ace/OS_NS_stdio.h"
#include "ace/Thread.h"

#include "boost/lexical_cast.hpp"
#include "GTI_Constants.h"
#include "GTI_GlobalConfigParams.h"


//statics
GTI_Logger::LoggersContainer  GTI_Logger::ms_loggersArraySP;
ACE_Thread_Mutex              GTI_Logger::m_lock[GTI_Logger::LG_END];


const ACE_TCHAR GTI_Logger::ms_logLevelNames[GTI_Logger::LL_END][6]={{"DEBUG"},{"INFO"},{"ERROR"},{"FATAL"}};
const GTI_Logger::LogLevel GTI_Logger::ms_defaultLevel = GTI_Logger::LL_INFO;
//////////////////////////////////////////////////////////////////////////
boost::shared_ptr<GTI_Logger> GTI_Logger::instance(GTI_Logger::Type i_type)
{   
  boost::shared_ptr<GTI_Logger> resultSP;

  if ( GTI_Logger::LG_FILE == i_type  )
  {
    if (! GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_FILE] )
    {
      //double check
      ACE_Guard<ACE_Thread_Mutex> guard( m_lock[GTI_Logger::LG_FILE] );
     
      if (! GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_FILE] )
      {
        ACE_INT16 logLevel = 0;
        //read the logger level parameter
        GTI_GlobalConfigParams::instance()->getParameterValue( GTI_Properties::LOGGER_SECTION_NAME ,  GTI_Properties::LOG_LEVEL_PARAM_NAME , logLevel , (ACE_INT16) ms_defaultLevel );

        GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_FILE] = boost::shared_ptr<GTI_Logger>( new GTI_Logger( GTI_Logger::LogLevel(logLevel) ));
        GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_FILE]->init<GTI_LoggerFile_Sink>( );     
      }
    }
    resultSP = GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_FILE];
  }
  else
  {
    if (! GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_CONSOLE] )
    {
      //double check
      ACE_Guard<ACE_Thread_Mutex> guard( m_lock[GTI_Logger::LG_CONSOLE] );
     
      if (! GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_CONSOLE] )
      {
        ACE_INT16 logLevel = 0;

        //read the logger level parameter
        GTI_GlobalConfigParams::instance()->getParameterValue( GTI_Properties::LOGGER_SECTION_NAME ,  GTI_Properties::LOG_LEVEL_PARAM_NAME , logLevel , (ACE_INT16) ms_defaultLevel );

        GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_CONSOLE] = boost::shared_ptr<GTI_Logger>(new GTI_Logger( GTI_Logger::LogLevel (logLevel) ));
        GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_CONSOLE]->init<GTI_LoggerConsole_Sink>();  
      } 
    }
    resultSP = GTI_Logger::ms_loggersArraySP[GTI_Logger::LG_CONSOLE];
  }

  return resultSP;
}

//////////////////////////////////////////////////////////////////////////

GTI_Logger::~GTI_Logger()
{
  m_sinkSP->close();
}

void GTI_Logger::logMessage( const GTI_LogItem & i_logItem )
{
  if ( this->m_sinkSP->msg_queue()->message_count() < m_highWaterMark )
    this->m_sinkSP->msg_queue()->enqueue_tail( (ACE_Message_Block *)(new GTI_LogItem( i_logItem )));
  else
    std::cerr<<" Logger has been throttled , ignoring the message" << std::endl;
}

//////////////////////////////////////////////////////////////////////////

GTI_LogItem& GTI_LogItem::operator<<(bool i_in)
{

  this->m_buffer += (i_in ? "true" : "false") ;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_INT32 i_in )
{
  char buffer[11];

  ACE_OS::sprintf(buffer,ACE_INT32_FORMAT_SPECIFIER,i_in);
  this->m_buffer += buffer;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_INT64 i_in)
{
  char buffer[19];

  ACE_OS::sprintf(buffer, ACE_INT64_FORMAT_SPECIFIER ,i_in);
  this->m_buffer += buffer;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_UINT16 i_in )
{
  return *this << (ACE_UINT32) i_in;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_UINT32 i_in )
{
  char buffer[11];

  ACE_OS::sprintf(buffer, ACE_UINT32_FORMAT_SPECIFIER ,i_in);
  this->m_buffer += buffer;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_UINT64 i_in)
{
  char buffer[19];

  ACE_OS::sprintf(buffer,ACE_UINT64_FORMAT_SPECIFIER,i_in);
  this->m_buffer += buffer;
  return *this;
}


//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( ACE_INT16 i_in)
{
  return *this<<((ACE_INT32)i_in);
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( double i_in )
{
  char buffer[19];

  ACE_OS::sprintf(buffer,"%.2f",i_in);
  this->m_buffer += buffer;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( const void * i_in )
{ 
  if(i_in)
  {
    char buffer[19];
    ACE_OS::sprintf( buffer,"0x%X",i_in );
    this->m_buffer += buffer;
  }

  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<(const ACE_TCHAR* i_in)
{
  if(i_in)
  {
    this->m_buffer += i_in;
  }
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem& GTI_LogItem::operator<<( const GTI_LogItem& i_in )
{
  this->m_buffer += i_in.m_buffer;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
GTI_LogItem&  GTI_LogItem::operator<<(const ACE_INET_Addr& i_in)
{
  //ACE_TCHAR buffer[MAXHOSTNAMELEN];

  //i_in.addr_to_string( buffer , MAXHOSTNAMELEN );

  return *this << i_in.get_host_addr() << ":" << i_in.get_port_number();
}

//////////////////////////////////////////////////////////////////////////
const ACE_TCHAR* GTI_LoggerBase_Sink::formatMessage(const GTI_LogItem &i_logItem)
{
  tm curGMTtime; 
 
  ACE_OS::gmtime_r(&(i_logItem.time()),&curGMTtime);

  ACE_OS::snprintf(m_messageBuffer,sizeof(m_messageBuffer) - 1 ,"(%llu) [%02d/%02d/%04d %02d:%02d:%02d] [%s]: %s",i_logItem.threadNum(),
    curGMTtime.tm_mday,
    curGMTtime.tm_mon + 1,
    curGMTtime.tm_year + 1900,
    curGMTtime.tm_hour,
    curGMTtime.tm_min,
    curGMTtime.tm_sec,
    i_logItem.severityName(),
    i_logItem.msgBuff());

  m_messageBuffer[1023]= 0x0;

  return m_messageBuffer;
}


GTI_LoggerConsole_Sink::GTI_LoggerConsole_Sink( void*  i_pData )
{

}

//////////////////////////////////////////////////////////////////////////

GTI_LoggerFile_Sink::GTI_LoggerFile_Sink( void*  i_pData )
{
  m_pFileDesc = 0;

  GTI_String logFilePath;

  GTI_GlobalConfigParams::instance()->getParameterValue( 
    GTI_Properties::LOGGER_SECTION_NAME , GTI_Properties::FILE_PATH_PARAM_NAME , logFilePath , "./log");

  GTI_String logFileNamePrefix;

  GTI_GlobalConfigParams::instance()->getParameterValue( 
    GTI_Properties::LOGGER_SECTION_NAME , GTI_Properties::FILE_NAME_PREFIX_PARAM_NAME , logFileNamePrefix  , "GTIS_log");


  GTI_GlobalConfigParams::instance()->getParameterValue( 
    GTI_Properties::LOGGER_SECTION_NAME , GTI_Properties::LOG_FILE_NAME_TIMESTAMP_ENABLED_PARAM_NAME , m_isTimeStampEnabled , false );

  // const ACE_TCHAR *pFileName = ACE::basename(reinterpret_cast< const ACE_TCHAR* >(i_data),'/');
  // const ACE_TCHAR *pDirName  = ACE::dirname(reinterpret_cast< const ACE_TCHAR* >(i_data),'/');

  ACE_TCHAR filePathAndName[FILENAME_MAX];

  ACE_INT32 filePathAndNameSize;

  if ( m_isTimeStampEnabled )
  {
    time_t currTime = ACE_OS::time(0);
    struct tm local_tm;

    ACE_OS::localtime_r(&currTime,&local_tm);

    filePathAndNameSize = ACE_OS::sprintf( filePathAndName, "%s/%s_%02u-%02u-%04u_%02u-%02u-%02u.txt",
      logFilePath.c_str(),
      logFileNamePrefix.c_str(),
      local_tm.tm_mday,
      local_tm.tm_mon + 1,
      local_tm.tm_year + 1900,
      local_tm.tm_hour,
      local_tm.tm_min,
      local_tm.tm_sec);
  }
  else
  {
    filePathAndNameSize = ACE_OS::sprintf( filePathAndName, "%s/%s.txt",
      logFilePath.c_str(),
      logFileNamePrefix.c_str());
  }

  m_fileName.assign( filePathAndName , filePathAndNameSize );
} 

//////////////////////////////////////////////////////////////////////////

void GTI_LoggerFile_Sink::write( const GTI_LogItem& i_logItem )
{
  //  ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, m_printMessageMutex);

  if( !m_pFileDesc )
  {
    m_pFileDesc = ACE_OS::fopen( m_fileName.c_str(), "w" );
   // add lock on file so the file can not be deleted
    if (!m_pFileDesc)
    {
      GTI_LOG_FATAL_MSG(GTI_CONSOLE_LOG,"Failed to open file ["<< m_fileName.c_str() <<"] for writing , error "<<ACE_OS::strerror(errno));
      return;
    }
  }

  int writtenLen = ACE_OS::fprintf( m_pFileDesc, "%s\n" , formatMessage(i_logItem) );

  if ( writtenLen < 0 )
  {
    GTI_LOG_FATAL_MSG(GTI_CONSOLE_LOG,"Failed to write to file ["<< m_fileName.c_str() <<"] , error "<<ACE_OS::strerror(errno));
    ACE_OS::fclose( m_pFileDesc );
    m_pFileDesc = 0;
    return;
  }

  ACE_OS::fflush( m_pFileDesc );
}
