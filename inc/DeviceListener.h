#ifndef __DeviceListener_H__
#define __DeviceListener_H__
#include "ace/Synch.h"
#include "ace/Task.h"
#include "ace/INET_Addr.h"
#include "ace/Thread.h"
#include "ace/Thread_Mutex.h"
#include "ace/Condition_T.h"
#include "ace/Barrier.h"
#include "boost/foreach.hpp"
#include "GTI_Logger.h"
#ifndef WIN32
#include "pcap.h"
#include "TcpStructs.h"
#endif
#include <vector>
#include "ConnectionInfo.h"
///////////////////////////////////////////////////////////////////////////////
class DeviceListener : public ACE_Task<ACE_MT_SYNCH>  , boost::noncopyable
{
	public :
		//DeviceListener( ACE_Barrier &i_bar ) : m_barrier( i_bar ) , m_device( "eth0" ) , count(0) /*,m_sniff_cond(m_cond_mutex)*/{}
		DeviceListener( ACE_Thread_Mutex &i_bar , const GTI_String &i_deviceName , ACE_UINT16 i_liveID , const ConnInfoContainer &i_addr ,bool i_singleProcess = true ) :
			m_sniff_cond(i_bar) ,m_device( i_deviceName) , m_lifeId(i_liveID ) ,   m_singleProcess(i_singleProcess) , m_addr(i_addr) , m_pPH(0) , count(0) , m_filter(0)  /*m_sniff_cond(m_cond_mutex),*/ 
		{
			buildFilter();
		}
		
		~DeviceListener()
		{
			if ( m_filter )
				delete[] m_filter;
				
			m_filter = 0;
		}
#ifndef WIN32
		int svc() 
		{
			m_threadID = ACE_Thread::self();
            
			cancel_state state;
			state.cancelstate = PTHREAD_CANCEL_ENABLE;
			state.canceltype = PTHREAD_CANCEL_ASYNCHRONOUS;
			
			ACE_Thread::setcancelstate(state, 0);
			
			//m_barrier.wait();
			if ( 0 == m_pPH )
				 start_sniff();
				 
			GTI_LOG_INFO_MSG(GTI_FILE_LOG,"Device  " << m_device.c_str() << " listener going down");
			
			return 0;
		}
		
		void handle_packet( const struct pcap_pkthdr *i_pHeader, const u_char *i_pPacket );
		int stop_sniff();
	
	
	private:
		int start_sniff();
#endif
	
	public :
		bool pollData( std::vector<ConnInfo> &o_container ) const;
		void refresh();
		const GTI_String& getDeviceName() const;
	
	private:
		bool updateConnInfo ( const ACE_INET_Addr &i_src , const ACE_INET_Addr &i_dest , ACE_UINT16 i_size );
		bool buildFilter();
	
	private:
		//ACE_Barrier &m_barrier;
		
	private:
	    ACE_Thread_Mutex &m_sniff_cond;
	  
		GTI_String m_device;
		
		ACE_INT32 m_lifeId;
		bool m_singleProcess;
		
		ACE_UINT64 m_threadID;
		
		ConnInfoContainer m_addr;
		
		pcap_t *m_pPH;
		
		int count;  /* packet counter */
		
		ACE_TCHAR  *m_filter;
		
		mutable ACE_Thread_Mutex m_refresh_lock;
		
	//	mutable ACE_Thread_Mutex m_cond_mutex;
		
      
};
#endif // __DeviceListener_H__
