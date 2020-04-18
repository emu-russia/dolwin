#include "pch.h"

namespace Debug
{
	void SamplingProfiler::ThreadProc(void* Parameter)
	{
		SamplingProfiler* profiler = (SamplingProfiler *)Parameter;

		while (true)
		{
			uint64_t ticks = Gekko::Gekko->GetTicks();
			if (ticks >= profiler->savedGekkoTbr)
			{
				profiler->savedGekkoTbr = ticks + profiler->pollingInterval;

				profiler->jsonLock->Lock();
				profiler->sampleData->AddUInt64(nullptr, ticks);
				profiler->sampleData->AddUInt32(nullptr, PC);
				profiler->jsonLock->Unlock();
			}
		}
	}

	SamplingProfiler::SamplingProfiler(const char* jsonFileName, int periodMs)
	{
		strcpy_s(filename, sizeof(filename) - 1, jsonFileName);

		jsonLock = new SpinLock;
		assert(jsonLock);

		pollingInterval = periodMs * Gekko::Gekko->OneMillisecond();
		savedGekkoTbr = Gekko::Gekko->GetTicks() + pollingInterval;

		json = new Json();
		assert(json);

		rootObj = json->root.AddObject(nullptr);
		assert(rootObj);

		sampleData = rootObj->AddArray("sampleData");
		assert(sampleData);

		thread = new Thread(ThreadProc, false, this, "SamplingProfiler");
		assert(thread);
	}

	SamplingProfiler::~SamplingProfiler()
	{
		delete jsonLock;
		delete thread;

		size_t textSize = 0;

		json->GetSerializedTextSize(nullptr, -1, textSize);
		
		uint8_t* jsonText = new uint8_t[2 *textSize];
		assert(jsonText);

		json->Serialize(jsonText, 2 * textSize, textSize);

		UI::FileSave(filename, jsonText, textSize);

		delete [] jsonText;
		delete json;
	}

}
