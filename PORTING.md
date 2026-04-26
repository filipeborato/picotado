# Picotado — Plano de Portabilidade (macOS / Linux)

> Estado atual: build oficial é **Windows-only** (VS 2026 + WebView2).
> Este documento descreve o que precisa mudar para gerar artefatos
> equivalentes em macOS e Linux. **Ainda não executado** — serve como
> referência técnica para quando decidirmos abrir frente cross-platform.

O código de DSP (`GranularEngine`, `BinauralRenderer`, libmysofa) é
portável sem alteração. A fricção está concentrada em três pontos:

1. **WebView backend** — hoje hard-coded em WebView2 (Windows).
2. **Formatos de plugin** — VST3 + Standalone hoje; AU (Mac) e LV2
   (Linux) são esperados em cada plataforma.
3. **Scripts de build/install** — `scripts/*.cmd` são CMD do Windows
   e referenciam paths como `%APPDATA%` e `%LOCALAPPDATA%`.

---

## Tabela comparativa

| Tópico              | Windows (atual)              | macOS                          | Linux                           |
| ------------------- | ---------------------------- | ------------------------------ | ------------------------------- |
| Toolchain           | VS 2026 + MSVC               | Xcode 15+ + clang              | gcc/clang + Ninja               |
| WebView backend     | WebView2 (Edge Chromium)     | WKWebView (system)             | WebKitGTK (`libwebkit2gtk-4.1`) |
| Web NuGet/dep       | `Microsoft.Web.WebView2`     | nenhuma — vem do sistema       | dependência runtime do .deb     |
| Formatos de plugin  | VST3, Standalone             | **AU**, VST3, Standalone       | **LV2**, VST3, Standalone       |
| Audio backend       | WASAPI / ASIO                | CoreAudio                      | JACK / ALSA / PipeWire          |
| HRTF storage path   | `%APPDATA%\Picotado\SOFA\`   | `~/Library/Application Support/Picotado/SOFA/` | `~/.config/Picotado/SOFA/` |
| Code signing        | opcional                     | **obrigatório p/ distribuir** (Apple ID + notarização) | nenhum                          |
| Distribuição        | NSIS / pasta `VST3` do user  | `.pkg` ou `.dmg`               | build-from-source / AppImage / `.deb` |
| Custo recorrente    | 0                            | USD 99/ano (Apple Developer)   | 0                               |

---

## macOS

### O que funciona sem mudança

- Toda a engine de áudio (`GranularEngine`, `BinauralRenderer`).
- libmysofa (CMake já compila no Mac).
- JUCE 8 — formato AU já está listado em `plugin/CMakeLists.txt:13`
  (`FORMATS AU VST3 Standalone`), só não foi exercitado.
- Build via CMake (presets atuais cobrem Ninja/Xcode com pequenos
  ajustes).

### O que precisa mudar

**1. WebView2 → WKWebView**

Em `plugin/source/PluginEditor.cpp:88` há
`.withWinWebView2Options(...)` configurando background color e
user data folder. No Mac essa chamada vira no-op (JUCE ignora), mas
vale guardá-la dentro de `#if JUCE_WINDOWS` para deixar a intenção
explícita e remover dependência conceitual.

WKWebView no Mac já é parte do sistema — não há equivalente do
NuGet WebView2 a baixar. O bloco MSVC em `CMakeLists.txt:117-127`
(que invoca `scripts/DownloadWebView2.ps1`) só roda em MSVC, então
no Mac é naturalmente skipado.

**2. Defines específicos de Windows**

`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` em
`plugin/CMakeLists.txt:103` é Windows-only. Envolver em
`if (WIN32)` ou `if (MSVC)`:

```cmake
if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
```

**3. App Sandbox / entitlements (se for distribuir via Mac App Store ou notarizar)**

O `loadSofaFunction` em `PluginEditor.cpp:208` abre `juce::FileChooser`.
Para passar pelo App Sandbox o bundle precisa do entitlement
`com.apple.security.files.user-selected.read-only`. JUCE expõe isso
via `juce_add_plugin(... HARDENED_RUNTIME_ENABLED TRUE
APP_SANDBOX_ENABLED TRUE APP_SANDBOX_OPTIONS
"com.apple.security.files.user-selected.read-only")`.

