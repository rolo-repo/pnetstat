#include "GTI_GlobalConfigParams.h"
#include "GTI_Logger.h"
#include "ace/Configuration_Import_Export.h"
/*
#include "boost/regex.hpp"
#include "boost/algorithm/string/replace.hpp"
*/

#include <iostream>
#include <string>

ACE_TCHAR * GTI_GlobalConfigParams::spPropsFileName = ACE_TEXT ("c:\\Private\\gti_server\\product\\bin\\gtis_config.ini");

bool GTI_GlobalConfigParams::initialize()
{
  ACE_INT32 rc = 0;

  if ( ( rc = m_conf.open ( ) ) != 0 )
  {
    std::cerr << "ACE_Configuration_Heap::open returned " << rc << std::endl;
    return false;
  }

  //TO DO check file existance ???
  
  ACE_Ini_ImpExp import ( m_conf );

  if ( (rc = import.import_config ( spPropsFileName ) ) != 0 ) 
  {
    std::cerr << "ACE_Ini_ImpExp::import_config returned " << rc << " file " << spPropsFileName << std::endl;

    return false;
  }

  m_isInitialized = true;

  return true;
}

/////////////////////////////////////////////////////////////
void GTI_GlobalConfigParams::finalize()
{
  m_isInitialized = false;
  
}


////////////////////////////////////////////////////////////

bool GTI_GlobalConfigParams::getParameterValue( const GTI_String & i_section , const GTI_String &i_paramName , GTI_String & o_value ,  const GTI_String & i_defaultValue)
{
  ACE_Configuration_Section_Key section;

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, false);

  const ACE_Configuration_Section_Key& root = m_conf.root_section ();

  if ( 0 != m_conf.open_section( root , i_section.c_str() , 0  , section ))
  {
    o_value = i_defaultValue;
    return false;
  }

  ACE_TString value;

  if ( 0 != m_conf.get_string_value( section , i_paramName.c_str() , value ) )
  {
    o_value = i_defaultValue;
    return false;
  }

  if (! convertEnvVars( value ) ) 
  {
    o_value = i_defaultValue;
    return false;
  }

  o_value.assign( value.c_str() );

  return true;
}

////////////////////////////////////////////////////////////////////////////
bool GTI_GlobalConfigParams::getParameterValue( const GTI_String & i_section , const GTI_String &i_paramName , bool & o_value  , const bool & i_defaultValue)
{
  o_value = false;

  ACE_Configuration_Section_Key section;
  
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, m_mutex, false);

  const ACE_Configuration_Section_Key& root = m_conf.root_section ();

  if ( 0 != m_conf.open_section(root , i_section.c_str() , 0  , section ))
  {
    o_value = i_defaultValue;
    return false;
  }

  ACE_TString value;

  if ( 0 != m_conf.get_string_value( section , i_paramName.c_str() , value ) )
  {
    o_value = i_defaultValue;
    return false;
  }

  if (! convertEnvVars( value ) ) 
  {
    o_value = i_defaultValue;
    return false;
  }

  if ( value == 'Y' || value == 'y' || value.compare("TRUE") == 0 || value.compare("true") == 0)
    o_value = true;

  return true;
}


bool GTI_GlobalConfigParams::convertEnvVars( ACE_TString &io_value )
{
  /*
  std::string xStr( io_value.c_str() );

  boost::regex xRegEx("\\$\\{*(\\w+)\\}*");

  //stores the result [0] - entire match [1] first (*) [2] second (*)
  boost::smatch xResults;

  std::string::const_iterator xItStart = xStr.begin();

  std::string::const_iterator xItEnd = xStr.end();

  while( boost::regex_search( xItStart, xItEnd, xResults, xRegEx , boost::match_default | boost::format_perl) )
  {
    std::string match( xResults[1].first,xResults[1].second);
    std::string entire_match( xResults[0].first,xResults[0].second);
   
    ACE_TCHAR *val = ACE_OS::getenv( match.c_str() );

    if ( val )
    {
      boost::replace_all ( xStr, entire_match , std::string( val ) );
    }
    else
    {
      std::cerr  << "Error >>> " << BOOST_CURRENT_FUNCTION << " env variable " << entire_match << " not defined" << std::endl;

      return false;
    }

    //string was changed retake the iterators
    xItStart = xStr.begin();
    xItEnd = xStr.end();
  }

  io_value = xStr.c_str() ;
 */
  return true;
}


/*
boost::regex EXPR( "[0-9][0-9][A-Za-z]" ) ;

cout << "The expression is:" << endl ;
cout << EXPR << endl << endl ;

cout << "Enter a string to match it, or just type Q to move on" << endl ;
getline( cin, input ) ;

bool matches = boost::regex_match( input, EXPR ) ;
if( matches )
{
puts( "Congrats, you entered a string that matches the expression." ) ;
}

boost::sregex_iterator xIt ( io_value.begin(), io_value.end(), xRegEx);
//boost::regex_replace( s , e, reg, boost::match_default | boost::format_perl);
boost::sregex_iterator xInvalidIt;

boost::regex_format()
xIt->swap()
while( xIt != xInvalidIt )

std::cout << *xIt++ << "*";

*/