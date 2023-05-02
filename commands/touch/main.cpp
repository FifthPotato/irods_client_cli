#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/rodsPath.h>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>

#include <boost/program_options.hpp>
#include <boost/dll.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <optional>

#define CLI_COMMAND_NAME touch

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

    class CLI_COMMAND_NAME : public command
    {
    public:
        auto name() const noexcept -> std::string_view override
        {
            return BOOST_PP_STRINGIZE(CLI_COMMAND_NAME);
        }

        auto description() const noexcept -> std::string_view override
        {
            return "Updates timestamps for data objects and collections.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            using rep_type = fs::object_time_type::duration::rep;

            po::options_description desc{""};
            desc.add_options()
                ("logical_path", po::value<std::string>(), "")
                ("modification_time", po::value<rep_type>(), "");

            po::positional_options_description pod;
            pod.add("logical_path", 1);
            pod.add("modification_time", 1);

            po::variables_map vm;
            po::store(po::command_line_parser(args).options(desc).positional(pod).run(), vm);
            po::notify(vm);

            if (vm.count("logical_path") == 0) {
                std::cerr << "Error: Missing logical path.\n";
                return 1;
            }

            rodsEnv env;

            if (getRodsEnv(&env) < 0) {
                std::cerr << "Error: Could not get iRODS environment.\n";
                return 1;
            }

            const auto logical_path = canonical(vm["logical_path"].as<std::string>(), env);

            if(!logical_path.has_value()) {
                std::cerr << "Invalid logical path.\n";
                return 1;
            }

            irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};
            auto conn = conn_pool.get_connection();

            {
                const auto object_status = fs::client::status(conn, logical_path.value());

                if (!fs::client::is_collection(object_status) && !fs::client::is_data_object(object_status)) {
                    std::cerr << "Error: Logical path does not point to a collection or data object.\n";
                    return 1;
                }
            }

            // clang-format off
            using clock_type    = fs::object_time_type::clock;
            using duration_type = fs::object_time_type::duration;
            // clang-format on

            fs::object_time_type new_mtime;

            if (vm.count("modification_time") > 0) {
                new_mtime = fs::object_time_type{duration_type{vm["modification_time"].as<rep_type>()}};
            }
            else {
                new_mtime = std::chrono::time_point_cast<duration_type>(clock_type::now());
            }

            try {
                fs::client::last_write_time(conn, logical_path.value(), new_mtime);
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "Error: " << e.what() << '\n';
                return 1;
            }

            return 0;
        }
    }; // class touch
} // namespace irods::cli

#ifdef DO_STATIC 
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
irods::cli::CLI_COMMAND_NAME BOOST_PP_CAT(cli_impl_, CLI_COMMAND_NAME);
#else
extern "C" BOOST_SYMBOL_EXPORT irods::cli::CLI_COMMAND_NAME cli_impl;
irods::cli::CLI_COMMAND_NAME cli_impl;
#endif
#undef CLI_COMMAND_NAME
