#include "CUI.h"

#include "GTI_Logger.h"
#include "DeviceListener.h"
#include "ConnectionInfo.h"
#include "ReferenceDataManager.h"

#include "boost/foreach.hpp"
#include <deque>

template <char D> 
short getDigit( const ACE_UINT64& i_val )
{
	ACE_UINT64 val = i_val;
	
	std::deque<char> digits;	
	do 
	{
		digits.push_front( val % 10);
		val /= 10;
	} while ( val > 0 );
    
	/*for( int i=0 ;i < digits.size(); i++ )
	{
		printf("%d\n" , digits[i]);
	}*/ 
	
	return ( D > digits.size() ) ? 0 : digits[D - 1]; 
} 

GTI_String convertUAM( const ACE_UINT64 &i_value )
{
	ACE_UINT64 val = i_value;
	std::ostringstream t;
	
	if ( i_value > (1 << 30) )
	{
		val = val >> 30 ;
		t << val;
		t << '.';
		t << getDigit<1>( i_value - val * ( 1 << 30 ) );
		t << getDigit<2>( i_value - val * ( 1 << 30 ) );
		t << " GB";
	}
	else if ( i_value > (1 << 20) )
	{
		val = val >> 20 ;
		t << val;
		t << '.';
		t << getDigit<1>( i_value - val * ( 1 << 20 ));
		t << getDigit<2>( i_value - val * ( 1 << 20 ));
		t << " MB";		
	}
	else if ( i_value > (1 << 10)  )
	{
		val = val >> 10 ;
		t << val;
		t << '.'	;																																																																																																												;
		t << getDigit<1>( i_value - val * ( 1 << 10 ))  ;
		t << getDigit<2>( i_value - val * ( 1 << 10 ))  ;
		t << " kB";
	}
	else
	{
		t << val;
		t << " B";
	}
	
	/*
	if ( i_value < (1 << 10))
	{
		t << val;
		t << " B";
	}
	else if ( i_value > (1 << 10) && i_value < (1 << 20))
	{
		val = val >> 10 ;
		t << val;
		t << '.'	;																																																																																																												;
		t << getDigit<1>( i_value - val * ( 1 << 10 ))  ;
		t << getDigit<2>( i_value - val * ( 1 << 10 ))  ;
		t << " kB";
	}
	else if ( i_value > (1 << 20) && i_value < (1 << 30) )
	{
		val = val >> 20 ;
		t << val;
		t << '.';
		t << getDigit<1>( i_value - val * ( 1 << 20 ));
		t << getDigit<2>( i_value - val * ( 1 << 20 ));
		t << " MB";																				
	}
	else */
	
	return GTI_String(t.str());
}

CUI::~CUI()
{
}


void CUI::subscribe(DeviceListener * i_pDL)
{
	m_devices.push_back( i_pDL );
}

bool CUI::init_ui()
{
	m_pScreen = initscr();
	//raw();
	noecho();
	cbreak();
	nodelay(m_pScreen,TRUE);
	refresh_ui();
	refresh();
	return true;
}

bool CUI::exit_ui()
{
	clear();
	endwin();
	return true;
}




