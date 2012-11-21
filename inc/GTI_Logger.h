#ifndef __GTI_Logger_H__
#define __GTI_Logger_H__


#include <iostream>

#include "ace/Thread_Mutex.h"
#include "ace/Synch.h"
#include "ace/Synch_Traits.h"
#include "ace/INET_Addr.h"
#include "ace/ACE.h"
#include "ace/Task.h"

#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/current_function.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/categories.hpp>

#include "GTI_Constants.h"
#include "GTI_GlobalConfigParams.h"

#define GTI_CONSOLE_LOG GTI_Logger::instance(GTI_Logger::LG_CONSOLE)

#define GTI_FILE_LOG GTI_Logger::instance(GTI_Logger::LG_FILE)

#define GTI_LOG_DEBUG_MSG(LOGGER,...)\
  if (LOGGER->isFor(GTI_Logger::LL_DEBUG))\
  {\
  GTI_LogItem __item__ (GTI_Logger::LL_DEBUG);\
  LOGGER->logMessage(__item__<<__VA_ARGS__);\
  }

#define GTI_LOG_INFO_MSG(LOGGER,...)\
  if (LOGGER->isFor(GTI_Logger::LL_INFO))\
  {\
  GTI_LogItem __item__ = (GTI_Logger::LL_INFO);\
  LOGGER->logMessage(__item__<<__VA_ARGS__);\
  }

#define GTI_LOG_INFO_MSG_FORCED(LOGGER,...)\
  {\
  GTI_LogItem __item__ = (GTI_Logger::LL_INFO);\
  LOGGER->logMessage( __item__<<__VA_ARGS__ );\
  }

#define GTI_LOG_ERROR_MSG(LOGGER,...)\
  if (LOGGER->isFor(GTI_Logger::LL_ERROR))\
  {\
  GTI_LogItem __item__ = (GTI_Logger::LL_ERROR);\
  LOGGER->logMessage(__item__<<ACE_TEXT(BOOST_CURRENT_FUNCTION)<<":["<<__LINE__<<"] "<<__VA_ARGS__);\
  }

#define GTI_LOG_FATAL_MSG(LOGGER,...)\
  if (LOGGER->isFor(GTI_Logger::LL_FATAL))\
  {\
  GTI_LogItem __item__ = (GTI_Logger::LL_FATAL);\
  LOGGER->logMessage(__item__<<ACE_TEXT(BOOST_CURRENT_FUNCTION)<<":["<<__LINE__<<"] "<<__VA_ARGS__);\
  }

class GTI_LogItem;

class GTI_LoggerBase_Sink;
class GTI_LoggerConsole_Sink;
class GTI_LoggerFile_Sink;

class GTI_EXPORT GTI_Logger
{
public :
 
  enum Type
  {
    LG_CONSOLE = 0,
    LG_FILE,
    LG_END
  };

  enum LogLevel
  {
    LL_DEBUG = 0, 
    LL_INFO,
    LL_ERROR,
    LL_FATAL,
    LL_END
  };

  static const GTI_Logger::LogLevel ms_defaultLevel;
  //static for log levels enums strings
  static const ACE_TCHAR ms_logLevelNames[LL_END][6];

  //returns instance of logger by type
  static boost::shared_ptr<GTI_Logger> instance(GTI_Logger::Type i_type);

  // D`ctr
  ~GTI_Logger();

  bool isFor( GTI_Logger::LogLevel i_level ) const 
  {
    return ( i_level >= m_loggerLevel );
  }

  void logMessage( const GTI_LogItem& i_logItem );

private :

  //C`ctr
  GTI_Logger( LogLevel i_level = ms_defaultLevel )
  {
    m_loggerLevel = i_level;
  }

  template <typename SINK>
  void init( void *i_extraData = 0 );

private:
  //The logger type implementation
  typedef  boost::shared_ptr<GTI_LoggerBase_Sink> LoggerSink;
  LoggerSink m_sinkSP;
  
