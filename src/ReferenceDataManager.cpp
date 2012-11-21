#include "ReferenceDataManager.h"

#include "boost/foreach.hpp"

const ACE_TCHAR ReferenceDataManager::SOCKET_LINK_PREFIX[] = "socket:[";

void ReferenceDataManager::print()
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, m_mutex );
  
  GTI_LOG_INFO_MSG(GTI_FILE_LOG,"###LiveID### " << m_lifeId );
  
  GTI_LOG_INFO_MSG(GTI_FILE_LOG,"###IpToDevMap### " << (ACE_UINT64)m_ip_to_dev.size());
  BOOST_FOREACH(IpToDevMap::value_type val, m_ip_to_dev)
  {
    GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Val:" <<  val.first << " " << val.second.c_str() );
  }

  GTI_LOG_INFO_MSG(GTI_FILE_LOG,"###InodToConnMap### " << (ACE_UINT64)m_inod_to_conn.size() );
  BOOST_FOREACH( InodToConnMap::value_type val, m_inod_to_conn )
  {
    GTI_LOG_INFO_MSG(GTI_FILE_LOG,"Val:" <<  ( (ACE_UINT64) val.first ) << " " << (ACE_TCHAR*) val.second.print(val.first) );
  }

  GTI_LOG_INFO_MSG(GTI_FILE_LOG,"###PidToInodMap### " << (ACE_UINT64) m_pid_to_inod.size() );
  BOOST_FOREACH( PidToInodMap::value_type val, m_pid_to_inod)
  {
    BOOST_FOREACH( const ino_t &val1, val.second )
      GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Val:" <<  val.first << " " << (ACE_UINT64) val1 );
  }

  GTI_LOG_INFO_MSG(GTI_FILE_LOG,"###DevToDevStatMap##### " << (ACE_UINT64)m_dev_dev_stat.size());
  BOOST_FOREACH( DevToDevStatMap::value_type val, m_dev_dev_stat )
  {
    GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Val:" <<  val.first.c_str() << " " << val.second.rx << " " <<val.second.tx );
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool ReferenceDataManager::init( ACE_UINT32 i_pid )
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);
  
  if (  true == m_isInitialized )
  	return m_isInitialized;
  	
  m_lifeId = 1;
  m_pid = i_pid;
  m_localIP = ACE_INET_Addr(ACE_MAX_DEFAULT_PORT, ACE_LOCALHOST).get_ip_address();

#ifndef WIN32 
  if ( readNetworkInterfaces() && /* fill map ip -> device name m_ip_to_dev */
       readProcNetDev() )  /* fill map dev -> dev usage m_dev_dev_stat */
  {
  	if ( 0 == m_pid )
			readProcXFd();  /* fill map pid -> socket fd m_pid_to_inod */
		else
			readProcPIDFd(m_pid); /* fill map pid -> socket fd m_pid_to_inod */
		
		readProcNetTCP() ; /* fill map socked fd -> connect data m_inod_to_conn */
	
		m_refresher.activate();
		
		m_initial_dev_dev_stat = m_dev_dev_stat;
	
    m_isInitialized = true;
  }
#else
  m_isInitialized = true;
#endif //WIN32 

  return m_isInitialized;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool  ReferenceDataManager::reinit()
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);    
#ifndef WIN32
  //fill map pid -> socket fd m_pid_to_inod
  if ( m_isInitialized )
  {
  	if ( 0 == m_pid && readProcXFd() && readProcNetTCP() ) 
  		m_lifeId++;
  	else if ( 0 != m_pid && readProcPIDFd( m_pid ) && readProcNetTCP() )
  		m_lifeId++;
  }
        
#endif //WIN32 
   return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////

bool ReferenceDataManager::fini()
{
	if ( m_isInitialized )
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);    
		
		m_refresher.stop();
		
		m_refresher.wait();
		
		m_isInitialized = false;
	}
	return true;
}

