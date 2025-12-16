#include "DLLConnection.h"
#include <iostream>

DLLConnection::DLLConnection() = default;

DLLConnection::~DLLConnection()
{
    Close();
}

// ======================================================
// Inicializa shared memory + mutex (lado LOADER)
// ======================================================
bool DLLConnection::Initialize()
{
    if (header && hMutex && hMap)
        return true;

    // Cria OU abre a região de memória compartilhada
    HANDLE map = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedHeader),
        SHM_NAME
    );

    if (!map)
    {
        std::cout << "[-] CreateFileMappingW falhou (" << GetLastError() << ")\n";
        return false;
    }

    DWORD lastError = GetLastError();
    hMap = map;

    void* view = MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(SharedHeader)
    );

    if (!view)
    {
        std::cout << "[-] MapViewOfFile falhou (" << GetLastError() << ")\n";
        CloseHandle(hMap);
        hMap = nullptr;
        return false;
    }

    header = reinterpret_cast<SharedHeader*>(view);

    // Mutex compartilhado
    hMutex = CreateMutexW(nullptr, FALSE, MUTEX_NAME);
    if (!hMutex)
    {
        std::cout << "[-] CreateMutexW falhou (" << GetLastError() << ")\n";
        UnmapViewOfFile(header);
        header = nullptr;
        CloseHandle(hMap);
        hMap = nullptr;
        return false;
    }

    // Se fomos o primeiro a criar o mapeamento, zeramos a estrutura
    if (lastError != ERROR_ALREADY_EXISTS)
    {
        WaitForSingleObject(hMutex, INFINITE);
        ZeroMemory(header, sizeof(SharedHeader));
        ReleaseMutex(hMutex);
    }

    connected.store(false);
    return true;
}

// ======================================================
// Helpers de canal
// ======================================================
bool DLLConnection::WriteChannel(IPCChannel& ch, const std::string& msg)
{
    if (!hMutex || !header)
        return false;

    if (msg.size() >= sizeof(ch.message))
        return false;

    WaitForSingleObject(hMutex, INFINITE);

    // Se ainda tem mensagem pendente não lida, não sobrescreve
    if (ch.ready && !ch.ack)
    {
        ReleaseMutex(hMutex);
        return false;
    }

    ZeroMemory(ch.message, sizeof(ch.message));
    memcpy(ch.message, msg.c_str(), msg.size());

    ch.ready = 1;
    ch.ack = 0;

    ReleaseMutex(hMutex);
    return true;
}

bool DLLConnection::ReadChannel(IPCChannel& ch, std::string& out)
{
    if (!hMutex || !header)
        return false;

    WaitForSingleObject(hMutex, INFINITE);

    if (!ch.ready || ch.ack)
    {
        ReleaseMutex(hMutex);
        return false;
    }

    out.assign(ch.message);

    ch.ack = 1;
    ch.ready = 0;

    ReleaseMutex(hMutex);
    return true;
}

// ======================================================
// API pública
// ======================================================
bool DLLConnection::Send(const std::string& msg)
{
    if (!header)
        return false;

    return WriteChannel(header->loaderToDll, msg);
}

bool DLLConnection::Receive(std::string& out, DWORD timeoutMs)
{
    if (!header)
        return false;

    DWORD start = GetTickCount();

    for (;;)
    {
        if (ReadChannel(header->dllToLoader, out))
            return true;

        if (timeoutMs == 0)
            return false;

        if (GetTickCount() - start >= timeoutMs)
            return false;

        Sleep(10);
    }
}

// ======================================================
// Handshake LOADER <-> DLL
// ======================================================
bool DLLConnection::TryConnect(DWORD timeoutMs)
{
    if (!Initialize())
        return false;

    DWORD start = GetTickCount();
    std::string msg;

    while (GetTickCount() - start < timeoutMs)
    {
        // ENVIA SEMPRE
        Send("LOADER_HELLO");

        // TENTA RECEBER
        if (Receive(msg, 100))
        {
            if (msg == "DLL_ALIVE")
            {
                connected.store(true);
                return true;
            }
        }

        Sleep(50);
    }

    return false;
}


// ======================================================
// Limpeza
// ======================================================
void DLLConnection::Close()
{
    connected.store(false);

    if (header)
    {
        UnmapViewOfFile(header);
        header = nullptr;
    }

    if (hMap)
    {
        CloseHandle(hMap);
        hMap = nullptr;
    }

    if (hMutex)
    {
        CloseHandle(hMutex);
        hMutex = nullptr;
    }
}
