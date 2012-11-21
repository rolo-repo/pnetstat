#include "DeviceListener.h"
#include "ReferenceDataManager.h"

#include "ace/Thread_Manager.h"

void DeviceListener::refresh()
{
	//prevent duble entry and polling during the refresh
	//ACE_GUARD(ACE_Thread_Mutex, guard, m_refresh_lock);

	if ( m_lifeId == ReferenceDataManager::instance()->getLiveID() )
		return;

	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"About to stop sniff thread device " << m_device.c_str() );
	stop_sniff();

	ACE_INT32 liveId = 0;

	ConnInfoContainer new_addr;

	//The m_addr can not be changed in there structure only the upload or download values are changed
	ConnInfoContainer::index<local_remote>::type& old_index = m_addr.get<local_remote>();

	if ( m_singleProcess ) 
	{
		ConnInfoContainer::index<local_remote>::type::iterator it = old_index.begin();

		liveId = ReferenceDataManager::instance()->getProcessConnections(  m_device , it->getOwnerID() , new_addr );
	} 
	else 
	{
		ReferenceDataManager::DevToConnInfoMap deviceToConnection;

		liveId = ReferenceDataManager::instance()->getAllConnections( deviceToConnection );

		if ( liveId > 0) {
			ReferenceDataManager::DevToConnInfoMap::const_iterator iter = deviceToConnection.find(m_device);

			if ( iter == deviceToConnection.end() ) {
				GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Failed to find connections for current device " << m_device.c_str() <<  " will try later");
				m_addr.clear();
				return;
			}

			new_addr = iter->second;
		}
	}

	if ( liveId < 0) {
		GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Error in refresh will try later");
		return;
	}

	//This is condition to ensure the refresher has exited
	//ACE_GUARD(ACE_Thread_Mutex, guard, m_cond_mutex );

	//No need to gurd the data any more thread is down

	// Making the data merge
	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"START Refresh DeviceListener " << m_device.c_str()  <<" data liveID " << m_lifeId << "->" << liveId) ;

	const ConnInfoContainer::index<local_remote>::type& new_index = new_addr.get<local_remote>();

	BOOST_FOREACH( const ConnInfo &addr_data , old_index  ) 
	{
		ConnInfoContainer::index<local_remote>::type::iterator it = new_index.find(boost::make_tuple( addr_data.getLocalAddr(), addr_data.getRemoteAddr() ));

		if ( it != new_addr.get<local_remote>().end() ) 
		{
			it->m_uload = addr_data.m_uload;
			it->m_dload = addr_data.m_dload;

			it->m_lastUpdate = addr_data.m_lastUpdate;

			it->m_delta_uload = addr_data.m_delta_uload;
			it->m_delta_dload =  addr_data.m_delta_dload;
		}
	}

	m_addr.clear();
	m_addr = new_addr;

	buildFilter();

	m_lifeId = liveId;

	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"END Refresh DeviceListener " << m_device.c_str()  <<" data liveID = " << m_lifeId) ;

	activate();
}
////////////////////////////////////////////////////////////////////////////////////////////////
const GTI_String& DeviceListener::getDeviceName() const
{
	return m_device;
}
////////////////////////////////////////////////////////////////////////////////////////////////
bool DeviceListener::buildFilter()
{
	GTI_String filter;

	ConnInfoContainer::index<local>::type& index = m_addr.get<local>();

	ACE_INET_Addr prev_addr;

	BOOST_FOREACH( const ConnInfo &addr , index  ) 
	{
		if ( prev_addr == addr.getLocalAddr() )
			continue;

		prev_addr = addr.getLocalAddr();

		filter += ( filter.empty() ) ? "( host " : " or ( host ";
		filter += addr.getLocalAddr().get_host_addr();
		filter += " and port ";

		std::ostringstream t;
		t << (addr.getLocalAddr().get_port_number());

		filter += t.str();
		filter +=" )";
	}

	if ( m_filter )
		delete [] m_filter;

	m_filter = 0;

	if ( filter.size() < 512 ) 
	{
		m_filter = new ACE_TCHAR[ filter.size() + 1 ];

		ACE_OS::strcpy( m_filter , filter.c_str() );
	} 
	else 
	{
		m_filter = new ACE_TCHAR[ sizeof("tcp") + 1 ];

		strcpy(m_filter,"tcp");

		GTI_LOG_INFO_MSG(GTI_FILE_LOG,"Filter is too long working without kernel filtering" );
	}

	return true;
}


bool DeviceListener::pollData(std::vector<ConnInfo> & o_container ) const
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_refresh_lock , false);

	const ConnInfoContainer::index<local_remote>::type& index = m_addr.get<local_remote>();

	// ACE_INET_Addr prev_addr;

	BOOST_FOREACH( const ConnInfo &addr , index  ) {
		o_container.push_back(addr);

		addr.m_delta_uload = 0;
		addr.m_delta_dload = 0;
	}

	return true;
}


