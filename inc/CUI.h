#ifndef __cui_h__
#define __cui_h__

#include "ace/Event_Handler.h"
#include "ace/Task.h"
#include "ace/Reactor.h"
#include "ace/OS_NS_time.h"
#include "ace/Barrier.h"

#include "boost/tuple/tuple.hpp"

#include "ncurses.h"

#include <vector>
#include <map>

class DeviceListener;

class CUI : public ACE_Task_Base
{
private :
    static const ACE_INT16 DEV_F_SIZE = 4;
	static const ACE_INT16 PID_F_SIZE = 9 ;
	static const ACE_INT16 IP_F_SIZE = 21;
	static const ACE_INT16 NET_USAGE_F_SIZE = 10;
	static const ACE_INT16 NET_USAGE_PERCENTAGE_F_SIZE = 6;
	static const ACE_INT16 LAST_UPDATE_TIME_F_SIZE = 8;
	static const ACE_INT16 COMMAND_F_SIZE = 13;
	static const ACE_INT16 MAX_COL = DEV_F_SIZE + PID_F_SIZE + IP_F_SIZE + IP_F_SIZE + NET_USAGE_F_SIZE + NET_USAGE_F_SIZE + LAST_UPDATE_TIME_F_SIZE + NET_USAGE_PERCENTAGE_F_SIZE + COMMAND_F_SIZE;
	static const ACE_UINT16 DELAY = 3; 
public:
	CUI( ACE_Barrier &i_barrier ): m_barrier(i_barrier) /*, 
	m_totalUp(0),
	m_totalDown(0)*/{}
	
	~CUI();
	
virtual int handle_timeout (const ACE_Time_Value &i_tv, const void *);
virtual int handle_signal (int, siginfo_t * = 0, ucontext_t * = 0);
  
virtual int svc(void)
{	  
	init_ui();
    
	ACE_Time_Value initialDelay (DELAY);
	ACE_Time_Value interval ( DELAY );
	  
	ACE_Reactor reactor;
	this->reactor(&reactor);
	this->reactor()->schedule_timer(this , 0, initialDelay ,interval );
	this->reactor()->register_handler(SIGINT,this);
	
	//m_barrier.wait();
	
	this->reactor()->run_reactor_event_loop();
	
	exit_ui();
	
	return 0;
}


void subscribe( DeviceListener* );
/*CUI handlers*/

bool init_ui();
bool refresh_ui();
bool exit_ui();


private:
	WINDOW *m_pScreen;
	
	std::vector<DeviceListener*> m_devices;
	
private:
	ACE_Barrier &m_barrier;
	
private:
	typedef boost::tuple<ACE_UINT64,ACE_UINT64,ACE_UINT64,ACE_UINT64> StatData;//std::pair<ACE_UINT64,ACE_UINT64>
	typedef std::map< std::string , StatData >  DevUsageMap;
	DevUsageMap m_totalDevUsage;
	
	//ACE_UINT64 m_totalUp;
	//ACE_UINT64 m_totalDown ;
	 
};

#endif // __cui_h__
