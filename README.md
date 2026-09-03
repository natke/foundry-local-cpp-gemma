# Foundry Local 2.0.1 C++ Gemma sample

This sample provides two executables:

- `foundry_local_gemma` uses the Foundry Local v2.0.1 C++ Session API.
- `raw_ort_genai` loads an already-downloaded model directly with the bundled
  ONNX Runtime GenAI 0.15.2 library.

The Foundry Local executable:

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

Run the near-maximum context test:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --context-test
```

The default test uses 68 sections, which fits close to Gemma's packaged
4,096-token limit after reserving 256 output tokens. Pass a different section
count after `--context-test` to adjust its size. A 2,300-section document is
approximately 127,800 tokens before the chat wrapper.

The v2.0.1 macOS release asset supports Apple silicon.

## Raw ONNX Runtime GenAI

The bootstrap scripts download the ORT GenAI 0.15.2 public headers that match
the libraries bundled with Foundry Local v2.0.1. The raw executable accepts a
downloaded model directory rather than a catalog alias.

Run Gemma 4 directly:

```bash
./build/cmake/raw_ort_genai \
  "$HOME/.foundry_local_cpp_gemma/cache/models/Microsoft/gemma-4-e2b-it-generic-gpu-2/v2" \
  "Explain why the sky appears blue in two sentences."
```

Run Qwen 3.5 2B directly:

```bash
./build/cmake/raw_ort_genai \
  "$HOME/.foundry_local_cpp_gemma/cache/models/Microsoft/qwen3.5-2b-text-generic-gpu-3/v3" \
  "Explain why the sky appears blue in two sentences."
```

The raw runner applies each model's chat template, explicitly sets a
2,048-token prefill chunk size, and sets ORT GenAI `max_length` to the tokenized
input plus the requested output allowance. This permits diagnostic tests that
are not constrained by Foundry Local's request validation.

Run the 8K Gemma context test:

```bash
./build/cmake/raw_ort_genai \
  "$HOME/.foundry_local_cpp_gemma/cache/models/Microsoft/gemma-4-e2b-it-generic-gpu-2/v2" \
  --context-test 140
```

Run the approximately 128K Qwen context test:

```bash
./build/cmake/raw_ort_genai \
  "$HOME/.foundry_local_cpp_gemma/cache/models/Microsoft/qwen3.5-2b-text-generic-gpu-3/v3" \
  --context-test 2300
```

Catalog variant versions can change. Adjust these paths to match the model
directory printed by `foundry_local_gemma` after download.

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

Run a single audio prompt without an image on WebGPU:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --audio ./assets/recording.wav "Transcribe this audio and summarize it."
```

Run the same prompt on CPU:

```bash
FOUNDRY_LOCAL_DEVICE=cpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --audio ./assets/recording.wav "Transcribe this audio and summarize it."
```

### Foundry Local 2.0.1 audio issue

Text and image requests have been run successfully with the Gemma 4 WebGPU
variant. The audio-only commands above were tested on both available providers,
but neither currently completes:

| Provider | Exit code | Observed failure |
| --- | ---: | --- |
| WebGPU | 134 | The audio encoder's `GroupedConv` WGSL shader rejects `inf`, followed by an uncaught `std::out_of_range`. |
| CPU | 1 | `Foundry Local error [2]: Invalid or unsupported chat template.` |

The `--audio-demo` mode additionally attempts a combined image/audio request,
but it cannot reach that request while the initial audio request fails:

```bash
FOUNDRY_LOCAL_DEVICE=webgpu ./build/cmake/foundry_local_gemma gemma-4-e2b-it \
  --audio-demo ./assets/shapes.jpg ./assets/recording.wav
```

The equivalent raw ORT GenAI audio test is:

```bash
./build/cmake/raw_ort_genai \
  "$HOME/.foundry_local_cpp_gemma/cache/models/Microsoft/gemma-4-e2b-it-generic-gpu-2/v2" \
  --audio ./assets/recording.wav \
  "Transcribe this audio and summarize it in one sentence."
```

It fails in the same audio encoder before token generation:

```text
WebGPU device error: Error while parsing WGSL: unresolved value 'inf'
While calling Device.CreateShaderModule("GroupedConv")
libc++abi: terminating due to uncaught std::out_of_range
```

The raw process exits with code 134. This demonstrates that the WebGPU audio
failure occurs in ORT/audio-encoder execution rather than in the Foundry Local
chat-session wrapper.

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
