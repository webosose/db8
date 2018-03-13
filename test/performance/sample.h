#pragma once

#include "suite.h"
#include <mutex>
#include <thread>

template <class Suite, class Clock>
MojErr sample(Suite& suite, MojDb* db, size_t repeates, std::vector<typename Clock::duration>* diffs)
{
	MojErr err;

	err = suite.init(db);
	MojErrCheck(err);

	typename Clock::time_point begin;
	typename Clock::time_point end;

	diffs->clear();
	diffs->reserve(repeates);

	for (size_t count = 0; count < repeates ; ++count)
	{
		begin = Clock::now();
		err = suite.run(db);		// <- no additional overhead should be here
		end = Clock::now();

		MojErrCheck(err);

		diffs->push_back(end - begin);
	}

	return MojErrNone;
}

template <class Suite, class Clock>
MojErr sample(Suite& suite, MojDb* db, size_t repeates, typename Clock::duration* diff)
{
	MojErr err;

	err = suite.init(db);
	MojErrCheck(err);

	typename Clock::time_point begin;
	typename Clock::time_point end;

	begin = Clock::now();
	for (size_t count = 0; count < repeates; ++count)
	{
		err = suite.run(db);		// <- no additional overhead should be here
		MojErrCheck(err);
	}
	end = Clock::now();

	*diff = (end - begin);

	return MojErrNone;
}

template <class Suite>
MojErr sampleThreaded(MojDb* db, size_t threads, size_t trepeats, Durations* diffs)
{
	std::mutex writeMutex;
	std::vector<std::thread> workers;

	for (size_t i = 0; i < threads; ++i) {
		workers.push_back(std::thread([&](){
			MojErr err;
			Suite suite;

			Durations threadDiffs;
			err = sample<Suite, boost::chrono::thread_clock>(suite, db, trepeats, &threadDiffs);
			MojErrCheck(err);

			writeMutex.lock();
			diffs->reserve(diffs->size() + threadDiffs.size());
			std::copy(threadDiffs.begin(), threadDiffs.end(), std::inserter(*diffs, diffs->end()));
			writeMutex.unlock();

			return MojErrNone;
		}));
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread &t) {
		t.join();
	});

	return MojErrNone;
}

template <class Suite>
MojErr sampleThreaded(MojDb* db, size_t threads, size_t trepeats, boost::chrono::thread_clock::duration* diff)
{
	std::mutex writeMutex;
	std::vector<std::thread> workers;

	for (size_t i = 0; i < threads; ++i) {
		workers.push_back(std::thread([&](){
			MojErr err;
			Suite suite;

			boost::chrono::thread_clock::duration threadDiff;
			err = sample<Suite, boost::chrono::thread_clock>(suite, db, trepeats, &threadDiff);
			MojErrCheck(err);

			writeMutex.lock();
			*diff += threadDiff;
			writeMutex.unlock();

			return MojErrNone;
		}));
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread &t) {
		t.join();
	});

	return MojErrNone;
}

template <class Suite>
MojErr sampleThreadedAccumulate(MojDb* db, size_t threads, size_t trepeats, Durations* diffs) {
	MojErr err;

	boost::chrono::thread_clock::duration diff;
	err = sampleThreaded<Suite>(db, threads, trepeats, &diff);
	MojErrCheck(err);
	diffs->push_back(diff);

	return MojErrNone;;
}
