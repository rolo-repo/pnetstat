#ifndef __ReferenceDataManager_H__
#define __ReferenceDataManager_H__

#include "ace/Basic_Types.h"
#include "ace/Singleton.h"
#include "ace/Thread.h"
#include "ace/Recursive_Thread_Mutex.h"

#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_dirent.h"
#include "ace/OS_NS_string.h"

#include "ace/Event_Handler.h"
#include "ace/Task.h"
#include "ace/Reactor.h"

#include <map>
#include <set>

#include "GTI_Logger.h"

#ifndef WIN32
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>

#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "ConnectionInfo.h"

struct pair_first_equal 
{
  template <typename P>
  bool operator() (P const &lhs, P const &rhs) const 
  {
    return lhs.first == rhs.first;
  }
};


class ReferenceDataRefresher : public ACE_Task_Base
{
public:
  /// Hook method that is called by the reactor when a timer expires.
  /// It prints the timer ID and the time it expired.
  virtual int handle_timeout (const ACE_Time_Value &i_tv,
                              const void *);

  /// Sets the timer id for this handler <tid_> to <tid>
  void set_timer_id (long i_tid)
  {
	 m_tid =  i_tid;
  }
  
  virtual int svc(void)
  {
	  
	  ACE_Time_Value initialDelay (7);
	  ACE_Time_Value interval ( 7 );
	  
	  ACE_Reactor reactor;
	  this->reactor(&reactor);
	  
	  this->reactor()->schedule_timer(this , 0, initialDelay ,interval );
	  this->reactor()->run_reactor_event_loop();
	  
	  return 0;
  }
  
  int stop()
  {
		return this->reactor()->end_reactor_event_loop();
  }

private:
  /// timer ID.
  long m_tid;
};


class ReferenceDataManager 
{
  friend class ACE_Singleton<ReferenceDataManager,ACE_Thread_Mutex>;
  friend class ReferenceDataRefresher;
public :

  typedef ACE_Singleton<ReferenceDataManager,ACE_Thread_Mutex> SingeltonImpl;

public :

  struct ConnData
  {
    ACE_UINT32 locaddr;
    ACE_UINT32 locport;
    ACE_UINT32 remaddr;
    ACE_UINT32 remport;
    ACE_UINT32 uid; 
    ACE_TCHAR buf[256];

    ACE_TCHAR* print( const  ino_t &i_node )
    {
      struct in_addr laddr;
      laddr.s_addr = ACE_HTONL(locaddr);

      ACE_TCHAR buf1[16];

      ACE_OS::memcpy(buf1, inet_ntoa ( laddr ),sizeof(buf1) );

      struct in_addr raddr;
      raddr.s_addr = ACE_HTONL(remaddr);

      ACE_TCHAR buf2[16];

      ACE_OS::memcpy(buf2, inet_ntoa ( raddr ),sizeof(buf2) );

      //printf("IP  local %lu %s remote %lu %s\n" , laddr.s_addr ,  , raddr.s_addr,inet_ntoa ( raddr ) );

      int n = ACE_OS::snprintf(buf, sizeof(buf) - 1 , "LocAddr:%u %s LocPort:%u RemAddr:%u %s RemPort:%u uid:%u inode:%u", locaddr , buf1 , locport, remaddr, buf2,  remport, uid , i_node);

      buf[n] = '\0';

      return buf;
    }
  };

  struct DevStat
  {
    ACE_UINT64 rx;
    ACE_UINT64 tx;
	
	DevStat operator-( const DevStat &other ) const 
	{
		DevStat t;
		
		t.rx = rx - other.rx ;
		t.tx = tx - other.tx ;
		
		return t;
	}
  };
  
  struct DeviceData
  {
	  GTI_String m_name;
	  ACE_UINT64 m_speed;
  };

public: 