**4. Path do SOFA padrão**

Em `PluginEditor.cpp:215-218` o picker abre em
`%APPDATA%\Picotado\SOFA` via
`juce::File::userApplicationDataDirectory`. No Mac essa special
location resolve para `~/Library/Application Support/`, então o
caminho efetivo vira `~/Library/Application Support/Picotado/SOFA`
sem mudança de código. Apenas confirmar que o `download-sofa.cmd`
tem mirror shell (próximo item).

**5. Scripts**

Espelhar `scripts/*.cmd` em equivalentes shell:

| .cmd                  | equivalente shell                |
| --------------------- | -------------------------------- |
| `setup.cmd`           | desnecessário (sem WebView2)     |
| `download-sofa.cmd`   | `download-sofa.sh` com `curl`    |
| `release.cmd`         | `release.sh` (cmake + cp p/ `~/Library/Audio/Plug-Ins/`) |
| `build.cmd`           | `build.sh`                       |
| `run.cmd`             | `run.sh` (executa `.app` do Standalone) |
| `install-vst3.cmd`    | `install-vst3.sh` (copia para `~/Library/Audio/Plug-Ins/VST3/`) |
| `debug.cmd`           | abrir Xcode / `lldb`             |
| `clean.cmd`           | `clean.sh`                       |

**6. Code signing e notarização**

Para qualquer DAW comercial carregar o AU/VST3 sem aviso, o bundle
tem que ser assinado com Developer ID e notarizado. Requer:
- Apple Developer Program (USD 99/ano).
- `codesign --deep --options runtime --sign "Developer ID Application: ..."`.
- `xcrun notarytool submit ... --wait` + `xcrun stapler staple`.

JUCE tem hooks (`HARDENED_RUNTIME_ENABLED`,
`COPY_PLUGIN_AFTER_BUILD`) que podem ser plugados no `juce_add_plugin`.

### Esforço estimado (macOS)

- **Build local rodando** (sem signing, dev only): ~1 dia.
- **Distribuição assinada/notarizada**: +2 dias na primeira vez
  (configurar provisioning, scripts de signing, CI).
- **Manutenção contínua**: pouca — uma vez plugado no CMake, JUCE
  cuida do bundle.

---

## Linux

### O que funciona sem mudança

- Toda a engine de áudio.
- libmysofa (compila trivialmente em Linux — o repo `hoene/libmysofa`
  é nativo Linux).
- zlib (já vem do sistema; o `find_package(ZLIB)` shim no
  `cmake/` continua funcionando).
- JUCE 8 — suporta VST3, LV2 e Standalone em Linux nativamente.

### O que precisa mudar

**1. WebView backend — WebKitGTK**

JUCE em Linux usa `libwebkit2gtk-4.1` (ou `4.0` em distros mais antigas).
Vira **runtime dependency** do plugin/standalone:
- Build: `apt install libwebkit2gtk-4.1-dev`.
- Runtime no DAW do usuário: o pacote `.deb` precisa declarar
  `Depends: libwebkit2gtk-4.1-0` (ou guidance no README).

Sem WebKitGTK instalado, o plugin abre mas a UI fica em branco.

**2. Formato LV2**

LV2 é o formato padrão de plugin no ecossistema Linux (Ardour,
Carla, Bitwig, REAPER linux nativo). VST3 funciona, mas LV2 maximiza
adoção. Adicionar ao `plugin/CMakeLists.txt:13`:

```cmake
FORMATS AU VST3 LV2 Standalone
```

JUCE 8 já tem suporte LV2 em `juce_audio_processors_headless/format_types/LV2_SDK/`.
Define adicional pode ser necessário:
`LV2_URI="https://boratocompany.com/picotado"`.

**3. Audio backends**