  //The maximum entities in logger
  ACE_INT16 m_highWaterMark;

  LogLevel m_loggerLevel;

  typedef boost::array<boost::shared_ptr<GTI_Logger>,GTI_Logger::LG_END> LoggersContainer;
  static  LoggersContainer ms_loggersArraySP;

  static ACE_Thread_Mutex m_lock[LG_END];
  
};

///////////////////////////////////////////////////////////////////
class GTI_EXPORT GTI_LogItem  : public ACE_Message_Block
{
public:
  GTI_LogItem( GTI_Logger::LogLevel i_severity ) : m_severity(i_severity),m_threadNum(ACE_Thread::self())
  {
    m_curTime = ACE_OS::time();
  }
  
  GTI_LogItem( const GTI_LogItem& i_other )
  { 
    m_buffer = i_other.m_buffer;
    m_severity = i_other.m_severity;
    m_threadNum = i_other.m_threadNum;
    m_curTime = i_other.m_curTime;
  }

  ~GTI_LogItem()
  {}

  GTI_LogItem& operator<<(bool i_in);
  GTI_LogItem& operator<<(ACE_INT32 i_in);
  GTI_LogItem& operator<<(ACE_INT64 i_in);
  GTI_LogItem& operator<<(ACE_INT16 i_in);
  GTI_LogItem& operator<<(ACE_UINT32 i_in);
  GTI_LogItem& operator<<(ACE_UINT64 i_in);
  GTI_LogItem& operator<<(ACE_UINT16 i_in);
  GTI_LogItem& operator<<(double i_in);
  GTI_LogItem& operator<<(const ACE_TCHAR* i_in);
  GTI_LogItem& operator<<(const GTI_LogItem& i_in);
  GTI_LogItem& operator<<(const void * i_in);
  GTI_LogItem& operator<<(const ACE_INET_Addr& i_in);
  
  const ACE_TCHAR* msgBuff() const
  {
    return m_buffer.c_str();
  }

  GTI_Logger::LogLevel severity () const
  {
    return m_severity;
  }

  const ACE_TCHAR* severityName() const
  {
    return GTI_Logger::ms_logLevelNames[m_severity];
  }

  const ACE_INT64& threadNum() const
  {
    return m_threadNum;
  }

  const time_t& time() const
  {
    return m_curTime;
  }

  void recycle()
  {
     delete this;
  }

private:

  time_t m_curTime;
  GTI_Logger::LogLevel m_severity;
  
  ACE_INT64 m_threadNum;
  
  std::string m_buffer;
  
  mutable ACE_Atomic_Op<ACE_SYNCH_MUTEX,ACE_INT64> m_referenceCounter;
};

//////////////////////////////////////////////////////////////////////////////
//Base Sink
class GTI_LoggerBase_Sink : public ACE_Task<ACE_MT_SYNCH> , boost::noncopyable
{
  friend class GTI_Logger;

public :
  virtual ~GTI_LoggerBase_Sink(){}

protected:
  GTI_LoggerBase_Sink():m_close(false){}

  int svc(void)
  {
    ACE_Message_Block *pLogItem;
    
    while(true)
    {            
      this->msg_queue()->dequeue_head( pLogItem );
      
	  if ( m_close )
		  return 1;
	  
	  this->write(*(static_cast<GTI_LogItem* >(pLogItem)) );
      
      (static_cast<GTI_LogItem* >(pLogItem))->recycle();
    }

    return 1;
  }
  
  virtual void write( const GTI_LogItem& ) = 0;
  
  virtual const ACE_TCHAR* formatMessage(const GTI_LogItem&);

  virtual bool init()
  {
    return true;
  }
  
