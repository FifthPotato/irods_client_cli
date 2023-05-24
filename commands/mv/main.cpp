#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/rodsPath.h>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>
#include <irods/irods_exception.hpp>
#include <irods/experimental_plugin_framework.hpp>

#include <boost/program_options.hpp>
#include <boost/dll.hpp>
#include <boost/progress.hpp>


#include <iostream>
#include <string>
#include <optional>

#define CLI_COMMAND_NAME mv 

namespace fs = irods::experimental::filesystem;
namespace po = boost::program_options;
namespace ia = irods::experimental::api;

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

    class CLI_COMMAND_NAME : public command
    {
    public:
        auto name() const noexcept -> std::string_view override
        {
            return BOOST_PP_STRINGIZE(CLI_COMMAND_NAME);
        }

        auto description() const noexcept -> std::string_view override
        {
            return "Moves collections or data objects";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            auto help =R"(
Given a fully qualified or relative path, move the collection or object

irods mv [options] source_fully_qualified_logical_path destination_fully_qualified_logical_path

      --number_of_threads : number of threads to use in recursive operations
      --progress          : request progress as a percentage)";
            return help;

        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description desc{""};
            desc.add_options()
                ("logical_path", po::value<std::string>(), "logical path to collection or object to move")
                ("destination", po::value<std::string>(), "destination logical path for the move");

            po::positional_options_description pod;
            pod.add("logical_path", 1);
            pod.add("destination",  2);

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(desc).positional(pod).run(), vm);
            po::notify(vm);

            if (vm.count("logical_path") != 1) {
                std::cerr << "Error: command expects exactly one (1) source logical path.\n";
                return 1;
            }

            if (vm.count("destination") != 1) {
                std::cerr << "Error: command expects exactly one (1) destination logical path.\n";
                return 1;
            }

            rodsEnv env;

            if (getRodsEnv(&env) < 0) {
                std::cerr << "Error: Could not get iRODS environment.\n";
                return 1;
            }

            irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};
            auto conn = conn_pool.get_connection();

            const auto logical_path = canonical(vm["logical_path"].as<std::string>(), env);
            const auto destination = canonical(vm["destination"].as<std::string>(), env);

            if(!logical_path.has_value()) {
                std::cerr << "Invalid source logical path.\n";
                return 1;
            }

            if(!destination.has_value()) {
                std::cerr << "Invalid destination logical path.\n";
                return 1;
            }

            const auto object_status = fs::client::status(conn, logical_path.value());

            if (!fs::client::is_collection(object_status) && !fs::client::is_data_object(object_status)) {
                std::cerr << "Error: Logical path does not point to a collection or data object. Do you need a fully qualified path?\n";
                return 1;
            }
            fs::client::rename(conn, logical_path.value(), destination.value());
            return 0;
        }

    }; // class cp

} // namespace irods::cli

#ifdef DO_STATIC 
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
#else
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME cli_impl;
irods::cli::CLI_COMMAND_NAME cli_impl;
#endif
#undef CLI_COMMAND_NAME
