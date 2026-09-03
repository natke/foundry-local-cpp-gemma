#include <ort_genai.h>

#include "long_context_test.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kDefaultMaxOutputTokens = 128;
constexpr int kContextTestMaxOutputTokens = 256;
constexpr int kPrefillChunkSize = 2048;

std::string ReadTextFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Unable to open file: " + path);
  }
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

std::string EscapeJson(std::string_view value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char character : value) {
    switch (character) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += character;
        break;
    }
  }
  return escaped;
}

std::string ApplyChatTemplate(const OgaTokenizer& tokenizer, const std::string& model_path,
                              std::string_view prompt) {
  const std::string chat_template = ReadTextFile(model_path + "/chat_template.jinja");
  const std::string messages =
      R"([{"role":"system","content":"You are a concise and helpful assistant."},{"role":"user","content":")" +
      EscapeJson(prompt) + R"("}])";
  const OgaString formatted =
      tokenizer.ApplyChatTemplate(chat_template.c_str(), messages.c_str(), nullptr, true);
  return static_cast<const char*>(formatted);
}

std::string ApplyAudioChatTemplate(const OgaTokenizer& tokenizer, const std::string& model_path,
                                   std::string_view prompt) {
  const std::string chat_template = ReadTextFile(model_path + "/chat_template.jinja");
  const std::string messages =
      R"([{"role":"system","content":"You are a concise and helpful assistant."},{"role":"user","content":[{"type":"audio"},{"type":"text","text":")" +
      EscapeJson(prompt) + R"("}]}])";
  const OgaString formatted =
      tokenizer.ApplyChatTemplate(chat_template.c_str(), messages.c_str(), nullptr, true);
  return static_cast<const char*>(formatted);
}

std::string JoinArguments(int argc, char* argv[], int first) {
  std::string value;
  for (int index = first; index < argc; ++index) {
    if (!value.empty()) {
      value += ' ';
    }
    value += argv[index];
  }
  return value;
}

std::string Generate(const std::string& model_path, const std::string& prompt,
                     int max_output_tokens) {
  OgaHandle handle;
  auto model = OgaModel::Create(model_path.c_str());
  auto tokenizer = OgaTokenizer::Create(*model);
  auto sequences = OgaSequences::Create();

  const std::string formatted_prompt = ApplyChatTemplate(*tokenizer, model_path, prompt);
  tokenizer->Encode(formatted_prompt.c_str(), *sequences);
  const std::size_t input_tokens = sequences->SequenceCount(0);
  const std::size_t max_length = input_tokens + static_cast<std::size_t>(max_output_tokens);

  std::cout << "ORT GenAI model type: " << static_cast<const char*>(model->GetType()) << "\n";
  std::cout << "ORT GenAI device: " << static_cast<const char*>(model->GetDeviceType()) << "\n";
  std::cout << "Prompt tokens: " << input_tokens << "\n";
  std::cout << "Requested max length: " << max_length << "\n";
  std::cout << "Prefill chunk size: " << kPrefillChunkSize << "\n";

  auto params = OgaGeneratorParams::Create(*model);
  params->SetSearchOption("max_length", static_cast<double>(max_length));
  params->SetSearchOption("chunk_size", kPrefillChunkSize);
  params->SetSearchOptionBool("do_sample", false);

  auto generator = OgaGenerator::Create(*model, *params);
  generator->AppendTokenSequences(*sequences);
  auto stream = OgaTokenizerStream::Create(*tokenizer);

  std::string output;
  std::cout << "Assistant: ";
  while (!generator->IsDone()) {
    generator->GenerateNextToken();
    const std::size_t count = generator->GetSequenceCount(0);
    if (count <= input_tokens) {
      continue;
    }
    const int32_t token = generator->GetSequenceData(0)[count - 1];
    const char* text = stream->Decode(token);
    output += text;
    std::cout << text << std::flush;
  }
  std::cout << "\nCompletion tokens: " << generator->GetSequenceCount(0) - input_tokens << "\n";
  return output;
}

