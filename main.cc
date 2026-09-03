#include <foundry_local/foundry_local_cpp.h>

#include "long_context_test.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace foundry_local;

namespace {

constexpr std::string_view kDefaultModelAlias = "gemma-4-e2b-it";

std::string Lowercase(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return result;
}

void PrintGemmaModels(const ModelList& models) {
  bool found = false;
  for (const auto& model : models) {
    const ModelInfo info = model->GetInfo();
    if (Lowercase(info.Alias()).find("gemma") == std::string::npos &&
        Lowercase(info.Name()).find("gemma") == std::string::npos) {
      continue;
    }

    if (!found) {
      std::cerr << "Gemma models in the current catalog:\n";
      found = true;
    }
    std::cerr << "  " << info.Alias() << " (" << info.Name() << ")\n";
  }

  if (!found) {
    std::cerr << "The current catalog does not contain a Gemma model.\n";
  }
}

void SelectRequestedVariant(IModel& model) {
  const char* requested_value = std::getenv("FOUNDRY_LOCAL_DEVICE");
  if (!requested_value || Lowercase(requested_value) == "auto") {
    return;
  }

  const std::string requested = Lowercase(requested_value);
  for (const auto& variant : model.GetVariants()) {
    const ModelInfo info = variant->GetInfo();
    const std::string execution_provider =
        info.ExecutionProvider() ? Lowercase(*info.ExecutionProvider()) : std::string();

    const bool matches =
        (requested == "cpu" && info.DeviceType() == FOUNDRY_LOCAL_DEVICE_CPU) ||
        (requested == "webgpu" && execution_provider == "webgpuexecutionprovider") ||
        (requested == "cuda" && execution_provider == "cudaexecutionprovider");
    if (matches) {
      model.SelectVariant(*variant);
      return;
    }
  }

  throw std::runtime_error("No model variant matched FOUNDRY_LOCAL_DEVICE=" + requested);
}

std::string StreamMessage(IModel& model, MessageItem user_message,
                          std::optional<int64_t> max_output_tokens = std::nullopt) {
  ChatSession session(model);
  session.SetStreamingCallback([](flStreamingCallbackData event) -> int {
    flItem* raw_item = nullptr;
    if (!detail::item_api()->ItemQueue_TryPop(event.item_queue, &raw_item)) {
      return 0;
    }

    Item item(*raw_item);
    if (item.GetType() == FOUNDRY_LOCAL_ITEM_TEXT) {
      std::cout << item.GetText().text << std::flush;
    }
    return 0;
  });

  Request request;
  request.AddItem(SystemMessage("You are a concise and helpful assistant."));
  request.AddItem(std::move(user_message));
  if (max_output_tokens) {
    RequestOptions options;
    options.search.max_output_tokens = *max_output_tokens;
    request.SetOptions(options);
  }

  std::cout << "Assistant: ";
  const Response response = session.ProcessRequest(request);
  std::cout << "\n";

  const flUsage usage = response.GetUsage();
  std::cout << "Tokens: " << usage.prompt_tokens << " prompt, " << usage.completion_tokens
            << " completion\n";

  std::string output;
  for (const auto& item : response.GetItems()) {
    if (item.GetType() != FOUNDRY_LOCAL_ITEM_MESSAGE) {
      continue;
    }
    const auto message = item.GetMessage();
    if (message.IsSimpleText()) {
      output += message.GetSimpleText();
    }
  }
  return output;
}

void StreamTextResponse(IModel& model, const std::string& prompt) {
  StreamMessage(model, UserMessage(prompt));
}

void StreamImageResponse(IModel& model, const std::string& prompt, const std::string& image_path) {
  std::vector<Item> parts;
  parts.push_back(Item::Text(prompt));
  parts.push_back(Item::ImageFromUri(image_path));
  StreamMessage(model, MessageItem(FOUNDRY_LOCAL_ROLE_USER, std::move(parts)));
}

std::vector<std::uint8_t> ReadFile(const std::string& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    throw std::runtime_error("Unable to open media file: " + path);
  }

  const auto size = input.tellg();
  std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
  input.seekg(0);
  input.read(reinterpret_cast<char*>(data.data()), size);
  if (!input) {
    throw std::runtime_error("Unable to read media file: " + path);
  }
  return data;
}

std::string FileFormat(const std::string& path) {
  const auto extension = path.find_last_of('.');
  if (extension == std::string::npos || extension + 1 == path.size()) {
    throw std::runtime_error("Media file has no extension: " + path);
  }
  return Lowercase(std::string_view(path).substr(extension + 1));
}

void StreamAudioResponse(IModel& model, const std::string& prompt, const std::string& audio_path) {
  const auto audio = ReadFile(audio_path);
  std::vector<Item> parts;
  parts.push_back(Item::Text(prompt));
  parts.push_back(Item::AudioFromData(FileFormat(audio_path), audio.data(), audio.size()));
  StreamMessage(model, MessageItem(FOUNDRY_LOCAL_ROLE_USER, std::move(parts)));
}

