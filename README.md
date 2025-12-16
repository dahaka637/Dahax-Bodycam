# Dahax Bodycam

Dahax Bodycam é um projeto em C++ dividido em dois módulos principais (DLL + Loader), projetado para interação em tempo real com o jogo **Bodycam (Unreal Engine)**.  
A arquitetura é baseada em **injeção de DLL**, **IPC via Shared Memory** e **overlay externo com ImGui**.

O foco do projeto é fornecer uma base sólida para:
- coleta de dados do jogo
- renderização de ESP
- lógica de Aimbot
- experimentação com leitura direta de memória (offsets)

---

## Estrutura Geral do Projeto

O projeto é composto por dois executáveis independentes, cada um com responsabilidades bem definidas.

```
Dahax-Bodycam/
├── dahax/              # DLL injetada no jogo (internal)
└── dahax_loader/       # Loader + Overlay externo (external)
```

---

## Arquitetura

### DLL Interna (dahax)

A DLL é injetada diretamente no processo do jogo e roda dentro do mesmo espaço de memória.

Responsabilidades:
- Leitura das estruturas internas do Unreal Engine
- Coleta de dados do jogador local, players, bots e bones
- Execução da lógica do Aimbot
- Envio de dados via Shared Memory para o processo externo

Principais componentes:
- dllmain.cpp – inicialização e ciclo de vida da DLL
- Cache – coleta e atualização dos atores
- Aimbot – lógica de mira (FOV, smooth, predição)
- SharedSender / SharedMemory – comunicação IPC

A DLL utiliza threads separadas para:
- atualização do cache
- envio de snapshots ao processo externo

---
<img width="1919" height="1076" alt="image" src="https://github.com/user-attachments/assets/daa8bd5f-0ab7-423d-a9e7-39764690c5e4" />

### Loader Externo (dahax_loader)

O loader roda fora do processo do jogo e é responsável pela interface visual e controle.

Responsabilidades:
- Localizar o processo do jogo
- Injetar a DLL
- Criar overlay com ImGui
- Ler dados do Shared Memory
- Renderizar ESP
- Controlar menus e hotkeys
- Ler a câmera diretamente da memória via offsets

Principais componentes:
- dahax_loader.cpp – entry point e loop principal
- Injector – injeção da DLL
- Overlay – janela transparente e renderização
- ESP / ESPHelper – desenho e projeção WorldToScreen
- Menu – interface gráfica
- Offsets – leitura direta da câmera

---

## Comunicação DLL ↔ Loader

A comunicação é feita via Shared Memory com controle de consistência usando seqlock.

Fluxo simplificado:

```
Jogo → dahax.dll → SharedMemory → dahax_loader.exe → Overlay / ESP
```

A DLL escreve os dados e o loader apenas realiza leitura.

---

## Funcionalidades

### Aimbot
- Seleção por FOV angular
- Alvo configurável (Head, Chest, NearestBone)
- Verificação de visibilidade
- Filtro por time
- Smooth configurável
- Predição baseada em movimento

### ESP
- Box 2D dinâmico
- Nome do jogador / BOT
- Distância em metros
- Skeleton
- Snaplines
- Visualização do FOV do Aimbot

---

## Tecnologias Utilizadas

- C++17
- Windows API
- ImGui
- Unreal Engine (estruturas internas)
- DLL Injection
- Shared Memory IPC
- Seqlock
- MinHook

---

## Aviso

Este projeto possui finalidade educacional e experimental, voltado ao estudo de:
- engenharia reversa
- arquitetura de sistemas em tempo real
- sincronização entre processos
- matemática 3D aplicada

O uso em ambientes online pode violar termos de serviço.

---

## Observações Finais

O código foi estruturado para manter separação clara entre lógica interna e interface externa, facilitando manutenção, estudo e extensão do projeto.