std::string GenerateAudio(const std::string& model_path, const std::string& audio_path,
                          const std::string& prompt) {
  OgaHandle handle;
  auto model = OgaModel::Create(model_path.c_str());
  auto tokenizer = OgaTokenizer::Create(*model);
  auto processor = OgaMultiModalProcessor::Create(*model);

  std::vector<const char*> audio_paths = {audio_path.c_str()};
  auto audios = OgaAudios::Load(audio_paths);
  const std::string formatted_prompt =
      ApplyAudioChatTemplate(*tokenizer, model_path, prompt);
  auto inputs = processor->ProcessAudios(formatted_prompt.c_str(), audios.get());

  auto input_ids = inputs->Get("input_ids");
  const auto input_shape = input_ids->Shape();
  if (input_shape.empty() || input_shape.back() <= 0) {
    throw std::runtime_error("Audio processor returned invalid input_ids");
  }
  const std::size_t input_tokens = static_cast<std::size_t>(input_shape.back());
  const std::size_t max_length = input_tokens + kContextTestMaxOutputTokens;

  std::cout << "ORT GenAI model type: " << static_cast<const char*>(model->GetType()) << "\n";
  std::cout << "ORT GenAI device: " << static_cast<const char*>(model->GetDeviceType()) << "\n";
  std::cout << "Audio: " << audio_path << "\n";
  std::cout << "Prompt tokens after audio expansion: " << input_tokens << "\n";
  std::cout << "Requested max length: " << max_length << "\n";

  auto params = OgaGeneratorParams::Create(*model);
  params->SetSearchOption("max_length", static_cast<double>(max_length));
  params->SetSearchOption("chunk_size", kPrefillChunkSize);
  params->SetSearchOptionBool("do_sample", false);

  auto generator = OgaGenerator::Create(*model, *params);
  generator->SetInputs(*inputs);
  auto stream = OgaTokenizerStream::Create(*tokenizer);

  std::string output;
  std::cout << "Assistant: ";
  while (!generator->IsDone()) {
    generator->GenerateNextToken();
    const auto next_tokens = generator->GetNextTokens();
    if (next_tokens.empty()) {
      continue;
    }
    const char* text = stream->Decode(next_tokens[0]);
    output += text;
    std::cout << text << std::flush;
  }
  std::cout << "\nCompletion tokens: " << generator->GetSequenceCount(0) - input_tokens << "\n";
  return output;
}

void PrintUsage(const char* executable) {
  std::cerr << "Usage:\n"
            << "  " << executable << " <model-directory> [prompt]\n"
            << "  " << executable << " <model-directory> --audio <audio-file> [prompt]\n"
            << "  " << executable << " <model-directory> --context-test [section-count]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsage(argv[0]);
    return 2;
  }

  try {
    const std::string model_path = argv[1];
    const bool context_test = argc >= 3 && std::string_view(argv[2]) == "--context-test";
    const bool audio_test = argc >= 4 && std::string_view(argv[2]) == "--audio";
    if (audio_test) {
      std::string prompt = JoinArguments(argc, argv, 4);
      if (prompt.empty()) {
        prompt = "Transcribe this audio and summarize it in one sentence.";
      }
      GenerateAudio(model_path, argv[3], prompt);
    } else if (context_test) {
      const std::size_t section_count = argc >= 4 ? std::stoull(argv[3]) : 68;
      const std::string document = sample::BuildLongDocument(section_count);
      std::cout << "Sections: " << section_count << "\n";
      std::cout << "Document bytes: " << document.size() << "\n";
      const std::string output = Generate(model_path, document, kContextTestMaxOutputTokens);
      std::cout << "Anchor labels: " << sample::CountAnchorLabels(output) << "/5\n";
    } else {
      std::string prompt = JoinArguments(argc, argv, 2);
      if (prompt.empty()) {
        prompt = "Explain why local AI inference is useful in two sentences.";
      }
      Generate(model_path, prompt, kDefaultMaxOutputTokens);
    }
  } catch (const std::exception& error) {
    std::cerr << "ORT GenAI error: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
