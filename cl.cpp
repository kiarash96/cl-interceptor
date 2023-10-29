#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "common.hpp"

constexpr auto original_compiler = "cl_origin.exe";

void call_compiler(const std::vector<std::string> &arguments) {
	using std::filesystem::path;

	std::stringstream ss;
	ss << boost::replace_all_copy((path(arguments[0].c_str()).parent_path() / path(original_compiler)).string(), " ", "\" \"");
	for (int i = 1; i < arguments.size(); ++i)
		ss << " " << boost::replace_all_copy(arguments[i], " ", "\" \"");

	std::system(ss.str().c_str());
}

int main(int argc, char **argv)
try {
	using namespace boost::interprocess;

	managed_shared_memory segment(open_only, shm_segment_name);
	void_allocator alloc_inst(segment.get_segment_manager());

	log_entry entry(alloc_inst);
	entry.cwd = std::filesystem::current_path().string().c_str();
	for (int i = 0; i < argc; ++i)
		entry.args.emplace_back(argv[i], alloc_inst);

	shared_state *state = segment.find<shared_state>(shared_state::name).first;
	{
		scoped_lock<interprocess_mutex> lock(state->mutex);
		state->queue.push_back(entry);
		state->cond.notify_one();
	}

	call_compiler({entry.args.begin(), entry.args.end()});

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