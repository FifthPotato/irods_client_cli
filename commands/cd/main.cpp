#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/rodsPath.h>
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

#define CLI_COMMAND_NAME cd 

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
            return "Changes the irods current working directory.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description options{""};
            options.add_options()
                ("logical_path", po::value<std::string>(), "logical path to collection to change directory to");

            po::positional_options_description pod;
            pod.add("logical_path", 1);

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(options).positional(pod).run(), vm);
            po::notify(vm);

            const auto envFile = std::string(getRodsEnvFileName());

            if (vm.count("logical_path") == 0) {
                boost::filesystem::path p{envFile};
                if(boost::filesystem::exists(p)) {
                    boost::filesystem::remove(p);
                    return 0;
                }
            }

            if (vm.count("logical_path") > 1) {
                std::cerr << "Error: Too many arguments..\n";
                return 1;
            }
            rodsEnv env;

            if (getRodsEnv(&env) < 0) {
                std::cerr << "Error: Could not get iRODS environment.\n";
                return 1;
            }
            const auto path = canonical(vm["logical_path"].as<std::string>(), env);

            if(!path.has_value()) {
                std::cerr << "Invalid environment path.\n";
                return 1;
            }

            auto conn_pool = irods::make_connection_pool();
            auto conn = conn_pool->get_connection();
            if(!fs::client::exists(conn, path.value())) {
                std::cerr << "Error: Requested path is nonexistent.\n";
                return 1;
            }
            if(!fs::client::is_collection(conn, path.value())) {
                std::cerr << "Error: Requested path is not a collection.\n";
                return 1;
            }

            std::ofstream outputEnvFile;
            outputEnvFile.open(envFile, std::ios::out | std::ios::trunc);
            outputEnvFile << "{\n\t\"irods_cwd\": \"" << path.value() << "\"\n}\n";
            outputEnvFile.close();
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
