// esp.hpp
#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <atomic>
#include <mutex>

class MemoryManager;
class AddressCache;

// Variáveis globais (externas - serão definidas em esp.cpp)
extern std::unordered_map<uint64_t, std::string> known_targets;
extern std::vector<std::tuple<int, int, std::string>> targets_on_screen;
extern std::atomic<bool> is_running;
extern std::mutex data_mutex;

// Funções ESP
// Declarações das funções ESP
void StartESPThreads(MemoryManager* mem, AddressCache* cache);
void StopESPThreads();
void RenderESP(AddressCache& cache, MemoryManager& mem);