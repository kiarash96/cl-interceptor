#include <iostream>
#include <fstream>
#include <string>
#include <numeric>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "common.hpp"

using namespace std::chrono_literals;

std::string escape_string(std::string input) {
    boost::replace_all(input, "\\", "\\\\");
    return input;
}

void log_to_file(std::ofstream &output, const log_entry &entry) {
    std::string cwd = escape_string(std::string(entry.cwd));

    output << "{" << std::endl;
    output << "\t\"directory\": \"" << cwd << "\"," << std::endl;

    std::string file;
    output << "\t\"arguments\": [";
    for (int i = 0; i < entry.args.size(); ++i) {
        std::string arg = escape_string(std::string(entry.args[i]));
        output << (i == 0 ? "" : ", ") << "\"" << arg << "\"";
        if (arg.ends_with(".c"))
            file = arg;
    }
    output << "]," << std::endl;

    output << "\t\"file\": \"" << file << "\"" << std::endl;
    output << "}," << std::endl;
}

int main(int argc, char **argv)
    try {
        namespace po = boost::program_options;

        po::options_description generic_options("generic options");
        generic_options.add_options()
            ("help", "view help message")
            ("output,o", po::value<std::string>()->default_value("compile_commands.json"),
             "path to output file")
            ;

        po::options_description hidden_options("hidden options");
        hidden_options.add_options()
            ("command", po::value<std::vector<std::string>>(), "build command")
            ;

        po::options_description cmdline_options;
        cmdline_options.add(generic_options).add(hidden_options);

        po::positional_options_description p;
        p.add("command", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv)
                .options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: intercept [options] -- [command]\n" << std::endl;
            std::cout << generic_options << std::endl;
            return 0;
        }

        auto output_path = vm["output"].as<std::string>();
        std::cout << "Output path is " << output_path << std::endl;

        std::ofstream output;
        output.open(output_path, std::ofstream::out);
        output << "[" << std::endl;

        using namespace boost::interprocess;

        struct shm_remove {
            shm_remove() { shared_memory_object::remove(shm_segment_name); }
            ~shm_remove() { shared_memory_object::remove(shm_segment_name); }
        } remover;

        managed_shared_memory segment(create_only, shm_segment_name, 65536);
        void_allocator alloc_inst(segment.get_segment_manager());

        shared_state *state = segment.construct<shared_state>(shared_state::name)(alloc_inst);
		
		namespace bp = boost::process;
        auto command = vm["command"].as<std::vector<std::string>>();
        bp::child c(bp::search_path("cmd.exe"), "/c", bp::args(command));
		
        while (c.running()) {
            scoped_lock<interprocess_mutex> lock(state->mutex);
            state->cond.wait(lock);

            while (!state->queue.empty()) {
                log_to_file(output, state->queue.front());
                state->queue.pop_front();
            }
        }

        c.wait();

        output << "]" << std::endl;

        return 0;
    }
catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
catch (...) {
    std::cerr << "Unknown exception" << std::endl;
    return 1;
}

// vim: ts=4 sw=4 sts=4 et