  virtual int close()
  {
	  m_close = true;
	  this->msg_queue()->close();
	  return 0;
  }

protected:
  //synchronization of messages inside one Logger
  mutable ACE_RW_Thread_Mutex m_printMessageMutex;
  bool m_close;
  ACE_TCHAR m_messageBuffer[1024];
};

////////////////////////////////////////////////////////////////////
//Sink for writing to screen
class GTI_EXPORT GTI_LoggerConsole_Sink : public GTI_LoggerBase_Sink
{
  friend class GTI_Logger;

private :
  GTI_LoggerConsole_Sink( void*  i_pData = 0 );
  
  void write( const GTI_LogItem& i_logItem )
  {
   // ACE_WRITE_GUARD(ACE_RW_Thread_Mutex, write_guard, m_printMessageMutex);

    ACE_OS::printf("%s\n",formatMessage(i_logItem)) ;

    ACE_OS::fflush(stdout);
  }
};

////////////////////////////////////////////////////////////////////
//Sink for writing to file
class GTI_EXPORT GTI_LoggerFile_Sink : public GTI_LoggerBase_Sink
{  
  friend class GTI_Logger;
public :

  ~GTI_LoggerFile_Sink()
  {
    ACE_OS::fflush(m_pFileDesc);
    ACE_OS::fclose(m_pFileDesc);
    m_pFileDesc = 0;
  }

private:
  GTI_LoggerFile_Sink( void*  i_pData = 0 );
 
  void write(const GTI_LogItem& i_logItem);
  

private:
  FILE      *m_pFileDesc;
  GTI_String m_fileName;
  bool       m_isTimeStampEnabled;
};


template <typename SINK>
void GTI_Logger::init( void *i_extraData )
{ 
  //set logger and get the configuration params
  GTI_GlobalConfigParams::instance()->getParameterValue( 
    GTI_Properties::LOGGER_SECTION_NAME , 
    GTI_Properties::LOG_MSG_QUEUE_HIGH_WATER_MARK_PARAM_NAME , 
    m_highWaterMark ,
    (ACE_INT16)30 );

  m_sinkSP =  LoggerSink( new SINK( i_extraData ) );

  m_sinkSP->init();

  // run logger
  m_sinkSP->activate();
}



// #include <iosfwd>                          // streamsize
// #include <boost/iostreams/categories.hpp>  // sink_tag
// 
// namespace io = boost::iostreams;
// 
// class my_sink {
// public:
//   typedef char      char_type;
//   typedef sink_tag  category;
// 
//   std::streamsize write(const char* s, std::streamsize n)
//   {
//     // Write up to n characters to the underlying 
//     // data sink into the buffer s, returning the 
//     // number of characters written
//   }
// 
//   /* Other members */
// };
// 
// #include <algorithm>                       // copy, min
// #include <iosfwd>                          // streamsize
// #include <boost/iostreams/categories.hpp>  // sink_tag
// 
// namespace boost { namespace iostreams { namespace example {
// 
//   template<typename Container>
//   class container_sink {
//   public:
//     typedef typename Container::value_type  char_type;
//     typedef sink_tag                        category;
//     container_sink(Container& container) : container_(container) { }
//     std::streamsize write(const char_type* s, std::streamsize n)
//     {
//       container_.insert(container_.end(), s, s + n);
//       return n;
//     }
//     Container& container() { return container_; }
//   private:
//     Container& container_;
//   };
// 
// } } } // End namespace boost::iostreams:example
// 
// #include <cassert>
// #include <string>
// #include <boost/iostreams/stream.hpp>
// #include <libs/iostreams/example/container_device.hpp> // container_sink
// 
// namespace io = boost::iostreams;
// namespace ex = boost::iostreams::example;
// 
// int main()
// {
//   using namespace std;
//   typedef ex::container_sink<string> string_sink;
// 
//   string                   result;
//   io::stream<string_sink>  out(result);
//   out << "Hello World!";
//   out.flush();
//   assert(result == "Hello World!");
// }


#endif // __GTI_Logger_H__
