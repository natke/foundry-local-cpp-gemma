#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace sample {

inline std::string BuildLongDocument(std::size_t section_count) {
  std::string document = "LONG OPERATIONS REVIEW\n";
  document.reserve(section_count * 340);

  for (std::size_t section = 0; section < section_count; ++section) {
    document += "Section " + std::to_string(section) +
                ". The regional operations team reviewed inventory levels, maintenance schedules, energy usage, "
                "customer requests, delivery timing, safety observations, and staffing plans. The report records "
                "ordinary activity and recommends continuing the existing monitoring process while collecting "
                "measurements for the next quarterly review.\n";

    if (section == 10) {
      document += "ANCHOR ALPHA: The emergency generator fuel contract expires on November 18.\n";
    }
    if (section == section_count / 4) {
      document += "ANCHOR BRAVO: Warehouse seven reported a twelve percent reduction in electricity use.\n";
    }
    if (section == section_count / 2) {
      document += "ANCHOR CHARLIE: The northern delivery route will move from Tuesday to Thursday.\n";
    }
    if (section == (section_count * 3) / 4) {
      document += "ANCHOR DELTA: Customer satisfaction reached ninety four percent in the August survey.\n";
    }
    if (section + 11 == section_count) {
      document +=
          "ANCHOR ECHO: The final safety drill is scheduled for December third at nine in the morning.\n";
    }
  }

  document +=
      "\nSummarize this document in five bullets. Each bullet must include one ANCHOR label and its exact fact.\n";
  return document;
}

inline std::size_t CountAnchorLabels(std::string_view output) {
  constexpr std::string_view anchors[] = {
      "ALPHA",
      "BRAVO",
      "CHARLIE",
      "DELTA",
      "ECHO",
  };

  std::size_t recalled = 0;
  for (const auto anchor : anchors) {
    if (output.find(anchor) != std::string_view::npos) {
      ++recalled;
    }
  }
  return recalled;
}

}  // namespace sample