Standalone roda nativamente em ALSA. Para JACK e PipeWire o usuário
precisa selecionar manualmente o device — JUCE expõe isso via
`AudioDeviceSelectorComponent`. PipeWire 0.3+ se apresenta como JACK,
então é transparente.

**4. Path do SOFA padrão**

Em Linux `juce::File::userApplicationDataDirectory` resolve para
`~/.config/`. Path efetivo: `~/.config/Picotado/SOFA/`. Sem mudança
de código.

**5. Defines Windows-only**

Mesmo cuidado do Mac: envolver
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` em `if (MSVC)`.

**6. Dependências do sistema**

Documentar no README. Mínimo para build:
```
apt install build-essential cmake ninja-build \
    libwebkit2gtk-4.1-dev \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libfontconfig1-dev \
    libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev
```

(JUCE mantém lista canônica em
[`docs/Linux Dependencies.md`](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md).)

**7. Scripts**

Mesma lista do Mac (`.sh`) + um `.deb` opcional via CPack.

**8. Distribuição**

Três caminhos não-exclusivos:
- **Build-from-source**: usuário roda `cmake --build --preset
  release` e copia o `.so`/`.lv2` para `~/.lv2/` ou `~/.vst3/`.
- **AppImage** do Standalone (empacota WebKitGTK junto).
- **`.deb`/`.rpm`** via CPack — declara `Depends:
  libwebkit2gtk-4.1-0` e instala em `/usr/lib/lv2/` e `/usr/lib/vst3/`.

### Esforço estimado (Linux)

- **Build local rodando**: ~1 dia (lidar com deps de sistema é a
  parte chata).
- **LV2 + VST3 funcionando em Ardour/REAPER**: +0.5 dia.
- **Empacotamento `.deb`**: +1 dia.
- **CI matrix (Ubuntu LTS)**: +0.5 dia.

---

## Resumo de arquivos afetados

| Arquivo                                    | Mudança                                                        |
| ------------------------------------------ | -------------------------------------------------------------- |
| `plugin/CMakeLists.txt`                    | `FORMATS` por plataforma; envolver defines MSVC em `if (MSVC)` |
| `plugin/source/PluginEditor.cpp:88`        | `.withWinWebView2Options(...)` dentro de `#if JUCE_WINDOWS`    |
| `CMakeLists.txt:117-127`                   | já está em `if (MSVC)` — OK                                   |
| `scripts/*.cmd`                            | espelhar em `.sh`                                              |
| `Picotado.jucer`                           | redundante para Mac/Linux (CMake é o caminho oficial)          |
| `README.md`                                | seções "Build no Mac" e "Build no Linux"                       |

A engine (`GranularEngine.cpp`, `BinauralRenderer.cpp`,
`PluginProcessor.cpp`, `Grain.h`, `ParameterIDs.h`) não precisa
mudar.

---

## Ordem sugerida de execução (quando for fazer)

1. **Linux primeiro**. Mais barato (sem signing), exercita CMake
   cross-platform, pega mais bugs de portabilidade que Mac.
2. **Mac depois**. Reaproveita as mudanças de Linux (defines
   condicionais, scripts shell). Custo é assinatura/notarização.
3. **CI (GitHub Actions matrix)**. Só depois das duas plataformas
   buildando local — caso contrário gasta-se tempo iterando em CI.

---

## Notas de risco

- **WebKitGTK 4.1 vs 4.0**: Ubuntu 22.04 tem 4.0, 24.04 tem 4.1. Pode
  precisar suportar ambos via `pkg-config` fallback. JUCE 8 resolve
  isso internamente, mas vale validar.
- **App Sandbox no Mac**: se um DAW (ex: Logic) rodar sandboxed, o
  `FileChooser` para SOFA precisa do entitlement explícito.
- **VST3 vs LV2 em DAWs Linux**: Bitwig prefere VST3, Ardour prefere
  LV2. Suportar os dois é mais simples que escolher.
- **PipeWire**: é o padrão em Fedora/Ubuntu novos. Apresenta-se como
  JACK, então não exige código novo, mas vale testar.
