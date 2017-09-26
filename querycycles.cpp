// querycycles.cpp : Defines the entry point for the console application.
//
#include <thread>
#include <windows.h>  
#include <chrono>
#include <iostream>
#include <stdio.h>
#include "querycomp.h"


int main(int argc, char* args[])
{
    std::vector<std::wstring> processNames;
    std::vector<std::wstring> threadDescs;
    DWORD timeout = 0;

    if (argc > 1)
    {
        char** pParams = args + 1;
        int paramCount = argc - 1;
        int curIndex = 0;


        while (curIndex < (paramCount - 1))
        {
            std::string param = pParams[curIndex];

            if (param[0] == '-')
            {
                char key = param[1];

                switch (key)
                {
                case 'p':
                case 'P':
                {
                    std::string value = pParams[curIndex + 1];
                    std::wstring processName(value.begin(), value.end());

                    processNames.push_back(processName);

                    curIndex += 2;
                    continue;
                }

                case 't':
                case 'T':
                {
                    std::string value = pParams[curIndex + 1];
                    std::wstring threadDesc(value.begin(), value.end());

                    threadDescs.push_back(threadDesc);
                    curIndex += 2;
                    continue;
                }

                case 'w':
                case 'W':
                {
                    std::string value = pParams[curIndex + 1];
                    timeout = std::stoi(value);

                    curIndex += 2;
                    continue;
                }
                default:
                    curIndex++;
                }

            }
            else
                curIndex++;
        }
    }
    else
    {
        return 0;
    }
    
    PerfQuery::ThreadSearch search(processNames, threadDescs);
    search.Search();

    const std::vector<PerfQuery::Thread>& threads = search.GetSearchResult();
    if (threads.size() > 0)
    {
        PerfQuery::GetThreadCPUCycles profile(threads, timeout, false);
        profile.WaitAndProfile();
        const std::vector<ULONG64>& cycles = profile.Cycles();

        for (unsigned int i = 0; i < threads.size(); i++)
        {
            std::wcout <<threads[i].processName << ":" <<  threads[i].threadDesc << ":" << cycles[i] << std::endl;
        }
    }

    return 0;
}

