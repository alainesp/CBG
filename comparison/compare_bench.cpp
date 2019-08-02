///////////////////////////////////////////////////////////////////////////////
//Compare various hash tables against Cuckoo Breeding Ground (CBG) hashtable
///////////////////////////////////////////////////////////////////////////////
//
// Written by Alain Espinosa <alainesp at gmail.com> in 2019 and placed
// under the MIT license (see LICENSE file for a full definition).
//
///////////////////////////////////////////////////////////////////////////////

#include <benchmark/benchmark.h>
// Hash tables and sets
#include "../cbg.hpp"// ours
// Others
#include <unordered_set>
#include "flat_hash_map/bytell_hash_map.hpp"
#include "abseil-cpp/absl/container/flat_hash_set.h"
#include "abseil-cpp/absl/container/flat_hash_map.h"

// Utility
static uint64_t getRandomSeed()
{
	std::random_device good_random;
	return good_random();
}

// Functions to benchmark
template <class HT, int64_t PERCENT, int64_t sumFix = 0> void Insertion(benchmark::State& state) {
	// Perform setup here
	std::mt19937_64 r(getRandomSeed());

	// Create the hash table to benchmark
	HT hashTable(state.range(0) + sumFix);
	hashTable.max_load_factor(0.999f);

	// How much to fill the table (final table load)
	size_t fillTo = state.range(0) * PERCENT / 100;

	// Warm-up
	for (size_t i = 0; i < fillTo; i++)
		hashTable.insert(r());

	if(hashTable.bucket_count() != (state.range(0) + sumFix))
		printf("\nError\n");
	hashTable.clear();

	for (auto _ : state) {
		// This code gets timed
		for (size_t i = 0; i < fillTo; i++)
			hashTable.insert(r());

		hashTable.clear();
	}

	// Insertions per second
	state.counters["insertions"] = benchmark::Counter(static_cast<double>(fillTo), benchmark::Counter::kIsIterationInvariantRate);
}
template <class HT, int64_t PERCENT, int64_t sumFix = 0> void PositiveLookup(benchmark::State& state) {
	// Perform setup here
	uint64_t seed = getRandomSeed();
	std::mt19937_64 r(seed);

	// Create the hash table to benchmark
	HT hashTable(state.range(0) + sumFix);
	hashTable.max_load_factor(0.999f);

	// How much to fill the table (final table load)
	size_t fillTo = state.range(0) * PERCENT / 100;

	// Warm-up
	for (size_t i = 0; i < fillTo; i++)
		hashTable.insert(r());

	if (hashTable.bucket_count() != (state.range(0) + sumFix))
		printf("\nError\n");

	for (auto _ : state) {
		// This code gets timed
		r = std::mt19937_64(seed);
		size_t count = 0;

		for (size_t i = 0; i < fillTo; i++)
			if (hashTable.count(r()))
				count++;

		if(count != fillTo)
			printf("\nError\n");
	}

	// Positive lookup per second
	state.counters["positive"] = benchmark::Counter(static_cast<double>(fillTo), benchmark::Counter::kIsIterationInvariantRate);
}
template <class HT, int64_t PERCENT, int64_t sumFix = 0> void NegativeLookup(benchmark::State & state) {
	// Perform setup here
	std::mt19937_64 r(getRandomSeed());

	// Create the hash table to benchmark
	HT hashTable(state.range(0) + sumFix);
	hashTable.max_load_factor(0.999f);

	// How much to fill the table (final table load)
	size_t fillTo = state.range(0) * PERCENT / 100;

	// Warm-up
	for (size_t i = 0; i < fillTo; i++)
		hashTable.insert(r());

	if (hashTable.bucket_count() != (state.range(0) + sumFix))
		printf("\nError\n");

	for (auto _ : state) {
		// This code gets timed
		size_t count = 0;

		for (size_t i = 0; i < fillTo; i++)
			if (hashTable.count(r()))
				count++;

		if (count)
			printf("\nError\n");
	}

	// Negative lookup per second
	state.counters["negative"] = benchmark::Counter(static_cast<double>(fillTo), benchmark::Counter::kIsIterationInvariantRate);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Simplify printing
////////////////////////////////////////////////////////////////////////////////////////////////
using stduno = std::unordered_set<uint64_t, cbg::hashing::t1ha2<uint64_t>>;
using bytell = ska::bytell_hash_set<uint64_t, cbg::hashing::t1ha2<uint64_t>>;
using abseil = absl::flat_hash_set<uint64_t, cbg::hashing::t1ha2<uint64_t>>;

template<size_t T> using SoA = cbg::Set_SoA<T, uint64_t>;
template<size_t T> using AoS = cbg::Set_AoS<T, uint64_t>;
template<size_t T> using AoB = cbg::Set_AoB<T, uint64_t>;

////////////////////////////////////////////////////////////////////////////////////////////////
// Table load = 50%
////////////////////////////////////////////////////////////////////////////////////////////////
// Insertion
BENCHMARK_TEMPLATE(Insertion, stduno, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(Insertion, bytell, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(Insertion, abseil, 50, -1)->Range(64, 2 << 24/*27*/);

BENCHMARK_TEMPLATE(Insertion, SoA<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, SoA<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, SoA<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(Insertion, AoS<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, AoS<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, AoS<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(Insertion, AoB<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, AoB<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(Insertion, AoB<4>, 50)->Range(64, 2 << 24/*27*/);

// Positive lookup ========================================================
BENCHMARK_TEMPLATE(PositiveLookup, stduno, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(PositiveLookup, bytell, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(PositiveLookup, abseil, 50, -1)->Range(64, 2 << 24/*27*/);

BENCHMARK_TEMPLATE(PositiveLookup, SoA<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, SoA<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, SoA<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(PositiveLookup, AoS<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, AoS<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, AoS<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(PositiveLookup, AoB<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, AoB<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(PositiveLookup, AoB<4>, 50)->Range(64, 2 << 24/*27*/);

// Negative lookup ========================================================
BENCHMARK_TEMPLATE(NegativeLookup, stduno, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(NegativeLookup, bytell, 50)->Range(64, 2 << 24/*27*/);
BENCHMARK_TEMPLATE(NegativeLookup, abseil, 50, -1)->Range(64, 2 << 24/*27*/);

BENCHMARK_TEMPLATE(NegativeLookup, SoA<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, SoA<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, SoA<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(NegativeLookup, AoS<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, AoS<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, AoS<4>, 50)->Range(64, 2 << 24/*27*/);

//BENCHMARK_TEMPLATE(NegativeLookup, AoB<2>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, AoB<3>, 50)->Range(64, 2 << 24/*27*/);
//BENCHMARK_TEMPLATE(NegativeLookup, AoB<4>, 50)->Range(64, 2 << 24/*27*/);

////////////////////////////////////////////////////////////////////////////////////////////////
// Table load = 85%
////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////
// Table load = 99%
////////////////////////////////////////////////////////////////////////////////////////////////