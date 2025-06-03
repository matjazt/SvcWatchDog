/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <Logger/Logger.h>

void SyncEventTest(bool autoReset)
{
    SyncEvent event(false, autoReset);

    LOGSTR() << "there should be an assertion failure in the next line (line " << (__LINE__ + 1) << ")";
    LOGASSERT(0);

    event.ResetEvent();
    LOGASSERT(event.WaitForSingleEvent(10) == false);
    event.SetEvent();
    LOGASSERT(event.WaitForSingleEvent(10));

    mutex mtx;
    int threadSignalCounter = 0;
    int threadLoopCounter = 0;

    auto endTime = SteadyTime() + 1000;
    int numThreads = 10;

    // now start a couple of threads
    for (int i = 0; i < numThreads; ++i)
    {
        thread(
            [&event, i, &endTime, &mtx, &threadSignalCounter, &threadLoopCounter]()
            {
                while (SteadyTime() < endTime)
                {
                    // wait for the event to be set
                    bool signaled = event.WaitForSingleEvent(1100);
                    lock_guard lock(mtx);
                    if (signaled)
                    {
                        threadSignalCounter++;
                    }
                    threadLoopCounter++;
                }
                LOGSTR() << "thread " << i << " finished";
            })
            .detach();
    }

    if (autoReset)
    {
        int expectedSignals = 0;
        while (SteadyTime() < endTime)
        {
            event.SetEvent();
            expectedSignals++;
            SLEEP(10);  // Sleep for 50 milliseconds to allow threads to process the event
        }
        LOGSTR() << "expected " << expectedSignals << " signals";
        LOGASSERT(threadSignalCounter == expectedSignals);
        LOGASSERT(threadLoopCounter == threadSignalCounter);
        LOGSTR() << "Auto reset TestRun completed with " << threadSignalCounter << " received signals and " << threadLoopCounter
                 << " loop iterations";

        // stop all threads
        endTime = 0;
        int i = 0;
        while (i < numThreads)
        {
            if (event.SetEvent())
            {
                i++;
            }
            else
            {
                SLEEP(1);
            }
        }
        SLEEP(100);
    }
    else
    {
        // in manual reset mode, we need to set the event only once
        event.SetEvent();
        SLEEP(100);
        // stop all threads
        endTime = 0;
        SLEEP(100);

        LOGASSERT(threadSignalCounter > 1000);
        LOGASSERT(threadLoopCounter == threadSignalCounter);

        LOGSTR() << "Manual reset TestRun completed with " << threadSignalCounter << " received signals and " << threadLoopCounter
                 << " loop iterations";
    }

    LOGSTR() << "all threads should be finished now";
}
