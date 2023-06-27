#define MAKE_IRODS_ERROR_MAP
#include "irods/rodsErrorTable.h"
const static std::map<const int, const std::string> irods_error_map = irods_error_map_construction::irods_error_map;

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
#include <boost/lexical_cast.hpp>

#define CLI_COMMAND_NAME error 

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
            return "Converts an irods error code to text.";
        }

        auto help_text() const noexcept -> std::string_view override
        {
            return "The help text.";
        }
       

        static auto number_parser(const std::string& str) -> std::pair<std::string, std::string>
        {
            try {
                int numeric_value = boost::lexical_cast<int>(str);
                return make_pair(std::string("errorNumber"), std::to_string(numeric_value));
            } catch(...) {}

            return make_pair(std::string(), std::string());
        }
        auto execute(const std::vector<std::string>& args) -> int override
        {
            po::options_description options{""};
            options.add_options()
                ("errorNumber", po::value<int>(), "The error number");

            po::positional_options_description pod;
            pod.add("errorNumber", -1);

            po::variables_map vm;

            po::store(po::command_line_parser(args).options(options).positional(pod).extra_parser(number_parser).run(), vm);
            po::notify(vm);

            if(vm.count("errorNumber") != 1) {
                std::cerr << "No error number provided.\n";
                return 1;
            }

            auto error_number = vm["errorNumber"].as<int>();
            
            if(error_number > 0) {
                error_number = -error_number;
            }

            int sub_error_number = error_number % 1000;
            int primary_error_number = error_number / 1000;

            auto error_map_iterator_search = irods_error_map.find(primary_error_number * 1000);
            if(error_map_iterator_search == irods_error_map.end()) {
                std::cerr << "Non-existent error code.\n";
                return 1;
            }

            std::cout << "irods error " << error_number << " " << error_map_iterator_search->second << "\n";

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
