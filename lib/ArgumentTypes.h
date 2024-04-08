#pragma once
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace ArgumentParser {

enum class ErrorStatus { kNoErrors, kTooFewArguments, kParsingError };

struct ArgumentMetadata {
  std::string name;
  char short_name = '\0';
  std::string description;

  bool has_default = false;
  uint64_t minimum_args = 1;
  bool is_stored_outside = false;
  bool is_positional = false;
  bool is_multivalue = false;
  bool is_bitwise = false;

  ErrorStatus error_status = ErrorStatus::kNoErrors;
};

class BaseArgument {
 public:
  virtual const ArgumentMetadata& GetMetadata() const = 0;
  virtual std::string GetTypeNameString() const = 0;
  virtual std::string GetDefaultValueString() const = 0;
  virtual std::vector<size_t> ParseValuesFromString(
      const std::string& first_value, const std::vector<std::string>& argv,
      size_t index) = 0;
  virtual bool IsCorrect() = 0;
  virtual ~BaseArgument() = default;
};

template <typename T>
class ExactArgument : public BaseArgument {
  ArgumentMetadata metadata_;
  T* value_;
  std::vector<T>* multi_values_;
  uint64_t args_count;

 public:
  ExactArgument(const char short_name, const std::string& name,
                std::string& description) {
    metadata_.short_name = short_name;
    metadata_.name = name;
    metadata_.description = description;
    multi_values_ = nullptr;
    args_count = 0;
    metadata_.is_bitwise = std::is_same<bool, T>::value;
    if (metadata_.is_bitwise) {
      args_count = 1;
      value_ = new T{};
      *value_ = false;
    } else {
      value_ = new T{};
    }
  }
  ExactArgument(const std::string& name, std::string& description) {
    metadata_.name = name;
    metadata_.description = description;
    multi_values_ = nullptr;
    args_count = 0;
    metadata_.is_bitwise = std::is_same<bool, T>::value;
    if (metadata_.is_bitwise) {
      args_count = 1;
      value_ = new T{};
      *value_ = false;
    } else {
      value_ = new T{};
    }
  }
  const ArgumentMetadata& GetMetadata() const override {
    return metadata_;
  }

  std::optional<T> ParseSingleValue(const std::string_view& value);

  std::string GetTypeNameString() const override;  // maybe typeid

  std::string GetDefaultValueString() const override {
    std::ostringstream os;
    if (metadata_.has_default) {
      os << *value_;
    }
    return os.str();
  }

  std::vector<size_t> ParseValuesFromString(
      const std::string& first_value, const std::vector<std::string>& argv,
      size_t index) override {
    std::optional<T> val = ParseSingleValue(first_value);
    if (!val) {
      metadata_.error_status = ErrorStatus::kParsingError;
      return {};
    }
    if (!metadata_.is_multivalue && val) {
      *value_ = val.value();
      args_count = 1;
      return std::vector{index};
    }
    std::vector<size_t> used_indices;
    used_indices.push_back(index);
    multi_values_->push_back(val.value());
    ++args_count;
    ++index;
    while (index < argv.size() && argv[index][0] != '-') {
      val = ParseSingleValue(argv[index]);
      if (val) {
        multi_values_->push_back(val.value());
        used_indices.push_back(index);
        ++args_count;
        ++index;
      } else {
        return used_indices;
      }
    }
    return used_indices;
  }

  bool IsCorrect() override {
    if (args_count < metadata_.minimum_args) {
      metadata_.error_status = ErrorStatus::kTooFewArguments;
    }
    if (metadata_.error_status != ErrorStatus::kNoErrors) {
      return false;
    }
    return true;
  }

  ExactArgument& Default(const T& default_value) {
    metadata_.has_default = true;
    args_count = 1;
    if (metadata_.is_multivalue) {
      (*multi_values_)[0] = default_value;
    } else {
      *value_ = default_value;
    }
    return *this;
  }
  ExactArgument& StoreValue(T& value) {
    metadata_.is_stored_outside = true;
    value_ = &value;
    return *this;
  }
  ExactArgument& StoreValues(std::vector<T>& values) {
    metadata_.is_stored_outside = true;
    delete multi_values_;
    multi_values_ = &values;
    return *this;
  }
  ExactArgument& MultiValue(size_t minimum_args = 1) {
    if (metadata_.is_multivalue) {
      return *this;
    }
    metadata_.is_multivalue = true;
    metadata_.minimum_args = minimum_args;
    multi_values_ = new std::vector<T>;
    delete value_;
    value_ = nullptr;
    return *this;
  }
  ExactArgument& Positional() {
    metadata_.is_positional = true;
    return *this;
  }

  ~ExactArgument() override {
    if (!metadata_.is_stored_outside) {
      if (metadata_.is_multivalue) {
        delete multi_values_;
      } else {
        delete value_;
      }
    }
  }
  T GetValue(size_t index = 0) {
    if (metadata_.is_multivalue) {
      return multi_values_->at(index);
    }
    return *value_;
  }
  ExactArgument(const ExactArgument&) = delete;
  ExactArgument& operator=(const ExactArgument&) = delete;
};

}  // namespace ArgumentParser
