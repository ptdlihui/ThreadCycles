#include "querycomp.h"
#include <psapi.h>
#include <TlHelp32.h>
#include <thread>

namespace PerfQuery
{
    bool PrintProcessNameAndID(DWORD processID, std::wstring& name)
    {
        bool ret = false;
        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

        // Get a handle to the process.

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, processID);

        // Get the process name.

        if (NULL != hProcess)
        {
            HMODULE hMod;
            DWORD cbNeeded;

            if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
                &cbNeeded))
            {
                GetModuleBaseName(hProcess, hMod, szProcessName,
                    sizeof(szProcessName) / sizeof(TCHAR));
            }


            name = szProcessName;
            ret = true;
        }


        // Print the process name and identifier.



        // Release the handle to the process.

        CloseHandle(hProcess);

        return ret;
    }

    static ThreadDescription threadDescFunc;

    bool PrintThreadDescription(DWORD threadId, std::wstring& desc)
    {
        HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadId);
        if (NULL == hThread)
            return false;

        std::vector<wchar_t> threadDesc;
        threadDesc.resize(1024);
        wchar_t* pDesc = threadDesc.data();

        threadDescFunc.GetThreadDescription(hThread, &pDesc);

        CloseHandle(hThread);

        desc = pDesc;

        return true;
    }

    void BuildProcessnNmesTable(ProcessNamesTable& table)
    {
        DWORD aProcesses[1024], cbNeeded, cProcesses;
        unsigned int i;

        if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        {
            return;
        }


        // Calculate how many process identifiers were returned.

        cProcesses = cbNeeded / sizeof(DWORD);

        // Print the name and process identifier for each process.

        for (i = 0; i < cProcesses; i++)
        {
            if (aProcesses[i] != 0)
            {
                std::wstring name;
                if (PerfQuery::PrintProcessNameAndID(aProcesses[i], name))
                    table[aProcesses[i]] = name;
            }
        }
    }

    void ThreadSearch::Search()
    {
        ProcessNamesTable processNames;
        BuildProcessnNmesTable(processNames);

        HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        std::vector<DWORD> candidates;
        if (INVALID_HANDLE_VALUE != hThreadSnapshot)
        {
            THREADENTRY32 objThreadEntry32;
            objThreadEntry32.dwSize = sizeof(objThreadEntry32);

            if (Thread32First(hThreadSnapshot, &objThreadEntry32))
            {
                do
                {
                    std::wstring desc;
                    PerfQuery::PrintThreadDescription(objThreadEntry32.th32ThreadID, desc);

                    if (meetConditions(processNames[objThreadEntry32.th32OwnerProcessID], desc))
                    {
                        Thread thread;
                        thread.pid = objThreadEntry32.th32OwnerProcessID;
                        thread.processName = processNames[objThreadEntry32.th32OwnerProcessID];
                        thread.tid = objThreadEntry32.th32ThreadID;
                        thread.threadDesc = desc;
                        m_threads.push_back(thread);
                    }


                } while (Thread32Next(hThreadSnapshot, &objThreadEntry32));
            }

            CloseHandle(hThreadSnapshot);
        }
    }

    bool ThreadSearch::meetConditions(std::wstring processName, std::wstring threadDesc)
    {
        for (auto& instance : m_processNames)
        {
            if (processName.find(instance) != std::wstring::npos)
                return true;
        }

        for (auto& instance : m_threadDescs)
        {
            if (threadDesc.find(instance) != std::wstring::npos)
                return true;
        }

        return false;
    }


    void
        GetThreadCPUCycles::WaitAndProfile()
    {
        std::vector<std::thread*> threads;

        m_cycles.resize(m_threads.size());
        std::memset(m_cycles.data(), 0, sizeof(ULONG64) * m_cycles.size());

        for (int i = 0; i < m_threads.size(); i++)
        {
            std::thread* pThread = new std::thread(ThreadEyeEntry(m_threads[i].tid, m_cycles[i], m_timeout, m_fromStart));
            threads.push_back(pThread);
        }

        for (auto& instance : threads)
        {
            instance->join();
            delete instance;
        }



    }
}