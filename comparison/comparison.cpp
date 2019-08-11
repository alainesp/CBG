///////////////////////////////////////////////////////////////////////////////
// Compare various hash tables against Cuckoo Breeding Ground (CBG) hashtable
///////////////////////////////////////////////////////////////////////////////
//
// Written by Alain Espinosa <alainesp at gmail.com> in 2019 and placed
// under the MIT license (see LICENSE file for a full definition).
//
///////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <thread>
// Hash tables and sets
#include "../cbg.hpp"// ours
// Others
#include <unordered_set>
#include "flat_hash_map/bytell_hash_map.hpp"
#include "abseil-cpp/absl/container/flat_hash_set.h"
#include "abseil-cpp/absl/container/flat_hash_map.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Hash tables to compare
////////////////////////////////////////////////////////////////////////////////////////////////
using stduno = std::unordered_set<uint64_t, cbg::hashing::wyhash<uint64_t>>;
using bytell = ska::bytell_hash_set<uint64_t, cbg::hashing::wyhash<uint64_t>>;
using abseil = absl::flat_hash_set<uint64_t, cbg::hashing::wyhash<uint64_t>>;

template<size_t T> using SoA = cbg::Set_SoA<T, uint64_t>;
template<size_t T> using AoS = cbg::Set_AoS<T, uint64_t>;
template<size_t T> using AoB = cbg::Set_AoB<T, uint64_t>;

////////////////////////////////////////////////////////////////////////////////////////////////
// Params
////////////////////////////////////////////////////////////////////////////////////////////////
static size_t repeats[] = { 100'000, 10'000, 10'000, 1'000, 100, 10, 10, 3 };
static constexpr size_t BEGIN_SIZE = 64;
static constexpr size_t MAX_TIME_INDEX = 7;

// Names of the hash tables used in the benchmark
static std::vector<std::string> ht_names;
static const char* add_ht_name(const char* ht_name)
{
	std::string s(ht_name);
	size_t less = s.find('<');
	size_t greater = s.find('>');

	// Because Matlab don't like <> characters
	if (less != std::string::npos)
		s = s.replace(less, 1, "_").replace(greater, 1, "");

	ht_names.push_back(std::move(s));

	return ht_names[ht_names.size() - 1].data();
}

#define TEST_PERFORMANCE(HT,PERCENT,SUM) test_complete_performance<HT,PERCENT,SUM>(matlab_script, seed, add_ht_name(#HT))

struct NextGen {
	uint64_t seed;
	NextGen(uint64_t seed) noexcept : seed(seed) {}
	uint64_t operator()() noexcept
	{
		seed += 1;
		return	seed;
	}
};

