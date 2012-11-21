#ifndef __ConnectionInfo_H__
#define __ConnectionInfo_H__

#include "ace/Basic_Types.h"
#include "ace/INET_Addr.h"


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/tag.hpp>

class ConnInfo
{
public:
  ConnInfo( const ACE_UINT32 &i_pid , const ACE_INET_Addr &i_local ,  const ACE_INET_Addr &i_remote )
  {
    m_pid = i_pid;
    m_local = i_local;
    m_remote = i_remote;
    m_uload = 0;
    m_dload = 0;
	m_delta_uload = 0;
	m_delta_dload = 0;
	m_lastUpdate = ACE_OS::time(0);
  }

  const ACE_INET_Addr& getLocalAddr()  const { return m_local; }
  const ACE_INET_Addr& getRemoteAddr() const { return m_remote; }
  ACE_UINT32           getOwnerID() const { return m_pid;}
  
private:
  ACE_INET_Addr m_local;
  ACE_INET_Addr m_remote;
  ACE_UINT32    m_pid;

public:
  mutable ACE_UINT64 m_uload;
  mutable ACE_UINT64 m_dload;
  mutable time_t m_lastUpdate;
  mutable ACE_UINT64 m_delta_uload;
  mutable ACE_UINT64 m_delta_dload;
};
//////////////////////////////////////////////////////////////////////////////

struct local{};
struct local_remote{};
struct remote_local{};

/////////////////////////////////////////////////////////////////////////////
typedef boost::multi_index::composite_key
<
ConnInfo,
BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                ConnInfo,
								const ACE_INET_Addr&,
                                getLocalAddr)
> ConnInfoKey_Local;
/////////////////////////////////////////////////////////////////////////////
typedef boost::multi_index::composite_key
<
ConnInfo,
BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                ConnInfo,
                                const ACE_INET_Addr&,
                                getLocalAddr),
BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                ConnInfo,
                                const ACE_INET_Addr&,
                                getRemoteAddr)
> ConnInfoKey_LocalRemote;
/////////////////////////////////////////////////////////////////////////////
typedef boost::multi_index::composite_key
<
ConnInfo,
BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                ConnInfo,
                                const ACE_INET_Addr&,
                                getRemoteAddr),
BOOST_MULTI_INDEX_CONST_MEM_FUN(
                                ConnInfo,
                                const ACE_INET_Addr&,
                                getLocalAddr)
> ConnInfoKey_RemoteLocal;

/////////////////////////////////////////////////////////////////////////////
typedef boost::multi_index::multi_index_container
<
  ConnInfo,
  boost::multi_index::indexed_by
  <
    boost::multi_index::ordered_unique
    < 
      boost::multi_index::tag<local_remote>, ConnInfoKey_LocalRemote
    >,
    boost::multi_index::ordered_unique
    < 
      boost::multi_index::tag<remote_local>, ConnInfoKey_RemoteLocal
    >,
    boost::multi_index::ordered_non_unique
    < 
      boost::multi_index::tag<local>, ConnInfoKey_Local
    >
  >
> ConnInfoContainer;
#endif // __ConnectionInfo_H__