bool DeviceListener::updateConnInfo ( const ACE_INET_Addr &i_src , const ACE_INET_Addr &i_dest , ACE_UINT16 i_size )
{
	ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_refresh_lock , false);
	// bool rc = false;

	//ConnInfoContainer::index<local_remote>::type& index = ;

	// look for package outgoing this machine
	ConnInfoContainer::index<local_remote>::type::iterator it = m_addr.get<local_remote>().find(boost::make_tuple( i_src, i_dest ));

	if( it != m_addr.get<local_remote>().end() ) 
	{
		it->m_lastUpdate = ACE_OS::time(0);
		it->m_uload += i_size ;
		it->m_delta_uload += i_size ;
		GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Upload package " << it->m_uload  << " From " << it->getLocalAddr() << " TO " << it->getRemoteAddr() ) ;
		return true;
	}

	// look for package incoming this machine
	ConnInfoContainer::index<remote_local>::type::iterator it2 = m_addr.get<remote_local>().find(boost::make_tuple( i_src, i_dest ));

	if( it2 != m_addr.get<remote_local>().end() ) 
	{
		it2->m_lastUpdate = ACE_OS::time(0);
		it2->m_dload += i_size ;
		it2->m_delta_dload += i_size ;
		GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Down package " << it2->m_dload << " From " << it2->getRemoteAddr() << " TO " << it2->getLocalAddr() );
		return true;
	}


	GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Uknown package " << i_src << " -> " << i_dest << "size " << i_size );

	return false;
}

#ifndef WIN32
void pcap_packet_cb( u_char *args, const struct pcap_pkthdr *header, const u_char *packet )
{
	DeviceListener *pListener = reinterpret_cast<DeviceListener*>(args);

	pListener->handle_packet( header, packet );
}


//////////////////////////////////////////////////////////////////////////

void DeviceListener::handle_packet( const struct pcap_pkthdr *i_pHeader, const u_char *i_pPacket )
{
	/* declare pointers to packet headers */
	const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	const struct sniff_ip *ip;              /* The IP header */
	const struct sniff_tcp *tcp;            /* The TCP header */
	const char  *payload;                   /* Packet payload */
	int size_ip;
	int size_tcp;
	int size_payload;

	ACE_INET_Addr dstAddr;
	ACE_INET_Addr srcAddr;


	ACE_TCHAR omessage[1024];
	ACE_UINT64 osize = 0;
	GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"message size:" << i_pHeader->len  << "+" << i_pHeader->caplen
	                  << "\n============\n"
	                  << GTI_Constants::printBuffer( (const char*)i_pPacket ,i_pHeader->len ,omessage, sizeof(omessage) , osize)
	                  << "============" );

// printf("\nPacket number %d:\n", count++);

	/* define ethernet header */
	ethernet = (struct sniff_ethernet*)(i_pPacket);

	/* define/compute ip header offset */

	ip = (struct sniff_ip*)(i_pPacket + SIZE_ETHERNET);

	size_ip = IP_HL(ip)*4; // IP header length

	if (size_ip < 20) {
		//GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Invalid IP header length:" << size_ip << " bytes "  );
		//printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}



	/* print source and destination IP addresses */
	//	printf("       From: %s\n", inet_ntoa(ip->ip_src));
	//	printf("         To: %s\n", inet_ntoa(ip->ip_dst));
	/* determine protocol */

	switch(ip->ip_p) {
		case IPPROTO_TCP:
			//	printf("   Protocol: TCP\n");
			break;
		case IPPROTO_UDP:
			//	printf("   Protocol: UDP\n");
			return;
		case IPPROTO_ICMP:
			//	printf("   Protocol: ICMP\n");
			return;
		case IPPROTO_IP:
			//	printf("   Protocol: IP\n");
			return;
		default:
			//	printf("   Protocol: unknown\n");
			return;
	}
	/*
	*  OK, this packet is TCP.
	*/
	/* define/compute tcp header offset */
	tcp = (struct sniff_tcp*)(i_pPacket + SIZE_ETHERNET + size_ip);
	size_tcp = TH_OFF(tcp)*4; // TCP header offset

	if (size_tcp < 20) {
		//GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Invalid TCP header length:" << size_tcp << " bytes "  );
		// printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
		return;
	}

	//	printf("   Src port: %d\n", ACE_NTOHS(tcp->th_sport));
	//	printf("   Dst port: %d\n", ACE_NTOHS(tcp->th_dport));

	dstAddr.set( ACE_NTOHS(tcp->th_dport ) , ACE_NTOHL(ip->ip_dst.s_addr));
	srcAddr.set( ACE_NTOHS(tcp->th_sport ) , ACE_NTOHL(ip->ip_src.s_addr));

	GTI_LOG_DEBUG_MSG(GTI_FILE_LOG,"Protocol: TCP From: " << srcAddr << " -> " << "To: " << dstAddr);

	/* define/compute tcp payload (segment) offset */
	payload = (const char *)(i_pPacket + SIZE_ETHERNET + size_ip + size_tcp);

	/* compute tcp payload (segment) size */
	size_payload = ACE_NTOHS(ip->ip_len) - ( size_ip + size_tcp );
	//printf("Payload size: %d %d %d \n",ACE_NTOHS(ip->ip_len) , size_ip , size_tcp);

	updateConnInfo( srcAddr , dstAddr ,  i_pHeader->len );

	return;
}

