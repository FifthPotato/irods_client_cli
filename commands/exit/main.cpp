#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/obf.h>
#include <irods/irods_service_account.hpp>
#include <irods/irods_exception.hpp>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>

#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/range/detail/any_iterator.hpp>

#include <fmt/format.h>

#include <string>
#include <iostream>
#include <fstream>
#include <optional>

#define CLI_COMMAND_NAME exit 

namespace fs = irods::experimental::filesystem;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;

namespace irods::cli
{
    class CLI_COMMAND_NAME: public command
    {
    public:
        auto name() const noexcept -> std::string_view override
        {
            return BOOST_PP_STRINGIZE(CLI_COMMAND_NAME);
        }

        auto description() const noexcept -> std::string_view override
        {
            return "Exits current irods session.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description options{""};
            options.add_options()
                ("force,f", "force deletion of auth file - WILL NOT PROMPT");

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(options).run(), vm);
            po::notify(vm);

            const auto envFile = std::string(getRodsEnvFileName());

            bool isServiceAccount = false;
            bool isServiceAccountError = false;
            try {
                isServiceAccount = irods::is_service_account();
            }
            catch (const irods::exception &e) {
                isServiceAccountError = true;
                isServiceAccount = true;
                std::fprintf(stderr,
                             "WARNING: Could not determine whether the current session is running as service account "
                             "due to thrown exception: %s\n",
                             e.client_display_what());
            }
            const auto path = bfs::path{envFile};

            if(bfs::exists(path)) {
                bfs::remove(path);
            }

            if(vm.count("force")) {
                obfRmPw(1);
                return 0;
            }

            if(isServiceAccountError) {
                std::fprintf(stderr, "WARNING: Cannot determine if the current session is running as service account, "
                    "Skipping auth file deletion (pass -f to force).\n");
                return 1;
            }
            else if(isServiceAccount) {
                std::printf("WARNING: current session appears to be running as a service account. "
                    "Skipping auth file deletion (pass -f to force).\n");
                return 1;
            }
            obfRmPw(0);
            return 0;
        }

    }; // class ls
} // namespace irods::cli

#ifdef DO_STATIC 
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
#else
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME cli_impl;
irods::cli::CLI_COMMAND_NAME cli_impl;
#endif
#undef CLI_COMMAND_NAME