#ifndef WIN32 
////////////////////////////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readNetworkInterfaces()
{
	/*
  //struct ifaddrs 
  //struct ifaddrs  *ifa_next;    Next item in list 
  //char            *ifa_name;     Name of interface
  //unsigned int     ifa_flags;   Flags from SIOCGIFFLAGS
  //struct sockaddr *ifa_addr;    Address of interface 
  //struct sockaddr *ifa_netmask; Netmask of interface 
  //union {
  //  struct sockaddr *ifu_broadaddr;
  //  Broadcast address of interface 
  //  struct sockaddr *ifu_dstaddr;
  //   Point-to-point destination address 
  //} ifa_ifu;
  */
  
  /*
   struct ifconf ifc;
   struct ifreq *ifr;
   int           sck;
   char          buf[1024];

	sck = socket(AF_INET, SOCK_DGRAM, 0);
	if(sck < 0)
	{
		perror("socket");
		return 1;
	}


	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl(SIOCGIFCONF)");
		return 1;
	}
	//strcpy( ifr.ifr_name , "eth0" ); 
	
	ifr = ifc.ifc_req;
    
	int nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    
    for ( int i = 0; i < nInterfaces; i++)
    {
        struct ifreq *item = &ifr[i];
        
        if(ioctl(sck, SIOCGIFFLAGS, &ifr[i]) >= 0)
        {
            if (!(ifr[i].ifr_flags & IFF_LOOPBACK))
            {
                printf("Name : %s\n", item->ifr_name);
                printf("Media : %s\n", (ifr->ifr_flags)&IFF_UP ? "Up" : "Down");
                printf("Bandwidth : %d\n", item->ifr_bandwidth);
			
            //    Get media type : wired or wireless 
             //   Get correct baudrate, baudrate is not bandwidth, am I correct?
               
            }
        }
    }
 */
  struct ifaddrs *addrs, *iap;
  struct sockaddr_in *sa;
  char buf[256];

  ACE_UINT32 inetaddr;

  getifaddrs(&addrs);

  for ( iap = addrs; iap != NULL; iap = iap->ifa_next ) 
  {
    if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) && iap->ifa_addr->sa_family == AF_INET) 
    {
      sa = (struct sockaddr_in *)(iap->ifa_addr);
      //get the buff in XXX.XXX.XXX  presentation
      inet_ntop( iap->ifa_addr->sa_family, (void *)&(sa->sin_addr), buf , sizeof(buf) );

      inetaddr = ACE_NTOHL(inet_addr(buf));

      GTI_LOG_INFO_MSG(GTI_FILE_LOG,"Found network interface [" << buf << "] [" << iap->ifa_name << "] [" << inetaddr << "]");

      m_ip_to_dev.insert(std::make_pair( inetaddr ,GTI_String(iap->ifa_name))); 
    }
  }

  freeifaddrs(addrs);

  return ( m_ip_to_dev.size() > 1 ) ? true : false ; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readProcNetTCP()
{
  /*
  st = 0A = LISTEN
  st = 01 = ESTABLISHED
  st = 06 = TIME_WAIT
  */
 
  enum { LISTEN = 0x0A, ESTABLISHED = 0x01 , TIME_WAIT = 0x06 };
 
  FILE *f = NULL;
  InodToConnMap inod_to_conn;

  ino_t inode;
  ACE_UINT32 connSt;
  ConnData connData;
  // ACE_UINT32 locaddr, locport, remaddr, remport, uid, inode;

  ACE_TCHAR big_str[512];

  if ( ( f = fopen("/proc/net/tcp", "r")) == NULL )
  {
    // perror("Failed to open /proc/net/tcp");

    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to open /proc/net/tcp");

    return false;
  }
  
  //remove the first line
  fgets(big_str, sizeof(big_str) ,f);

  while( fgets(big_str, sizeof(big_str) ,f) )
  {
    sscanf( big_str, "%*d: %8x:%4x %8x:%4x %2x %*8x:%*8x %*2x:%*8x %*8x %d  %*d %lu \n",&(connData.locaddr), (ACE_UINT32 *) &(connData.locport), &(connData.remaddr), (ACE_UINT32 *)  &(connData.remport), &connSt, &(connData.uid), &inode);

    if ( inode !=0 && connSt == ESTABLISHED )
    {
      connData.locaddr = ACE_NTOHL(connData.locaddr);
      connData.remaddr = ACE_NTOHL(connData.remaddr);

      inod_to_conn.insert(std::make_pair( inode , connData ));
    }
    //printf("LocAddr:%u LocPort:%d RemAddr:%u RemPort:%d uid:%d inode:%d \n",locaddr, locport, remaddr, remport, uid, inode);
  }
  
  ACE_OS::fclose(f);
  
  if (! fast_map_compare( inod_to_conn , m_inod_to_conn ) ) 
    m_inod_to_conn.swap( inod_to_conn );
  else
    return false;
  
  //Clear
  inod_to_conn.clear();
  
  return true;
}

