#include "command.hpp"

#include <irods/rodsClient.h>
#include <irods/rodsPath.h>
#include <irods/connection_pool.hpp>
#include <irods/filesystem.hpp>

#include <boost/config.hpp>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/range/detail/any_iterator.hpp>

#include <fmt/format.h>

#include <string>
#include <iostream>
#include <optional>

#define CLI_COMMAND_NAME pwd 

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
            return "Print the irods current working directory.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }

        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description options{""};
            options.add_options();

            rodsEnv env;

            if (getRodsEnv(&env) < 0) {
                std::cerr << "Error: Could not get iRODS environment.\n";
                return 1;
            }
            const auto path = canonical(".", env);

            if(!path.has_value()) {
                std::cerr << "Invalid environment path.\n";
                return 1;
            }

            fmt::print("{}\n", path.value());

            return 0;
        }

    private:
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
