#include "ArgumentTypes.h"

#include <charconv>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace ArgumentParser {

template class ExactArgument<std::string>;

template <>
std::optional<std::string> ExactArgument<std::string>::ParseSingleValue(
    const std::string_view& value) {
  return std::string{value.begin(), value.end()};
}

template <>
std::string ExactArgument<std::string>::GetTypeNameString() const {
  return "string";
}

template class ExactArgument<bool>;

template <>
std::optional<bool> ExactArgument<bool>::ParseSingleValue(
    const std::string_view& value) {
  if (value == "1" || value == "true") {
    args_count++;
    return true;
  } else if (value == "0" || value == "false") {
    args_count++;
    return false;
  }
  metadata_.error_status = ErrorStatus::kParsingError;
  return std::nullopt;
}

template <>
std::string ExactArgument<bool>::GetTypeNameString() const {
  return "";
}

template class ExactArgument<int>;

template <>
std::optional<int> ExactArgument<int>::ParseSingleValue(
    const std::string_view& value) {
  int result = 0;
  auto [ptr, err] = std::from_chars(value.begin(), value.end(), result);
  if (err != std::errc() || ptr != value.end()) {
    metadata_.error_status = ErrorStatus::kParsingError;
    return std::nullopt;
  }
  return result;
}

template <>
std::string ExactArgument<int>::GetTypeNameString() const {
  return "int";
}

}  // namespace ArgumentParser
