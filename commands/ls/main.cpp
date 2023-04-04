#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>

#include <boost/config.hpp>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/range/detail/any_iterator.hpp>

#include <fmt/format.h>

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <functional>

#define CLI_COMMAND_NAME ls

namespace fs = irods::experimental::filesystem;
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
            return "List the contents of a collection.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description options{""};
            options.add_options()
                ("acls,A", "")
                ("l,l", "")
                ("L,L", "")
                ("r,r", "")
                ("t,t", po::value<std::string>(), "")
                ("bundle", "")
                ("logical_path", po::value<std::string>(), "");

            po::positional_options_description positional_options;
            positional_options.add("logical_path", 1);

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(options).positional(positional_options).run(), vm);
            po::notify(vm);

            rodsEnv env;

            if (getRodsEnv(&env) < 0) {
                std::cerr << "Error: Could not get iRODS environment.\n";
                return 1;
            }

            const auto logical_path = vm.count("logical_path") ? vm["logical_path"].as<std::string>() : env.rodsCwd;
            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();

            const auto s = fs::client::status(conn, logical_path);
            if(!fs::client::is_collection(s) && !fs::client::is_data_object(s)) {
                std::cerr << "Error: Logical path does not point to a collection or data object.\n";
                return 1;
            }

            auto const printfunc = vm.count("L") ? std::bind(&CLI_COMMAND_NAME::print_multi_line_description, this, std::placeholders::_1, std::placeholders::_2)
                                                 : std::bind(&CLI_COMMAND_NAME::print_one_liner_description, this, std::placeholders::_1, std::placeholders::_2);

            if(vm.count("r")) {
                for(auto&& e : fs::client::recursive_collection_iterator{conn,logical_path}) {
                    std::invoke(printfunc, conn, e);
                }
            }
            else {
                for(auto&& e : fs::client::collection_iterator{conn,logical_path}) {
                    std::invoke(printfunc, conn, e);
                }
            }

            return 0;
        }

    private:
        auto print_one_liner_description(rcComm_t& conn, const fs::client::collection_entry& e) -> void
        {
            auto tm = std::chrono::system_clock::to_time_t(e.last_write_time());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&tm), "%F %T");

            fmt::print("{:<10} {} {:<10} {:>15} {} & {}\n",
                       e.owner(),
                       0,
                       "demoResc",
                       fs::client::data_object_size(conn, e),
                       ss.str(),
                       e.path().object_name().c_str());
        }

        auto print_multi_line_description(rcComm_t& conn, const fs::client::collection_entry& e) -> void
        {
            auto tm = std::chrono::system_clock::to_time_t(e.last_write_time());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&tm), "%F %T");
            fmt::print("MULTILINE TODO");
            fmt::print("{:<10} {} {:<10} {:>15} {} & {}\n",
                       e.owner(),
                       0,
                       "demoResc",
                       fs::client::data_object_size(conn, e),
                       ss.str(),
                       e.path().object_name().c_str());

        }
    }; // class ls
} // namespace irods::cli

// TODO Need to investigate whether this is truely required.
#ifdef DO_STATIC 
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
#else
irods::cli::CLI_COMMAND_NAME cli_impl;
#endif
#undef CLI_COMMAND_NAME
