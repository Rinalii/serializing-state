#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

struct Args {
    std::string config_file_path;
    std::string root_path;
    std::string state_file_path;
    unsigned int save_state_period = 0;
    unsigned int tick_period = 0;
    bool random_spawn = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "Show help")
        ("tick-period,t",   po::value<unsigned int>(&args.tick_period)->value_name("milliseconds"s), "Set tick period")
        ("config-file,c",   po::value(&args.config_file_path)->value_name("file"s), "Set config file path")
        ("www-root,w",      po::value(&args.root_path)->value_name("dir"s), "Set root dir")
        ("randomize-spawn-points", po::value<bool>(&args.random_spawn), "Set random dog spawn")
        ("state-file,st",   po::value(&args.state_file_path)->value_name("state_file"s), "Set state file path")
        ("save-state-period,sv",   po::value<unsigned int>(&args.save_state_period)->value_name("milliseconds"s), "Set save state period");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file")) {
        throw std::runtime_error("Config files have not been specified"s);
        
    } 
    if (!vm.contains("www-root")) {
        throw std::runtime_error("Root dir have not been specified"s);
    } 

    return args;
}