bool CUI::refresh_ui()
{
	ACE_INT32 row;
	ACE_INT32 col;
	
	ACE_UINT64 utotal;
	ACE_UINT64 dtotal;
	
	getmaxyx(stdscr,row,col);
	
	if ( col < MAX_COL )
	{
		mvprintw(0,0,"The terminal is too small");
		return true;
	}
	
	std::vector<ConnInfo> connDataContainer;
	
	BOOST_FOREACH( DeviceListener* pDL, m_devices )
	{
		pDL->pollData( connDataContainer );
		
		/* StatData :
		* total upload
		* total download
		* total delta upload
		* tota delta download
		*/
		if ( m_totalDevUsage.find(pDL->getDeviceName()) == m_totalDevUsage.end())
			m_totalDevUsage[pDL->getDeviceName()] = boost::make_tuple(0,0,0,0);//std::make_pair(0,0);
		
		m_totalDevUsage[pDL->getDeviceName()].get<2>() = 0;
		m_totalDevUsage[pDL->getDeviceName()].get<3>() = 0;	
	}
	
	int row_i = 0;
	
	time_t currTime = ACE_OS::time(0);
    struct tm local_tm;

    ACE_OS::localtime_r(&currTime,&local_tm);
	
	int max_size = row - 10;
	
	//double	total_netUsagePer = 0;
	
	BOOST_FOREACH( const ConnInfo& conn, connDataContainer )
	{
		if ( conn.m_delta_uload > 0 || conn.m_delta_dload > 0)
		{
		  GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Data " << GTI_String(conn.getLocalAddr().get_host_addr()).c_str() << ":" << conn.getLocalAddr().get_port_number() << " " << conn.m_delta_uload << " " << conn.m_delta_dload);
		
		  m_totalDevUsage[ ReferenceDataManager::instance()->getDevName( conn ) ].get<0>() += conn.m_delta_uload;
		  m_totalDevUsage[ ReferenceDataManager::instance()->getDevName( conn ) ].get<1>() += conn.m_delta_dload;
		  
		  //previusly those fileds have been cleaned to enable rate calculation
		  m_totalDevUsage[ ReferenceDataManager::instance()->getDevName( conn ) ].get<2>() += conn.m_delta_uload;
		  m_totalDevUsage[ ReferenceDataManager::instance()->getDevName( conn ) ].get<3>() += conn.m_delta_dload;
		}
	}
	
	clear();
	mvprintw( row_i++ ,0 , "Last refresh: %02u:%02u:%02u , connections: %d total, %d displayed" , local_tm.tm_hour, local_tm.tm_min , local_tm.tm_sec , connDataContainer.size() , ( max_size < connDataContainer.size() ) ?  max_size : connDataContainer.size() );

	/* Show accumulted packets size captured per device 
	 * and its persentage usage to all transfered data in this device*/
	BOOST_FOREACH( CUI::DevUsageMap::value_type val , m_totalDevUsage )
	{
		mvprintw( row_i++ ,0 , "Device %s : accumulated upload %s (%s/s), download %s (%s/s), total %s , %.2f %% " ,
		val.first.c_str(),  
		convertUAM(val.second.get<0>()).c_str() , //upload 
		convertUAM(val.second.get<2>()/DELAY).c_str() ,//upload rate
		convertUAM(val.second.get<1>()).c_str(),  //download
		convertUAM(val.second.get<3>()/DELAY).c_str(),  //download rate
		convertUAM(val.second.get<0>() + val.second.get<1>()).c_str() , //upload + download
		(float)((float) (val.second.get<0>() +  val.second.get<1>() )/(float) (ReferenceDataManager::instance()->getDevUsage( val.first , 2 ) )) * 100 );
	}
	
	
//	mvprintw( row_i++ ,0 , "Total processed : upload %s , download %s" , convertUAM(m_totalUp).c_str() ,convertUAM(m_totalDown).c_str() );
	
	
	row_i++;
	
	attron( A_BOLD | A_REVERSE) ;
	mvprintw(row_i++, 0, " %-*s %-*s %-*s %-*s %-*s %-*s %-*s %*s %-*s ",
	DEV_F_SIZE,"DEV",
	PID_F_SIZE,"PID",
	IP_F_SIZE,"       Local IP",
	IP_F_SIZE,"      Remote IP",
	NET_USAGE_F_SIZE,"      Up/s",
	NET_USAGE_F_SIZE,"    Down/s",
	LAST_UPDATE_TIME_F_SIZE,"  Time+",
	NET_USAGE_PERCENTAGE_F_SIZE,"%  ",
	COMMAND_F_SIZE,"COMMAND");
	attroff(A_BOLD |  A_REVERSE);   
	
	
	double   netUsagePer = 0;
	//double   totalNetUsagePer = 0;
	GTI_String currDevName;
	BOOST_FOREACH( const ConnInfo& conn, connDataContainer )
	{
		currDevName = ReferenceDataManager::instance()->getDevName(conn);
		netUsagePer = (double)(((conn.m_uload + conn.m_dload) / (double) (ReferenceDataManager::instance()->getDevUsage( currDevName , 2 )))  * 100 );
		
		//if ( 0 != ( netUsagePer = ReferenceDataManager::instance()->getDevUsage( conn.getLocalAddr().get_ip_address(), 2 )) )
		//{
		//	netUsagePer =  (double)(((conn.m_uload + conn.m_dload) / (double) (ReferenceDataManager::instance()->getDevUsage( conn.getLocalAddr().get_ip_address(), 2 )))  * 100 );
		//} 
		
		struct tm update_tm;
		
		ACE_OS::localtime_r(&(conn.m_lastUpdate),&update_tm);
		 
		mvprintw(row_i++, 0 ," %-*s %-*u %*s:%-*d %*s:%-*d %*s %*s %02d:%02d:%02d %*.2f %-*s",
			DEV_F_SIZE , currDevName.c_str(),
			PID_F_SIZE,conn.getOwnerID(),
			15, GTI_String(conn.getLocalAddr().get_host_addr()).c_str() , 5, conn.getLocalAddr().get_port_number(),
			15, GTI_String(conn.getRemoteAddr().get_host_addr()).c_str() , 5, conn.getRemoteAddr().get_port_number(),
			NET_USAGE_F_SIZE, convertUAM((ACE_UINT64)(conn.m_delta_uload/DELAY)).c_str(), 
			NET_USAGE_F_SIZE, convertUAM((ACE_UINT64)(conn.m_delta_dload/DELAY)).c_str() ,
			update_tm.tm_hour,update_tm.tm_min,update_tm.tm_sec,
			NET_USAGE_PERCENTAGE_F_SIZE,netUsagePer ,
			COMMAND_F_SIZE, ReferenceDataManager::getProccessName(conn.getOwnerID(),COMMAND_F_SIZE).c_str()
			);
		
		if ( 0 == --max_size )
			break;
	}
	/*
	attron( A_REVERSE) ;
	mvprintw( row_i++ ,0 , "%-*s%*.2f" , MAX_COL + NET_USAGE_PERCENTAGE_F_SIZE +  10, " " , NET_USAGE_PERCENTAGE_F_SIZE, totalNetUsagePer );
	attroff( A_REVERSE );  
	*/
	return true;
}



int CUI::handle_timeout (const ACE_Time_Value &i_tv, const void *)
{
	refresh_ui();
	refresh();
	
	BOOST_FOREACH( DeviceListener* pDL, m_devices )
	{
		pDL->refresh();
	}
	
	return 0;
}

int CUI::handle_signal (int, siginfo_t *, ucontext_t *)
{
	this->reactor()->end_reactor_event_loop();
	
	BOOST_FOREACH( DeviceListener* pDL, m_devices )
	{
		pDL->stop_sniff();
	}	
	
	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"handle_signal CUI exiting" );
  return 0;
}