//////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readInode( ino_t& o_inode, const ACE_TCHAR* i_pPath )  const
{
  char buf[512];

  ssize_t n = ACE_OS::readlink( i_pPath, buf, sizeof(buf) - 1);

  if ( -1 == n )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to read link" << i_pPath);
    return false;
  } 

  buf[n] = '\0'; // set \0

  if ( ACE_OS::memcmp( SOCKET_LINK_PREFIX, buf, sizeof(SOCKET_LINK_PREFIX) - 1) )
    return false;

  char *endptr;

  //ino_t is 8 bytes
  unsigned long long int inode_ul = strtoull( buf + sizeof(SOCKET_LINK_PREFIX) - 1, &endptr, 10 );

  if (*endptr != ']')
    return false;

  if ( inode_ul == ULLONG_MAX )
    return false;

  o_inode = inode_ul;

  return true;
}


//////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readProcPIDFd( const ACE_UINT32 &i_pid , std::set<ino_t> &o_inods ) const
{
  char buf[512];

  ACE_OS::snprintf(buf, sizeof(buf), "/proc/%u/fd", i_pid);
 /*
  uid_t uid = ACE_OS::getuid();
 
  //Check that user is owner of the process
  {
  char tbuf[256];

  ACE_stat statbuf;

  ACE_OS::snprintf( tbuf, sizeof(tbuf), "/proc/%lu", i_pid );

  if ( ACE_OS::stat(tbuf, &statbuf) < 0 )
  return 0;

  if ( uid != statbuf.st_uid )
  {
  GTI_LOG_DEBUG_MSG(GTI_CONSOLE_LOG,"The process is owned by deferent user you are not allowed to listen to it PID: " << i_pid );
  return false;
  }
  }*/

  GTI_LOG_DEBUG_MSG( GTI_FILE_LOG,"Checking PID" << i_pid);
  //printf("Checking process id %lu\n" , i_pid);

  ACE_DIR* fd = ACE_OS::opendir(buf);

  if (!fd)
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to open directory " << buf << " " << ACE_OS::strerror( ACE_OS::last_error( ) ) );
    //perror("Failed to open the directory %s ",buf);
    return false;
  }

  struct ACE_DIRENT* dent;

  while (( dent = ACE_OS::readdir(fd) )) 
  {
    if ( *(dent->d_name) == '.' )
      continue;

    if (  ACE_OS::snprintf( buf, sizeof(buf), "/proc/%u/fd/%s", i_pid, dent->d_name ) >= sizeof(buf) - 1 ) 
    {
      GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to read directory " << buf << ACE_OS::strerror(ACE_OS::last_error()));
      continue;
    }

    ino_t fd_inode;

    if ( readInode( fd_inode, buf ) ) 
    {
      //  printf("Found socket inode %llu\n",fd_inode);	

      o_inods.insert(fd_inode);
    }
    else
    {
      //  printf("Not a socket %s\n",buf);	
    }
  }

  ACE_OS::closedir(fd);

  return (o_inods.size() > 0 ) ? true : false;

}

//////////////////////////////////////////////////////////////////////////

bool ReferenceDataManager::readProcPIDFd (const ACE_UINT32 &i_pid )
{
	PidToInodMap pid_to_inod;
		
	std::set<ino_t> inodes;
			
	if ( readProcPIDFd( i_pid, inodes ) )
    {
      pid_to_inod.insert( make_pair(i_pid,inodes) );
    }
    
    
   if (! full_map_compare( pid_to_inod , m_pid_to_inod ) ) 
    m_pid_to_inod.swap(pid_to_inod);
   else
    return false;
	
  pid_to_inod.clear();

  return ( m_pid_to_inod.size() > 0 ) ? true : false;
}
	
	
//////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readProcXFd()
{
  PidToInodMap pid_to_inod;
  
  ACE_DIR* proc = ACE_OS::opendir("/proc");

  if (!proc)
    return false;

  //m_pid_to_inod.clear();

 // uid_t uid = getuid();
  ACE_DIRENT* dent;

  while ((dent = ACE_OS::readdir(proc))) 
  {
    char *endptr;
    ACE_UINT32 pid_ul = strtoul(dent->d_name, &endptr, 10);

    if ( pid_ul == ULONG_MAX || *endptr )
      continue;

    std::set<ino_t> inodes;

    if ( readProcPIDFd( pid_ul,inodes ) )
    {
      pid_to_inod.insert( make_pair(pid_ul,inodes) );
    }
  }

  ACE_OS::closedir(proc);
	
  if (! full_map_compare( pid_to_inod , m_pid_to_inod ) ) 
    m_pid_to_inod.swap(pid_to_inod);
  else
    return false;
	
  pid_to_inod.clear();

  return ( m_pid_to_inod.size() > 0 ) ? true : false;
}

