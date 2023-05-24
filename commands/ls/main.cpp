#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>
#include <irods/irods_query.hpp>

#include <boost/config.hpp>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/range/detail/any_iterator.hpp>

#include <fmt/format.h>

#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <iostream>
#include <ctime>
#include <sstream>
#include <functional>

#define CLI_COMMAND_NAME ls

namespace fs = irods::experimental::filesystem;
namespace po = boost::program_options;

namespace irods::cli
{
    auto getAclString(rcComm_t& conn, const fs::client::collection_entry& ce) -> std::string 
    {
        bool didUseSpecificQuery = false;
        std::stringstream ss, os;
        os << "        ACL - ";
        auto _path = ce.path();
        if(ce.is_data_object()) {
            ss << "SELECT USER_NAME, DATA_ACCESS_NAME WHERE COLL_NAME = '";
            ss << _path.parent_path().string() << "' AND DATA_NAME = '" << _path.object_name().string() << "'";
        }
        else {
            try {
                std::vector<std::string> testArgs;
                testArgs.push_back(_path.string());
                for (auto&& row : irods::query<rcComm_t>(&conn, "ShowCollAcls", &testArgs, {}, 0, 0, irods::query<rcComm_t>::SPECIFIC, 0)) {
                    std::cout << row.at(0) << "|" << row.at(1) << "|" << row.at(2);
                }
                didUseSpecificQuery = true;
            } 
            catch(const irods::exception& e) {
                std::cout << "reached exception block\n";
                ss << "SELECT COLL_USER_NAME, COLL_ACCESS_NAME WHERE COLL_NAME = '";
                ss << _path.string() << "'";
            }
        }
        if(!didUseSpecificQuery) {
            for (auto&& row : irods::query<rcComm_t>{&conn, ss.str()}) {
                os << "    " << row.at(0) << "#" << row.at(1);
            }
        }
        os << "\n";
        return os.str();
    }
    inline auto canonical(const std::string_view _path, const rodsEnv& _env) -> std::optional<std::string>
    {
        rodsPath_t input{};
        rstrcpy(input.inPath, _path.data(), MAX_NAME_LEN);

        if (parseRodsPath(&input, const_cast<rodsEnv*>(&_env)) != 0) {
            return std::nullopt;
        }

        auto* escaped_path = escape_path(input.outPath);
        std::optional<std::string> p = escaped_path;
        std::free(escaped_path);

        return p;
    }
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
            std::string logical_path;
            if(vm.count("logical_path")) {
                const auto path = canonical(vm["logical_path"].as<std::string>(), env);
                if(!path.has_value()) {
                    std::cerr << "Invalid logical path.\n";
                    return 1;
                }
                logical_path = path.value();
            }
            else {
                logical_path = env.rodsCwd;
            }
            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();

            const auto s = fs::client::status(conn, logical_path);
            if(!fs::client::is_collection(s) && !fs::client::is_data_object(s)) {
                std::cerr << "Error: Logical path does not point to a collection or data object.\n";
                return 1;
            }

            //default behavior: just print name
            auto printFuncPtr = &CLI_COMMAND_NAME::print_short_description;

            if(vm.count("L")) {
                printFuncPtr = &CLI_COMMAND_NAME::print_multi_line_description;
            }
            else if(vm.count("l")) {
                printFuncPtr = &CLI_COMMAND_NAME::print_one_liner_description;
            }
            //std::bind lame :(
            auto const printfunc = std::bind(printFuncPtr, this, std::placeholders::_1, std::placeholders::_2);
            if(vm.count("r")) {
                for(auto&& e : fs::client::recursive_collection_iterator{conn,logical_path}) {
                    std::invoke(printfunc, conn, e);
                    if(vm.count("acls")) {
                        std::cout << getAclString(conn, e);
                    }
                }
            }
            else {
                for(auto&& e : fs::client::collection_iterator{conn,logical_path}) {
                    std::invoke(printfunc, conn, e);
                    if(vm.count("acls")) {
                        std::cout << getAclString(conn, e);
                    }
                }
            }

            return 0;
        }

    private:
        auto print_short_description(rcComm_t& conn, const fs::client::collection_entry& e) -> void
        {
            std::stringstream os;
            if(!e.is_data_object()) {
                os << "C- ";
            }
            os << e.path().object_name().c_str() << "\n";
            std::cout << os.str();
        }
        auto print_one_liner_description(rcComm_t& conn, const fs::client::collection_entry& e) -> void
        {
            auto tm = std::chrono::system_clock::to_time_t(e.last_write_time());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&tm), "%F %T");
            if(e.is_data_object()) {
                fmt::print("{:<10} {} {:<10} {:>15} {} & {}\n",
                           e.owner(),
                           0,
                           "demoResc",
                           fs::client::data_object_size(conn, e),
                           ss.str(),
                           e.path().object_name().c_str());
            } else {
                fmt::print("{:<10} {} {:<10} {} & {}\n",
                           e.owner(),
                           0,
                           "demoResc",
                           ss.str(),
                           e.path().object_name().c_str());

            }
        }

        auto print_multi_line_description(rcComm_t& conn, const fs::client::collection_entry& e) -> void
        {
            auto tm = std::chrono::system_clock::to_time_t(e.last_write_time());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&tm), "%F %T");
             if(e.is_data_object()) {
                fmt::print("{:<10} {} {:<10} {:>15} {} & {}\n",
                           e.owner(),
                           0,
                           "demoResc",
                           fs::client::data_object_size(conn, e),
                           ss.str(),
                           e.path().object_name().c_str());
            } else {
                fmt::print("{:<10} {} {:<10} {} & {}\n",
                           e.owner(),
                           0,
                           "demoResc",
                           ss.str(),
                           e.path().object_name().c_str());

            }

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
