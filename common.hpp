#ifndef CL_INTERCEPTOR_COMMON_HPP
#define CL_INTERCEPTOR_COMMON_HPP

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

typedef boost::interprocess::managed_shared_memory::segment_manager segment_manager_t;
typedef boost::interprocess::allocator<void, segment_manager_t> void_allocator;

typedef boost::interprocess::allocator<char, segment_manager_t> char_allocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, char_allocator> string;

typedef boost::interprocess::allocator<string, segment_manager_t> string_allocator;
typedef boost::interprocess::vector<string, string_allocator> string_vector;

struct log_entry {
	string cwd;
	string_vector args;

	log_entry(const void_allocator &alloc) : cwd(alloc), args(alloc) { }
};

typedef boost::interprocess::allocator<log_entry, segment_manager_t> log_entry_allocator;
typedef boost::interprocess::deque<log_entry, log_entry_allocator> log_entry_deque;

struct shared_state {
	static constexpr auto name = "SharedState";

	boost::interprocess::interprocess_mutex mutex;
	boost::interprocess::interprocess_condition cond;
	log_entry_deque queue;

	shared_state(const void_allocator &alloc) : queue(alloc) { }
};

constexpr auto shm_segment_name = "ClIntercetorSharedMem";

#endif // CL_INTERCEPTOR_COMMON_HP
