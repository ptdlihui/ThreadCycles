#pragma once
#include <windows.h>  
#include <string>
#include <tchar.h>
#include <vector>
#include <unordered_map>

class ThreadDescription
{
public:
    typedef HRESULT(WINAPI * SetThreadDescriptionFunc)(HANDLE, PCWSTR);
    typedef HRESULT(WINAPI * GetThreadDescriptionFunc)(HANDLE, PWSTR*);

    ThreadDescription()
        : m_pSetDescription(nullptr)
        , m_pGetDescription(nullptr)
    {
        m_kernel32 = LoadLibrary(L"kernel32.dll");

        if (m_kernel32)
        {
            m_pSetDescription = reinterpret_cast<SetThreadDescriptionFunc>(GetProcAddress(m_kernel32, "SetThreadDescription"));
            m_pGetDescription = reinterpret_cast<GetThreadDescriptionFunc>(GetProcAddress(m_kernel32, "GetThreadDescription"));
        }
    }

    HRESULT SetThreadDescription(HANDLE handle, PCWSTR name)
    {
        if (m_pSetDescription)
            return m_pSetDescription(handle, name);

        return ERROR_INVALID_FUNCTION;
    }

    HRESULT GetThreadDescription(HANDLE handle, PWSTR* pName)
    {
        if (m_pGetDescription)
            return m_pGetDescription(handle, pName);

        return ERROR_INVALID_FUNCTION;
    }

    ~ThreadDescription()
    {
        if (m_kernel32)
            FreeLibrary(m_kernel32);
    }
protected:
    HMODULE m_kernel32;
    SetThreadDescriptionFunc m_pSetDescription;
    GetThreadDescriptionFunc m_pGetDescription;
};

namespace PerfQuery
{
bool PrintProcessNameAndID(DWORD processID, std::wstring& name);
bool PrintThreadDescription(DWORD threadId, std::wstring& desc);

typedef std::unordered_map<DWORD, std::wstring> ProcessNamesTable;
void BuildProcessnNmesTable(ProcessNamesTable& table);

struct Thread
{
    DWORD tid;
    DWORD pid;
    std::wstring threadDesc;
    std::wstring processName;
};

class ThreadSearch
{
public:
    ThreadSearch()
    {

    }

    ThreadSearch(std::vector<std::wstring>& processNames, std::vector<std::wstring>& threadDescs)
        : m_processNames(processNames)
        , m_threadDescs(threadDescs)
    {

    }

    void AddProcessName(std::wstring name)
    {
        m_processNames.push_back(name);
    }

    void AddThreadDesc(std::wstring desc)
    {
        m_threadDescs.push_back(desc);
    }

    const std::vector<Thread>& GetSearchResult() const
    {
        return m_threads;
    }

    void Reset()
    {
        m_threads.clear();
        m_processNames.clear();
        m_threadDescs.clear();
    }

    void Search();

protected:
    bool meetConditions(std::wstring, std::wstring);
protected:
    std::vector<Thread> m_threads;
    std::vector<std::wstring> m_processNames;
    std::vector<std::wstring> m_threadDescs;
};

class ThreadEyeEntry
{
public:
    ThreadEyeEntry(DWORD id, ULONG64& cycles, DWORD timeout, bool fromStart = true) : m_threadId(id), m_cycles(cycles), m_timeout(timeout), m_fromStart(fromStart) {}

    void operator()()
    {
        HANDLE threadHandle = OpenThread(SYNCHRONIZE | THREAD_QUERY_INFORMATION, FALSE, m_threadId);
        if (NULL == threadHandle)
            return;

        ULONG64 start = 0;
        if (m_fromStart)
            QueryThreadCycleTime(threadHandle, &start);

        DWORD timeout = m_timeout == 0 ? INFINITE : m_timeout;
        DWORD dwRet = WaitForSingleObject(threadHandle, timeout);

        ULONG64 cycles = 0;
        QueryThreadCycleTime(threadHandle, &cycles);

        m_cycles = cycles - start;

        CloseHandle(threadHandle);
    }

protected:
    DWORD m_threadId;
    bool m_fromStart;
    ULONG64& m_cycles;
    DWORD m_timeout;

};


class GetThreadCPUCycles
{
public:
    GetThreadCPUCycles(const std::vector<Thread>& threads, DWORD timeout, bool fromStart)
        : m_threads(threads)
        , m_timeout(timeout)
        , m_fromStart(fromStart)
    {

    }

    void WaitAndProfile();
    const std::vector<ULONG64>& Cycles() const
    {
        return m_cycles;
    }


protected:
    const std::vector<Thread> m_threads;
    DWORD m_timeout;
    std::vector<ULONG64> m_cycles;
    bool m_fromStart;
};
}
