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

#define CLI_COMMAND_NAME tree

namespace fs = irods::experimental::filesystem;
namespace po = boost::program_options;

namespace irods::cli
{
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
            std::cout << logical_path << ":" << '\n';
            for(auto&& e = fs::client::recursive_collection_iterator{conn,logical_path}; e != end(e); ++e ) {
                std::cout << std::setw(e.depth()+1) << ' ' << std::setw(0) << e->path().object_name().c_str() << '\n';
            }

            return 0;
        }

    }; // class tree
} // namespace irods::cli

#ifdef DO_STATIC 
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
#else
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME cli_impl;
irods::cli::CLI_COMMAND_NAME cli_impl;
#endif
#undef CLI_COMMAND_NAME
