
//#include "ace/INET_Addr.h"
#include "ace/Barrier.h"

//#include "ace/Recursive_Thread_Mutex.h"

#include <ace/ARGV.h>
#include <ace/Get_Opt.h>
#include "ace/Thread_Mutex.h"
#include "ace/Condition_T.h"

#include "boost/foreach.hpp"

#include "GTI_Logger.h"
#include "GTI_Constants.h"
#include "GTI_GlobalConfigParams.h"


#include "ReferenceDataManager.h"
#include "DeviceListener.h"
#include "ConnectionInfo.h"

#include "CUI.h"

#define USAGE( EXE ) \
  "\nUsage : " << \
  EXE << " [-f configuration file] " \
  

int ACE_TMAIN ( int argc, ACE_TCHAR ** argv ) 
{
  
 ACE_Get_Opt get_opt( argc, argv, ACE_TEXT("f:p:") );    

  get_opt.long_option (ACE_TEXT ("cfile"), 'f', ACE_Get_Opt::ARG_OPTIONAL);

  int c = 0;
 
 ACE_UINT32 pid = 0;
 
  while ( ( c = get_opt () ) != EOF )
  {
    switch (c)
    {
    case 'f':
      {
        GTI_GlobalConfigParams::spPropsFileName = get_opt.opt_arg();  
      }
      break;
	case 'p':
	  pid = ACE_OS::atoi(get_opt.opt_arg());
	  break;
    case '?': // An unrecognized option.
    default:
      std::cerr << "Fatal >>> Failed to parse input params " << USAGE( argv[0] ) << std::endl ; 
      return 1;
    }
  }
	

  bool isSingleProcess  = ( pid == 0 ) ? false : true ; 
  
  ReferenceDataManager *p_rfm = ReferenceDataManager::instance();
 
  if ( p_rfm )
  {
  	p_rfm->init( pid );
	p_rfm->print();
 
	// ... fill the map...

	ReferenceDataManager::DevToConnInfoMap deviceToConnection;

	ACE_INT16 liveID = 0;
 
	if ( isSingleProcess )
	{
		  if ( 0 > ( liveID = p_rfm->getProcessConnections( pid , deviceToConnection  ) ) )
		  {
			GTI_LOG_ERROR_MSG( GTI_FILE_LOG,"Failed to build list of devices to open" );
		  }
	}
	else
	{
		  if ( 0 >  ( liveID = p_rfm->getAllConnections( deviceToConnection )) )
		  {
			GTI_LOG_ERROR_MSG( GTI_FILE_LOG,"Failed to build list of devices to open" );
		  }
	}
 
	ACE_Barrier barrier(2);
  
	if ( deviceToConnection.size() > 0 && liveID > 0 )
	{
		DeviceListener **p_listeners = new  DeviceListener*[ deviceToConnection.size() ];
		
		ACE_INT16 i = 0;
		
		CUI cui( barrier );
		
		ACE_Thread_Mutex 					   m_cond_mutex;
       
		BOOST_FOREACH( ReferenceDataManager::DevToConnInfoMap::value_type dev , deviceToConnection  )
		{
			p_listeners[i] = new DeviceListener( m_cond_mutex , dev.first , liveID , dev.second , isSingleProcess );

		    p_listeners[i]->activate();
			
			cui.subscribe( p_listeners[i] );
			
			i++;
			//it looks like a condition  is more convinient here ,but after 
			//i need  the threads to make auto refresh and 
			//they should be synchronized in the actions so condition is not the tool here
			ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_cond_mutex , -1);
		}
		
		cui.activate();
		
		cui.wait();	
		
		for ( i = 0 ; i < deviceToConnection.size() ; i++ )
		{
			p_listeners[i]->wait() ;
		}  
	}
	else
	{
		GTI_LOG_INFO_MSG( GTI_CONSOLE_LOG, " NO device to listen "); 
	}
  
	p_rfm -> fini();
  }
  
  GTI_LOG_INFO_MSG( GTI_FILE_LOG, " EXIT ");

  return 0;
}