int  DeviceListener::stop_sniff()
{
	if ( m_pPH ) {
		/*	{
				ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_sniff_cond , -1);
				pcap_breakloop(m_pPH);
			}*/
		{
			ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_refresh_lock , -1);

			ACE_Thread_Manager::instance ()->cancel_task(this,1);
			//ACE_Thread_Manager::instance ()->kill_task(this,SIGABRT);
		}
		
		this->wait();
		
		{
			ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_sniff_cond , -1);

			if ( m_pPH == 0 )
				pcap_close( m_pPH );
		}

		m_pPH = 0;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////

int DeviceListener::start_sniff( )
{
	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"START sniffing on " << m_device.c_str() );

	pcap_t *handle;			/* Session handle */
	const char *dev = m_device.c_str();			/* The device to sniff on */
	char errbuf[PCAP_ERRBUF_SIZE];				/* Error string */
	struct bpf_program fp;					/* The compiled filter */
	// and ( tcp[tcpflags] & (tcp-syn|tcp-fin) != 0 )
	char *filter_exp = m_filter ;// [] = "port 15000";	/* The filter expression */
	bpf_u_int32 mask = 0;		/* Our netmask */
	bpf_u_int32 net = 0;		/* Our IP */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const u_char *packet;		/* The actual packet */

	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_sniff_cond , -1);
		/* Find the properties for the device */
		if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
			//printf( "Couldn't get netmask for device %s: %s\n", dev, errbuf);
			GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Couldn't get netmask for device " << dev << " Error "  << errbuf );
			net = 0;
			mask = 0;
			return -1;
		}

		//GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"IP " << (ACE_UINT64)net << "MASK " << (ACE_UINT64)mask);
		/* Open the session in promiscuous mode */
		handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
		if (handle == NULL) {
			GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Couldn't open device " << dev << " Error "  << errbuf );

			// printf( "Couldn't open device %s: %s\n", dev, errbuf);
			return -1;
		}

		m_pPH = handle;

		if ( filter_exp ) {
			GTI_LOG_INFO_MSG(GTI_FILE_LOG,"FILTER : " << filter_exp );

			/* Compile and apply the filter */
			if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
				GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Couldn't parse filter " << filter_exp << " Error "  << pcap_geterr(handle) );
				// fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
				return -1;
			}

			if (pcap_setfilter(handle, &fp) == -1) {
				GTI_LOG_ERROR_MSG(GTI_FILE_LOG,"Couldn't install filter " << filter_exp << " Error "  << pcap_geterr(handle) );
				//printf( "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
				return -1;
			}
		}
	} // end gurad
	/* Grab a packet */
	//	packet = pcap_next(handle, &header);


// m_sniff_cond.signal();

	/* now we can set our callback function */
	pcap_loop( handle, -1/*50*/ , pcap_packet_cb,  reinterpret_cast<u_char*>(this) );

	/* Print its length */
	//	printf("Jacked a packet with length of [%d]\n", header.len);
	/*	ACE_TCHAR omessage[1024];
	ACE_UINT64 osize = 0;
	GTI_LOG_INFO_MSG(GTI_CONSOLE_LOG,"message size:" << header.len << "\n============\n" <<
	GTI_Constants::printBuffer((const char*)packet,header.len,omessage,sizeof(omessage),osize)
	<< "============" );

	*/
	//got_packet( NULL , &header , packet );
	//GTI_LOG_ERROR_MSG(GTI_CONSOLE_LOG,"length of" << header.len <<  " " << GTI_String((const char*)packet,header.len).c_str() );
	/* And close the session */
	{
		ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_sniff_cond , -1);
		pcap_close( handle );
	}

	m_pPH = 0;
// m_sniff_cond.signal();

	GTI_LOG_INFO_MSG(GTI_FILE_LOG,"STOP sniffing on " << m_device.c_str() );
	return 0;
}

#endif