//////////////////////////////////////////////////////////////////////////
bool ReferenceDataManager::readProcNetDev()
{
  char dev[256];
  char big_str[512];
  DevStat stat;

  m_dev_dev_stat.clear();

  FILE *f;

  if ( ( f = fopen("/proc/net/dev", "r")) == NULL )
  {
    //perror("Failed to open /proc/net/dev");

    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to open /proc/net/dev");

    return false;
  }

  //Skip first 2 lines it is header
  fgets(big_str,sizeof( big_str ),f);
  fgets(big_str,sizeof( big_str ),f);

  //Validate we are on the correct line
  if( ( strstr( big_str, "compressed" ) ) == NULL )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"/proc/net/dev header format is not supported");
	ACE_OS::fclose(f);
    return false;
  }	

  while( fgets( big_str, sizeof( big_str ), f ) )
  {
    //find the : it delimits device name from data
    char * delim = ACE_OS::strchr( big_str, ':' );

    char *p = dev;

    //trim device name
    for ( int i = 0 ; i < ( delim - big_str) ; i++)
    {
      if ( big_str[i] != ' ' )
        *(p++) = big_str[i];
    }

    *p = '\0'; // put null ind

    sscanf( delim + 1,"%llu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %llu %*lu %*lu %*lu %*lu %*lu %*lu %*lu",&(stat.rx), &(stat.tx) );

    //	printf(">>>>>> %lu %lu\n",)
    m_dev_dev_stat.insert(std::make_pair(GTI_String(dev),stat));
  }

  ACE_OS::fclose(f);
  
  return (m_dev_dev_stat.size() > 0) ? true : false;
}

/*static*/
GTI_String ReferenceDataManager::getProccessName(const ACE_UINT32 &i_pid ,const ACE_UINT16 & i_size)
{
  char buf[512];
  char big_str[1024];

  ACE_OS::snprintf(buf, sizeof(buf), "/proc/%lu/cmdline", i_pid);
 /*
  uid_t uid = ACE_OS::getuid();
 
  //Check that user is owner of the process
  {
  char tbuf[256];

  ACE_stat statbuf;

  ACE_OS::snprintf( tbuf, sizeof(tbuf), "/proc/%lu", i_pid );

  if ( ACE_OS::stat(tbuf, &statbuf) < 0 )
  return 0;

  if ( uid != statbuf.st_uid )
  {
  GTI_LOG_DEBUG_MSG(GTI_CONSOLE_LOG,"The process is owned by deferent user you are not allowed to listen to it PID: " << i_pid );
  return false;
  }
  }*/

  GTI_LOG_DEBUG_MSG( GTI_FILE_LOG,"Checking PID" << i_pid << "File " << buf);
  //printf("Checking process id %lu\n" , i_pid);

  FILE *f = NULL;
  
  if ( ( f = fopen(buf, "r")) == NULL )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to open" << buf );
	
	return "?";
  }
 
  GTI_String processName;
  //remove the first line
 // fgets(big_str, sizeof(big_str) ,f);
  //printf("%s\n", big_str);

  while( fgets(big_str, sizeof(big_str) ,f) )
  {
    processName += big_str;
  }
  
  ACE_OS::fclose(f);
   
  int pos = processName.find_last_of("/\\");
  
  //return the file name of the path , remove the base name
  return processName.substr(pos + 1).substr(0,i_size);	
}

#endif // WIN32 
//////////////////////////////////////////////////////////////////////////

/* returns liveID
 * -1 on failure
 */
ACE_INT32 ReferenceDataManager::getAllConnections ( DevToConnInfoMap &o_devices   ) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);
 
  bool found = false;
  
  BOOST_FOREACH( const PidToInodMap::value_type &pid_to_inod, m_pid_to_inod )
  {
	  GTI_LOG_DEBUG_MSG( GTI_FILE_LOG,"getAllConnections checking PID " <<  (ACE_UINT32)pid_to_inod.first );

	  if ( 0 >  getProcessConnections( pid_to_inod.first , o_devices  ) )
	  {
		  GTI_LOG_INFO_MSG( GTI_FILE_LOG,"PID " << (ACE_UINT32)pid_to_inod.first << " [" << getProccessName(pid_to_inod.first).c_str() << "] has no TCP connection" );
		  continue;
	  }
	  
	  found = true;
  }
  
  return ( found ) ? m_lifeId : -1 ;
}