template<class HT, size_t PERCENT, size_t SUM> void test_complete_performance(FILE* matlab_script, uint64_t seed, const char* ht_name)
{
	// Seed with a real random value, if available
	//cbg::wyrand r(seed);
	NextGen r(seed);

	// Initialize timers
	std::chrono::nanoseconds insert_times[MAX_TIME_INDEX];
	std::chrono::nanoseconds positive_times[MAX_TIME_INDEX];
	std::chrono::nanoseconds negative_times[MAX_TIME_INDEX];
	for (size_t time_index = 0; time_index < MAX_TIME_INDEX; time_index++)
	{
		insert_times[time_index] = std::chrono::nanoseconds::zero();
		positive_times[time_index] = std::chrono::nanoseconds::zero();
		negative_times[time_index] = std::chrono::nanoseconds::zero();
	}

	// Fill the table
	for (size_t new_size = BEGIN_SIZE, time_index = 0; time_index < MAX_TIME_INDEX; new_size *= 8, time_index++)
	{
		// Define Hash Table
		HT cuckoo_table(new_size+SUM);
		size_t original_capacity = cuckoo_table.bucket_count();
		cuckoo_table.max_load_factor(0.9999f);

		////////////////////////////////////////////////////////////////////////////////////////////////
		// Insert
		////////////////////////////////////////////////////////////////////////////////////////////////
		r.seed = seed;
		
		for (size_t repeat = 0; repeat < repeats[time_index]; repeat++)
		{
			cuckoo_table.clear();
			
			std::this_thread::yield();
			// Time this cycle
			auto start = std::chrono::high_resolution_clock::now();
			for (size_t i = 0; i < new_size * PERCENT / 100; i++)
				cuckoo_table.insert(r());
			// Calculate time
			auto end = std::chrono::high_resolution_clock::now();
			insert_times[time_index] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			
			// Warning
			if (cuckoo_table.bucket_count() != original_capacity)
				printf("Warning, table '%s' grew from %zu to %zu\n", ht_name, original_capacity, cuckoo_table.bucket_count());
		}

		////////////////////////////////////////////////////////////////////////////////////////////////
		// Insert again for lookup tests
		////////////////////////////////////////////////////////////////////////////////////////////////
		r.seed = seed;
		cuckoo_table.clear();
		for (size_t i = 0; i < new_size * PERCENT / 100; i++)
			cuckoo_table.insert(r());

		////////////////////////////////////////////////////////////////////////////////////////////////
		// Positive lookup
		////////////////////////////////////////////////////////////////////////////////////////////////
		std::this_thread::yield();
		// Time this cycle
		auto start = std::chrono::high_resolution_clock::now();
		for (size_t repeat = 0; repeat < repeats[time_index]; repeat++)
		{
			size_t num_found = 0;
			r.seed = seed;

			for (size_t i = 0; i < new_size * PERCENT / 100; i++)
				if (cuckoo_table.count(r()))//, cbg::Search_Hint::Expect_Positive))
					num_found++;

			// Warning
			if (num_found != new_size * PERCENT / 100)
				printf("Error in positive lookup %zu of %zu\n", num_found, new_size * PERCENT / 100);
		}
		// Calculate time
		auto end = std::chrono::high_resolution_clock::now();
		positive_times[time_index] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

		////////////////////////////////////////////////////////////////////////////////////////////////
		// Negative
		////////////////////////////////////////////////////////////////////////////////////////////////
		std::this_thread::yield();
		// Time this cycle
		start = std::chrono::high_resolution_clock::now();
		for (size_t repeat = 0; repeat < repeats[time_index]; repeat++)
		{
			size_t num_found = 0;
			
			// Time this
			for (size_t i = 0; i < new_size*PERCENT/100; i++)
				if (cuckoo_table.count(r()))//, cbg::Search_Hint::Expect_Negative))
					num_found++;
			
			// Warning
			if (num_found)
				printf("This is probably an error on negative lookup %zu of %zu\n", num_found, new_size);
		}
		// Calculate time
		end = std::chrono::high_resolution_clock::now();
		negative_times[time_index] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	}

	// Report in millions operations per second
	fprintf(matlab_script, "y_Insert_%s=[", ht_name);
	for (size_t new_size = BEGIN_SIZE, time_index = 0; time_index < MAX_TIME_INDEX; new_size *= 8, time_index++)
		fprintf(matlab_script, "%s%zu", time_index ? "," : "", 1'000/*'000'000*/ * repeats[time_index] * (new_size * PERCENT / 100) / insert_times[time_index].count());
	fprintf(matlab_script, "];\n");

	fprintf(matlab_script, "y_Positive_%s=[", ht_name);
	for (size_t new_size = BEGIN_SIZE, time_index = 0; time_index < MAX_TIME_INDEX; new_size *= 8, time_index++)
		fprintf(matlab_script, "%s%zu", time_index ? "," : "", 1'000/*'000'000*/ * repeats[time_index] * (new_size * PERCENT / 100) / positive_times[time_index].count());
	fprintf(matlab_script, "];\n");
	
	fprintf(matlab_script, "y_Negative_%s=[", ht_name);
	for (size_t new_size = BEGIN_SIZE, time_index = 0; time_index < MAX_TIME_INDEX; new_size *= 8, time_index++)
		fprintf(matlab_script, "%s%zu", time_index ? "," : "", 1'000/*'000'000*/ * repeats[time_index] * (new_size * PERCENT / 100) / negative_times[time_index].count());
	fprintf(matlab_script, "];\n");
}
int main()
{
	// Prologue
	FILE* matlab_script = fopen("ht_plot.m", "w");
	uint64_t seed = 0;
	constexpr size_t table_load = 50;
	std::vector<const char*> operation_names = {"Insert", "Positive", "Negative"};

	// Save x values
	fprintf(matlab_script, "x=[");
	for (size_t new_size = BEGIN_SIZE, time_index = 0; time_index < MAX_TIME_INDEX; new_size *= 8, time_index++)
		fprintf(matlab_script, "%s%zu", time_index ? "," : "", new_size);
	fprintf(matlab_script, "];\n");

	// Perform bechmark and save Y values
	TEST_PERFORMANCE(stduno, table_load,  0);
	TEST_PERFORMANCE(bytell, table_load,  0);
	TEST_PERFORMANCE(abseil, table_load, -1);
	// Our hash tables
	TEST_PERFORMANCE(SoA<2>, table_load,  0);
	TEST_PERFORMANCE(AoS<2>, table_load,  0);

	for (auto op_name : operation_names)
	{
		// Plot
		fprintf(matlab_script, "p=semilogx(");
		for (size_t i = 0; i < ht_names.size(); i++)
		{
			// Format lines
			const char* line_format = "";
			if (ht_names[i].find("stduno") != std::string::npos) line_format = ",':'";
			if (ht_names[i].find("bytell") != std::string::npos) line_format = ",'--'";
			if (ht_names[i].find("abseil") != std::string::npos) line_format = ",'--'";

			fprintf(matlab_script, "%sx,y_%s_%s%s", i ? "," : "", op_name, ht_names[i].data(), line_format);
		}
		fprintf(matlab_script, ");\n");

		fprintf(matlab_script,
			"title('%s at table load %zu%%');\n"
			"xlabel('Number of elements');\n"
			"ylabel('Operations / sec');\n"
			"legend(", op_name, table_load);
		// Legend
		for (size_t i = 0; i < ht_names.size(); i++)
			fprintf(matlab_script, "%s'%s'", i ? "," : "", ht_names[i].data());
		fprintf(matlab_script, ");\n");
		// Line width
		for (size_t i = 0; i < ht_names.size(); i++)
			fprintf(matlab_script, "p(%zi).LineWidth = 2;", i + 1);
		fprintf(matlab_script, "\npause;\n");
	}

	fclose(matlab_script);

	return 0;
}