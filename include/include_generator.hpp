#include <boost/dll/alias.hpp>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/cat.hpp>


// default values, should be overridden by cmake
#ifndef IRODS_CLI_SEQ
#   define IRODS_CLI_SEQ (ls)(put)
#endif

#define IRODS_STATIC_CLI(r,d,t) \
  namespace irods::cli { \
  class t; \
  } \
  extern irods::cli::t BOOST_PP_CAT(cli_impl_, t); \
  BOOST_DLL_ALIAS_SECTIONED(BOOST_PP_CAT(cli_impl_, t), BOOST_PP_CAT(irods_cli_, t), irodscli);

BOOST_PP_SEQ_FOR_EACH(IRODS_STATIC_CLI, ~, IRODS_CLI_SEQ);

#undef IRODS_STATIC_CLI