/* returns lifeID on success
 * -1 on failure
 * */
//////////////////////////////////////////////////////////////////////////
ACE_INT32 ReferenceDataManager::getProcessConnections ( const ACE_UINT32 &i_pid , ReferenceDataManager::DevToConnInfoMap &o_devices   ) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);

  PidToInodMap::const_iterator pidIter = m_pid_to_inod.find( i_pid ) ;

  if ( pidIter == m_pid_to_inod.end() ) 
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find any data for PID " << i_pid );
    return -1;
  }

  GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Checking PID " << i_pid );

  BOOST_FOREACH( ino_t inod , pidIter->second )
  {
    GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Checking PID " << i_pid << " inod " << (ACE_UINT32) inod);

    InodToConnMap::const_iterator connIter = m_inod_to_conn.find( inod );

    if ( connIter == m_inod_to_conn.end() ) 
    {
      //  GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find INOD " << (ACE_UINT32) inod );
      continue;
    }

	IpToDevMap::const_iterator devIter;
	
	if ( connIter->second.locaddr != connIter->second.remaddr )
    {
		devIter = m_ip_to_dev.find( connIter->second.locaddr );
	}
	else
	{
		devIter = m_ip_to_dev.find( m_localIP );	
	}

	if ( devIter == m_ip_to_dev.end() ) 
	{
		GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device for IP  " << connIter->second.locaddr );
		return -1;
	}

	GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Found conenction to listen inode " << (ACE_UINT32)inod << " device "  << devIter->second.c_str() << " PID " << (ACE_UINT32)i_pid) ;

	o_devices[devIter->second].insert( ConnInfo ( i_pid , ACE_INET_Addr( connIter->second.locport , connIter->second.locaddr ) , ACE_INET_Addr( connIter->second.remport , connIter->second.remaddr ) ) );
  }
 
  return  m_lifeId;
}

//////////////////////////////////////////////////////////////////////////
ACE_INT32 ReferenceDataManager::getProcessConnections ( const GTI_String & i_dev , const ACE_UINT32 &i_pid , ConnInfoContainer &o_connects ) const
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , false);

  PidToInodMap::const_iterator pidIter = m_pid_to_inod.find( i_pid ) ;

  if ( pidIter == m_pid_to_inod.end() ) 
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find any data for PID " << i_pid );
    return -1;
  }

  GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Checking PID " << i_pid );

  BOOST_FOREACH( ino_t inod , pidIter->second )
  {
    GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Checking PID " << i_pid << " inod " << (ACE_UINT32) inod);

    InodToConnMap::const_iterator connIter = m_inod_to_conn.find( inod );

    if ( connIter == m_inod_to_conn.end() ) 
    {
      //  GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find INOD " << (ACE_UINT32) inod );
      continue;
    }

    IpToDevMap::const_iterator devIter = m_ip_to_dev.find( connIter->second.locaddr );

    if ( devIter == m_ip_to_dev.end() ) 
    {
      GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device for IP  " << connIter->second.locaddr );
      return -1;
    }

	// we need to filter only for device that was requested and 
	// the connection that have both remote and local same IP is lo
	if (  ( i_dev == getDevName( m_localIP ) && 
		    ( i_dev == devIter->second  || connIter->second.locaddr == connIter->second.remaddr ) 
		  ) || ( i_dev == devIter->second && connIter->second.locaddr != connIter->second.remaddr)
	   )		
	{
		GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Found conenction to listen inode " << (ACE_UINT32)inod << " device "  << i_dev.c_str() << " PID " << (ACE_UINT32)i_pid) ;

		o_connects.insert( ConnInfo ( i_pid , ACE_INET_Addr( connIter->second.locport , connIter->second.locaddr ) , ACE_INET_Addr( connIter->second.remport , connIter->second.remaddr ) ) );
	}
	/*else if ( i_dev == devIter->second && connIter->second.locaddr != connIter->second.remaddr)
	{
		GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Found conenction to listen inode " << (ACE_UINT32)inod << " device "  << i_dev.c_str() << " PID " << (ACE_UINT32)i_pid) ;

		o_connects.insert( ConnInfo ( i_pid , ACE_INET_Addr( connIter->second.locport , connIter->second.locaddr ) , ACE_INET_Addr( connIter->second.remport , connIter->second.remaddr ) ) );
	}*/
	
	/*
	//We need to look for particular device
	// in case i_dev is not lo need to check if local ip == remote ip then it is lo
    if (  i_dev != getDevName( m_localIP ) && i_dev != devIter->second  && connIter->second.locaddr == connIter->second.remaddr )
      continue;
	
	GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Found conenction to listen inode " << (ACE_UINT32)inod << " device "  << i_dev.c_str() << " PID " << (ACE_UINT32)i_pid) ;

	o_connects.insert( ConnInfo ( i_pid , ACE_INET_Addr( connIter->second.locport , connIter->second.locaddr ) , ACE_INET_Addr( connIter->second.remport , connIter->second.remaddr ) ) );
  
   */ 
   }
 
 return  m_lifeId;
}

