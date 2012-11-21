#ifndef __GTI_GlobalConfigParams_H__
#define __GTI_GlobalConfigParams_H__

#include "GTI_Exports.h"

#include "ace/Singleton.h"
#include "ace/Synch.h"
#include "ace/Configuration.h"
#include "ace/SString.h"

#include <stdio.h>
#include <iostream>

#include "boost/lexical_cast.hpp"

#include "GTI_Constants.h"

class GTI_EXPORT GTI_GlobalConfigParams : public boost::noncopyable
{
	friend class ACE_Singleton<GTI_GlobalConfigParams,ACE_Thread_Mutex>;
public :
  typedef ACE_Singleton<GTI_GlobalConfigParams,ACE_Thread_Mutex> SingeltonImpl;
  
  
  static GTI_GlobalConfigParams* instance( )
  {
    GTI_GlobalConfigParams * p_gcp = SingeltonImpl::instance();

    if ( false ==  p_gcp->m_isInitialized )
    {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, write_guard, p_gcp->m_mutex , 0);      
      
      if ( false == p_gcp->m_isInitialized &&  false == p_gcp->initialize( ) )
         return 0;
    }
  
    return p_gcp;
  }

  void finalize();

  bool getParameterValue( const GTI_String & i_section , const GTI_String &i_paramName , GTI_String & o_value , const GTI_String & i_defaultValue = "");
  bool getParameterValue( const GTI_String & i_section , const GTI_String &i_paramName , bool       & o_value , const bool & i_defaultValue = false);

  template < typename T >
    bool getParameterValue( const GTI_String & i_section , const GTI_String &i_paramName , T & o_value , const T & i_defaultValue = 0)
    {
      bool rc = true;
      ACE_Configuration_Section_Key section;

      ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, false);

      const ACE_Configuration_Section_Key& root = m_conf.root_section ();

      if ( 0 != m_conf.open_section(root , i_section.c_str() , 0  , section ))
      {
        o_value = i_defaultValue;
        return false;
      }

       ACE_TString value;
      
      if ( 0 != m_conf.get_string_value( section , i_paramName.c_str() , value ))
      {
        o_value = i_defaultValue;
        return false;
      }

      if (! convertEnvVars( value ) ) 
      {
        o_value = i_defaultValue;
        return false;
      }
      
      try
      {
        o_value = boost::lexical_cast< T >( value.c_str() );
        rc = true;
      }
      catch (boost::bad_lexical_cast*)
      {
        std::cerr << BOOST_CURRENT_FUNCTION << " Bad casting for " << i_section.c_str() << "/" << i_paramName.c_str() << std::endl;
        rc = false;
      }  

      return rc;
    }

 static ACE_TCHAR  *spPropsFileName;

private:

  bool initialize();

  GTI_GlobalConfigParams() : m_isInitialized(false) {}

  bool convertEnvVars( ACE_TString &io_value );

private:
  //All properties are saved as stings and should be casted to proper value
  ACE_Configuration_Heap  m_conf;
  bool                    m_isInitialized;
  ACE_Thread_Mutex        m_mutex;

};
#endif // __GTI_GlobalConfigParams_H__