void RunAudioDemo(IModel& model, const std::string& shapes_image, const std::string& audio_file) {
  std::cout << "\n--- Audio understanding ---\n";
  StreamAudioResponse(model, "Transcribe this audio, then summarize it in one sentence.", audio_file);

  std::cout << "\n--- Combined image and audio ---\n";
  const auto audio = ReadFile(audio_file);
  std::vector<Item> parts;
  parts.push_back(Item::Text(
      "Describe the image, summarize the audio, and clearly separate the two observations."));
  parts.push_back(Item::ImageFromUri(shapes_image));
  parts.push_back(Item::AudioFromData(FileFormat(audio_file), audio.data(), audio.size()));
  StreamMessage(model, MessageItem(FOUNDRY_LOCAL_ROLE_USER, std::move(parts)));
}

void RunMultimodalDemo(IModel& model, const std::string& shapes_image, const std::string& ui_image,
                       const std::string& audio_file) {
  std::cout << "\n--- Text reasoning ---\n";
  StreamTextResponse(model, "A farmer has 17 sheep and all but 9 run away. How many remain? Explain briefly.");

  std::cout << "\n--- Simple image understanding ---\n";
  StreamImageResponse(model, "List every visible shape and its color.", shapes_image);

  std::cout << "\n--- UI screenshot understanding ---\n";
  StreamImageResponse(model,
                      "Summarize this interface and identify the currently selected transcription model.",
                      ui_image);

  RunAudioDemo(model, shapes_image, audio_file);
}

void RunContextTest(IModel& model, std::size_t section_count) {
  const std::string document = sample::BuildLongDocument(section_count);
  std::cout << "\n--- Near-maximum context summary test ---\n";
  std::cout << "Sections: " << section_count << "\n";
  std::cout << "Document bytes: " << document.size() << "\n";

  const std::string output = StreamMessage(model, UserMessage(document), 256);
  std::cout << "Anchor recall: " << sample::CountAnchorLabels(output) << "/5\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  const std::string model_alias = argc > 1 ? argv[1] : std::string(kDefaultModelAlias);
  const bool run_multimodal_demo = argc >= 6 && std::string_view(argv[2]) == "--multimodal-demo";
  const bool run_audio_demo = argc >= 5 && std::string_view(argv[2]) == "--audio-demo";
  const bool run_image_prompt = argc >= 5 && std::string_view(argv[2]) == "--image";
  const bool run_audio_prompt = argc >= 5 && std::string_view(argv[2]) == "--audio";
  const bool run_context_test = argc >= 3 && std::string_view(argv[2]) == "--context-test";
  std::vector<std::string> prompts;
  if (!run_multimodal_demo && !run_audio_demo && !run_image_prompt && !run_audio_prompt &&
      !run_context_test) {
    for (int argument = 2; argument < argc; ++argument) {
      prompts.emplace_back(argv[argument]);
    }
  }
  if (prompts.empty()) {
    prompts = {
        "Explain why the sky appears blue in two sentences.",
        "Write a haiku about running AI locally.",
        "What are three practical benefits of on-device inference?",
    };
  }

  try {
    Configuration config("foundry_local_cpp_gemma");
    if (const char* cache_dir = std::getenv("FOUNDRY_LOCAL_MODEL_CACHE")) {
      config.SetModelCacheDir(cache_dir);
    }

    Manager manager(std::move(config));
    auto& catalog = manager.GetCatalog();
    auto model = catalog.GetModel(model_alias);

    if (!model) {
      std::cerr << "Model alias '" << model_alias << "' was not found.\n";
      PrintGemmaModels(catalog.GetModels());
      std::cerr << "Run this sample again with an alias printed above.\n";
      return 2;
    }

    SelectRequestedVariant(*model);
    const ModelInfo info = model->GetInfo();
    std::cout << "Foundry Local SDK: " << Version() << "\n";
    std::cout << "Model: " << info.Name() << "\n";
    std::cout << "Variant: " << info.Id() << "\n";

    if (!model->IsCached()) {
      std::cout << "Downloading model...\n";
      model->Download([](float progress) -> int {
        std::cout << "\rDownloaded " << static_cast<int>(progress) << "%" << std::flush;
        return 0;
      });
      std::cout << "\n";
    }

    std::cout << "Model path: " << model->GetPath() << "\n";
    if (!model->IsLoaded()) {
      std::cout << "Loading model...\n";
      model->Load();
    }

    if (run_multimodal_demo) {
      RunMultimodalDemo(*model, argv[3], argv[4], argv[5]);
    } else if (run_audio_demo) {
      RunAudioDemo(*model, argv[3], argv[4]);
    } else if (run_image_prompt) {
      StreamImageResponse(*model, argv[4], argv[3]);
    } else if (run_audio_prompt) {
      StreamAudioResponse(*model, argv[4], argv[3]);
    } else if (run_context_test) {
      const std::size_t section_count = argc >= 4 ? std::stoull(argv[3]) : 68;
      RunContextTest(*model, section_count);
    } else {
      for (const auto& prompt : prompts) {
        std::cout << "\nUser: " << prompt << "\n";
        StreamTextResponse(*model, prompt);
      }
    }
    model->Unload();
  } catch (const Error& error) {
    std::cerr << "Foundry Local error [" << error.Code() << "]: " << error.what() << "\n";
    return 1;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }

  return 0;
}