///////////////////////////////////////////////////////////////////////////////////////////

GTI_String ReferenceDataManager::getDevName( const ACE_UINT32 &i_ip ) const 
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , "");    
  
  IpToDevMap::const_iterator devIter =  m_ip_to_dev.find(i_ip);

  if ( devIter == m_ip_to_dev.end() )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device for IP " << i_ip );
    return "?";
  }	
  
  return devIter->second;
}

/////////////////////////////////////////////////////////////////////////////////////
GTI_String ReferenceDataManager::getDevName( const ConnInfo &i_conn ) const 
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , "");    
  
  if ( i_conn.getLocalAddr().get_ip_address() == i_conn.getRemoteAddr().get_ip_address()) 
  {
	 return getDevName( m_localIP );
  }
  
  return getDevName( i_conn.getLocalAddr().get_ip_address() );
}

//////////////////////////////////////////////////////////////////////////

ACE_UINT64 ReferenceDataManager::getDevUsage ( const GTI_String& i_devName , ACE_INT16 i_usageType  )
{
	ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , ACE_UINT64_MAX);
	
	ACE_UINT64 usage = ACE_UINT64_MAX;
#ifndef WIN32
  readProcNetDev();
#endif

  DevToDevStatMap::const_iterator devStatIter = m_dev_dev_stat.find( i_devName );

  if ( devStatIter == m_dev_dev_stat.end() )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device " << i_devName.c_str() );
    return ACE_UINT64_MAX;
  }
  
  DevToDevStatMap::const_iterator prev_devStatIter = m_initial_dev_dev_stat.find( i_devName );
  
  if ( prev_devStatIter == m_initial_dev_dev_stat.end() )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device " << i_devName.c_str() );
    return ACE_UINT64_MAX;
  }
  
  //Calculate the deferenece from initial state
  DevStat stat = (devStatIter->second) - (prev_devStatIter->second);
  
  switch ( i_usageType )
  {
	case 0:
		usage = stat.tx; // out
		break;
	case 1:
		usage = stat.rx; // in
		break;
	case 2:
		usage = stat.tx + stat.rx; //total
		break;
  }
  
  GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Device delta usage U " << stat.tx  << " D " << stat.rx << " T " << usage);
  
  return ( usage == 0 ) ?  ACE_UINT64_MAX : usage;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ACE_UINT64 ReferenceDataManager::getDevUsage ( const ACE_UINT32 &i_ip , ACE_INT16 i_usageType  )
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, m_mutex , ACE_UINT64_MAX);    
  
  IpToDevMap::const_iterator devIter =  m_ip_to_dev.find(i_ip);

  if ( devIter == m_ip_to_dev.end() )
  {
    GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find device for IP " << i_ip );
    return ACE_UINT64_MAX;
  }
  
  return getDevUsage(devIter->second , i_usageType );
}

///////////////////////////////////////////////////////////////////////

int ReferenceDataRefresher::handle_timeout (const ACE_Time_Value &,
											const void *)
{
  return ( ReferenceDataManager::instance()->reinit() ) ? 0 : -1;

/*
  ACE_Time_Value txv = ACE_OS::gettimeofday ();
  ACE_DEBUG ((LM_DEBUG,
              "\nTimer #%d fired at %d.%06d (%T)!\n",
              this->tid_,
              txv.sec (),
              txv.usec ()));
  delete this;

  return 0
*/
}

