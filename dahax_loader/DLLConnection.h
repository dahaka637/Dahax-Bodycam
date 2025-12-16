#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <atomic>

// Os nomes DEVEM bater com a DLL (DLLCommunication.h)
inline constexpr wchar_t SHM_NAME[] = L"dahax_ipc_mem";
inline constexpr wchar_t MUTEX_NAME[] = L"dahax_ipc_mutex";

// ---------------------------------------------------------------------
// Estruturas compartilhadas – IGUAIS às da DLL
// ---------------------------------------------------------------------
struct IPCChannel
{
    volatile LONG ready;   // 1 = há mensagem nova
    volatile LONG ack;     // 1 = mensagem foi lida / reconhecida
    char          message[256];
};

struct SharedHeader
{
    IPCChannel loaderToDll; // Loader escreve / DLL lê
    IPCChannel dllToLoader; // DLL escreve / Loader lê
};

// ---------------------------------------------------------------------
// Classe de comunicação do lado LOADER
// ---------------------------------------------------------------------
class DLLConnection
{
public:
    DLLConnection();
    ~DLLConnection();

    // Garante que o shared memory + mutex estão criados/abertos
    bool Initialize();

    // Handshake:
    //  - envia "LOADER_HELLO"
    //  - espera "DLL_ALIVE" dentro do timeout
    bool TryConnect(DWORD timeoutMs = 3000);

    // Envia string para a DLL (lado loaderToDll)
    bool Send(const std::string& msg);

    // Lê string enviada pela DLL (lado dllToLoader)
    // timeoutMs == 0 -> não bloqueia (retorna false se não tiver nada)
    bool Receive(std::string& out, DWORD timeoutMs = 0);

    // Fecha tudo
    void Close();

    bool IsConnected() const { return connected.load(); }

private:
    bool WriteChannel(IPCChannel& ch, const std::string& msg);
    bool ReadChannel(IPCChannel& ch, std::string& out);

private:
    HANDLE        hMap = nullptr;
    HANDLE        hMutex = nullptr;
    SharedHeader* header = nullptr;

    std::atomic<bool> connected{ false };
};