  static ReferenceDataManager* instance( )
  {
    ReferenceDataManager * p_rdm = SingeltonImpl::instance();

		/*
	    if ( false == p_rdm->m_isInitialized )
	    {
	      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, p_rdm->m_mutex , 0);      
	
	      if ( false == p_rdm->m_isInitialized && false == p_rdm->init( ) )
	        return 0;
	    }
		*/
    return p_rdm;
  }

public:

  typedef std::map < ACE_UINT32, GTI_String >  IpToDevMap;
  typedef std::map < ino_t, ConnData >  InodToConnMap;
  typedef std::map < ACE_UINT32, std::set< ino_t > >  PidToInodMap;
  typedef std::map < GTI_String , DevStat>   DevToDevStatMap;

  typedef std::map < GTI_String , ConnInfoContainer >   DevToConnInfoMap;

  template <typename M>
  static bool full_map_compare ( const M &i_lhs, const M &i_rhs ) 
  {
    // No predicate needed because there is operator== for pairs already.
    return i_lhs.size() == i_rhs.size() && std::equal( i_lhs.begin(), i_lhs.end(),i_rhs.begin() );
  }

  template <typename M>
  static bool fast_map_compare ( const M &i_lhs, const M &i_rhs ) 
  {
    return i_lhs.size() == i_rhs.size() && std::equal(i_lhs.begin(), i_lhs.end(), i_rhs.begin(),  pair_first_equal()); // predicate instance
  }
  
  ~ReferenceDataManager()
  {
	  fini();
  }

private:
  ReferenceDataManager() : m_isInitialized(false) , m_pid (0) , m_localIP(0) {}

  
  bool reinit();
 
#ifndef WIN32 
private:
  bool readNetworkInterfaces();
  bool readProcNetTCP();
  bool readProcXFd();
  bool readProcPIDFd( const ACE_UINT32 &i_pid  , std::set<ino_t> &o_inods ) const;
  bool readProcPIDFd (const ACE_UINT32 &i_pid );
  bool readProcNetDev();
  bool readInode( ino_t& o_inode, const ACE_TCHAR* i_pPath ) const;
#endif


public:
	bool init( ACE_UINT32 i_pid = 0 );
  bool fini(); 
  void print();
  ACE_UINT64  getDevUsage ( const ACE_UINT32 &i_ip , ACE_INT16 i_usageType );
  ACE_UINT64  getDevUsage ( const GTI_String& i_devName , ACE_INT16 i_usageType  );
  GTI_String  getDevName( const ACE_UINT32 &i_ip ) const;
  GTI_String  getDevName( const ConnInfo &i_conn ) const;
  ACE_INT32   getProcessConnections ( const ACE_UINT32 &i_pid , DevToConnInfoMap &o_devices ) const;
  ACE_INT32   getProcessConnections ( const GTI_String & i_dev , const ACE_UINT32 &i_pid , ConnInfoContainer &o_connects ) const;
  ACE_INT32   getAllConnections 	( DevToConnInfoMap &o_devices ) const;
  ACE_INT32   getLiveID() const {  return  m_lifeId; }
  
  static GTI_String getProccessName(const ACE_UINT32 &i_pid ,const ACE_UINT16 & i_size = std::string::npos );
  
private :
  static const ACE_TCHAR SOCKET_LINK_PREFIX[];

private:		
  IpToDevMap          m_ip_to_dev;
  
  InodToConnMap       m_inod_to_conn;
  PidToInodMap  	  m_pid_to_inod;
  
  DevToDevStatMap     m_dev_dev_stat;
  DevToDevStatMap 	  m_initial_dev_dev_stat;

private:
  mutable 	ACE_Recursive_Thread_Mutex m_mutex;
  bool    	 m_isInitialized;
  ACE_INT32  m_lifeId;
  ACE_UINT32 m_pid;
  ACE_UINT32 m_localIP;
  
  ReferenceDataRefresher m_refresher;

}; // ReferenceDataManager
#endif // __ReferenceDataManager_H__
