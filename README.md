# Foundry Local 2.0.1 C++ Gemma sample

This sample uses the Foundry Local v2.0.1 C++ Session API to:

1. Resolve a model from the live catalog.
2. Download its selected hardware-optimized variant.
3. Load it into memory.
4. Stream a chat response.

The default alias is `gemma-4-e2b-it`. Foundry Local automatically selects a
hardware-optimized variant. Set `FOUNDRY_LOCAL_DEVICE` to `cpu`, `webgpu`, or
`cuda` to request a specific variant. The CPU variant is about 6.3 GB, so make
sure sufficient disk space is available before running it.

## macOS

From the sample directory:

```bash
cd /Users/natke/Develop/samples/foundry-local-cpp-gemma
./scripts/bootstrap.sh
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --config Release --parallel
```

Run the default prompt set with WebGPU:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it
```

Run custom prompts:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  "Write a haiku about local AI." \
  "Name three benefits of on-device inference."
```

The v2.0.1 macOS release asset supports Apple silicon.

## Linux

The bootstrap script supports x64 and ARM64:

```bash
./scripts/bootstrap.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
FOUNDRY_LOCAL_DEVICE=cpu ./build/foundry_local_gemma gemma-4-e2b-it
```

## Multimodal requests

Run a single image prompt without audio:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --image ./assets/shapes.jpg "Describe every shape and its color."
```

Run text and image requests, followed by experimental audio requests:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --multimodal-demo ./assets/shapes.jpg ./assets/interface.png ./assets/recording.wav
```

Images are accepted as file paths. Audio is read into memory and passed through
`Item::AudioFromData`, because v2.0.1 rejects URI-based audio in chat requests.

### Foundry Local 2.0.1 audio issue

Text and image requests have been run successfully with the Gemma 4 WebGPU
variant. Audio does not currently complete:

- WebGPU fails in the audio encoder with an invalid WGSL shader involving
  `inf`, followed by an uncaught `std::out_of_range`.
- CPU returns `Invalid or unsupported chat template`.

The standalone audio retry mode is:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --audio-demo ./assets/shapes.jpg ./assets/recording.wav
```

## Windows

Run from a Visual Studio Developer PowerShell:

```powershell
.\scripts\bootstrap.ps1
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\foundry_local_gemma.exe "gemma-4-e2b-it" `
  "Write a haiku about local AI." `
  "Name three benefits of on-device inference."
```

For Windows ARM64, omit `-A x64` or select an ARM64 generator.

## Model aliases

Catalog contents can change independently of the SDK release. If the requested
alias is unavailable, the program prints every Gemma alias currently returned
by the catalog. Run it again with one of those aliases:

```bash
./build/foundry_local_gemma "<catalog-alias>" "<prompt>"
```

Set `FOUNDRY_LOCAL_MODEL_CACHE` to override the model download directory.
