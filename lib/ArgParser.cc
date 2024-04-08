#include "ArgParser.h"

#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace ArgumentParser {

void SetUsedPosition(std::vector<bool>& used_positions,
                     const std::vector<size_t>& positions) {
  for (size_t i = 0; i < positions.size(); i++) {
    if (used_positions[positions[i]]) {
      std::cerr << "Reused argument at position " << positions[i] << std::endl;
    }
    used_positions[positions[i]] = true;
  }
}

std::vector<size_t> ArgParser::SetValuesForParameter(
    const std::string_view& name, const std::string& value,
    const std::vector<std::string>& argv, size_t i) {
  std::optional<size_t> j_opt = FindArgument(name);
  if (!j_opt) {
    std::cerr << "Incorrect parameter name: " << name << std::endl;
    return {};
  }
  size_t j = j_opt.value();
  if (i >= argv.size()) {
    std::cerr << "Incorrect value for parameter " << name << std::endl;
    return {};
  }
  std::vector<size_t> positions =
      arguments_[j]->ParseValuesFromString(value, argv, i);
  if (positions.empty()) {
    std::cerr << "Incorrect value for parameter " << name << std::endl;
    return {};
  }
  return positions;
}

bool ArgParser::Parse(const std::vector<std::string>& args) {
  bool is_parsing_ok = true;
  bool is_help_requested = false;
  std::vector<bool> used_positions(args.size(), false);
  used_positions[0] = true;

  for (int i = 1; i < args.size(); i++) {
    if (used_positions[i]) {
      continue;
    }

    // If starts with --
    if (args[i].compare(0, 2, "--") == 0) {
      size_t delimiter_pos = args[i].find('=');
      if (delimiter_pos != std::string::npos) {
        std::string_view name(args[i].data() + 2, delimiter_pos - 2);
        if (delimiter_pos ==
            args[i].length() - 1) {  // if argument ends after delimiter
          std::cerr << "Incorrect value for parameter: " << std::endl;
          return false;
        }
        std::string value = args[i].substr(delimiter_pos + 1);
        const std::vector<size_t> pos =
            SetValuesForParameter(name, value, args, i);
        is_parsing_ok &= pos.size() > 0;
        SetUsedPosition(used_positions, pos);

      } else {
        std::string_view name(args[i].data() + 2);
        std::optional<size_t> j_opt = FindArgument(name);
        if (!j_opt) {
          std::cerr << "Incorrect parameter name: " << name << std::endl;
          return false;
        }
        size_t j = j_opt.value();
        const ArgumentMetadata& meta = arguments_[j]->GetMetadata();
        bool is_single_flag = !meta.is_multivalue && meta.is_bitwise;
        if (meta.name == name && is_single_flag) {
          arguments_[j]->ParseValuesFromString("true", args, i);
          used_positions[i] = true;
          continue;
        }
        used_positions[i] = true;
        const std::vector<size_t> pos =
            SetValuesForParameter(name, args[i + 1], args, i + 1);
        is_parsing_ok &= pos.size() > 0;
        SetUsedPosition(used_positions, pos);
      }
      continue;
    }

    // If starts with -
    if (args[i][0] == '-') {
      std::string_view names;
      std::string value;
      size_t first_value_index_offset = 0;
      size_t delimiter_pos = args[i].find("=");
      if (delimiter_pos != std::string::npos) {
        names = std::string_view(args[i].data() + 1, delimiter_pos - 1);
        value = args[i].data() + delimiter_pos + 1;
      } else {
        names = args[i].data() + 1;
        if (i + 1 < args.size()) {
          value = args[i + 1];
          first_value_index_offset = 1;
        }
        ++first_value_index_offset;
      }
      for (char c : names) {
        std::string& name = short_to_long_names_.at(c);
        std::optional<size_t> j_opt = FindArgument(name);
        if (!j_opt) {
          std::cerr << "Incorrect parameter name: " << name << std::endl;
          return false;
        }
        size_t j = j_opt.value();
        if (arguments_[j]->GetMetadata().is_bitwise) {
          arguments_[j]->ParseValuesFromString("true", args, i);
        } else {
          const std::vector<size_t> positions =
              arguments_[j]->ParseValuesFromString(
                  value, args, i + first_value_index_offset);
          if (positions.empty()) {
            std::cerr << "Incorrect value for parameter " << name << std::endl;
            return false;
          }
          SetUsedPosition(used_positions, positions);
        }
      }
      used_positions[i] = true;
      continue;
    }
  }

  // parse positional arguments
  for (size_t k = 0; k < used_positions.size(); ++k) {
    if (!used_positions[k]) {
      for (size_t j = 0; j < arguments_.size(); ++j) {
        if (arguments_[j]->GetMetadata().is_positional) {
          const std::vector<size_t> positions =
              arguments_[j]->ParseValuesFromString(args[k], args, k);
          if (positions.empty()) {
            std::cerr << "Incorrect value for parameter "
                      << arguments_[j]->GetMetadata().name << std::endl;
            return false;
          }
          SetUsedPosition(used_positions, positions);
        }
      }
    }
  }

  for (bool b : used_positions) {
    if (!b) {
      is_parsing_ok = false;
      break;
    }
  }
  for (size_t i = 0; i < arguments_.size(); ++i) {
    is_parsing_ok &= arguments_[i]->IsCorrect();
  }
  if (Help()) {
    return true;
  }
  return is_parsing_ok;
}

bool ArgParser::Parse(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    args.push_back(argv[i]);
  }
  return ArgParser::Parse(args);
}

std::optional<size_t> ArgParser::FindArgument(
    const std::string_view& name) const {
  for (size_t i = 0; i < arguments_.size(); i++) {
    if (arguments_[i]->GetMetadata().name == name) {
      return i;
    }
  }
  return std::nullopt;
}

ExactArgument<std::string>& ArgParser::AddStringArgument(
    const char short_name, const std::string& name, std::string description) {
  ExactArgument<std::string>* arg =
      new ExactArgument<std::string>(short_name, name, description);
  short_to_long_names_.insert(std::make_pair(short_name, name));
  arguments_.push_back(arg);
  return *arg;
}
ExactArgument<std::string>& ArgParser::AddStringArgument(
    const std::string& name, std::string description) {
  ExactArgument<std::string>* arg =
      new ExactArgument<std::string>(name, description);
  arguments_.push_back(arg);
  return *arg;
}

std::string ArgParser::GetStringValue(const std::string& name,
                                      size_t index) const {
  std::optional<size_t> i_opt = FindArgument(name);
  if (!i_opt) {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
  ExactArgument<std::string>* arg =
      dynamic_cast<ExactArgument<std::string>*>(arguments_.at(i_opt.value()));
  if (arg) {
    return arg->GetValue(index);
  } else {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
}

ExactArgument<int>& ArgParser::AddIntArgument(const char short_name,
                                              const std::string& name,
                                              std::string description) {
  ExactArgument<int>* arg =
      new ExactArgument<int>(short_name, name, description);
  short_to_long_names_.insert(std::make_pair(short_name, name));
  arguments_.push_back(arg);
  return *arg;
}

ExactArgument<int>& ArgParser::AddIntArgument(const std::string& name,
                                              std::string description) {
  ExactArgument<int>* arg = new ExactArgument<int>(name, description);
  arguments_.push_back(arg);
  return *arg;
}

int ArgParser::GetIntValue(const std::string& name, size_t index) const {
  std::optional<size_t> i_opt = FindArgument(name);
  if (!i_opt) {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
  ExactArgument<int>* arg =
      dynamic_cast<ExactArgument<int>*>(arguments_.at(i_opt.value()));
  if (arg) {
    return arg->GetValue(index);
  } else {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
}

ExactArgument<bool>& ArgParser::AddFlag(const char short_name,
                                        const std::string& name,
                                        std::string description) {
  ExactArgument<bool>* arg =
      new ExactArgument<bool>(short_name, name, description);
  short_to_long_names_.insert(std::make_pair(short_name, name));
  arguments_.push_back(arg);
  return *arg;
}

ExactArgument<bool>& ArgParser::AddFlag(const std::string& name,
                                        std::string description) {
  ExactArgument<bool>* arg = new ExactArgument<bool>(name, description);
  arguments_.push_back(arg);
  return *arg;
}

bool ArgParser::GetFlag(std::string name, size_t index) const {
  std::optional<size_t> i_opt = FindArgument(name);
  if (!i_opt) {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
  ExactArgument<bool>* arg =
      dynamic_cast<ExactArgument<bool>*>(arguments_.at(i_opt.value()));
  if (arg) {
    return arg->GetValue(index);
  } else {
    throw std::runtime_error("Argument with name " + name + " not found");
  }
}

void ArgParser::AddHelp(const std::string& name, std::string description) {
  help_keyword_ = name;
  ExactArgument<bool>* arg = new ExactArgument<bool>(name, description);
  arguments_.push_back(arg);
}

void ArgParser::AddHelp(const char short_name, const std::string& name,
                        std::string description) {
  help_keyword_ = name;
  ExactArgument<bool>* arg =
      new ExactArgument<bool>(short_name, name, description);
  short_to_long_names_.insert(std::make_pair(short_name, name));
  arguments_.push_back(arg);
}

bool ArgParser::Help() const {
  std::optional<size_t> i_opt = FindArgument(help_keyword_);
  if (!i_opt) {
    return false;
  }
  size_t i = i_opt.value();
  ExactArgument<bool>* arg =
      dynamic_cast<ExactArgument<bool>*>(arguments_.at(i));
  if (arg) {
    return arg->GetValue();
  } else {
    std::cerr << "No help message specified" << std::endl;
  }
  return false;
}

const std::string ArgParser::HelpDescription() const {
  std::string help_string;
  help_string += name_ + '\n';
  std::optional<size_t> help_index = FindArgument(help_keyword_);

  if (help_index == std::nullopt) {
    help_string += "No description specified\n";
  } else {
    help_string +=
        arguments_.at(help_index.value())->GetMetadata().description + "\n\n";
  }
  for (size_t i = 0; i < arguments_.size(); i++) {
    if (arguments_[i]->GetMetadata().name == help_keyword_) {
      continue;
    }
    const ArgumentMetadata& metadata = arguments_[i]->GetMetadata();
    std::string arg_info;
    if (metadata.short_name != '\0') {
      arg_info += "-";
      arg_info += metadata.short_name;
      arg_info += ",  ";
    } else {
      arg_info += "     ";
    }
    arg_info += "--" + metadata.name;
    std::string arg_type = arguments_[i]->GetTypeNameString();
    if (!metadata.is_bitwise && arg_type != "") {
      arg_info += "=<" + arg_type + ">";
    }
    arg_info += ",  " + metadata.description;
    if (metadata.is_positional) {
      arg_info += " [positional]";
    }
    if (metadata.is_multivalue) {
      arg_info +=
          " [repeated, min args = " + std::to_string(metadata.minimum_args) +
          "]";
    }
    if (metadata.has_default) {
      arg_info += " [default = " + arguments_[i]->GetDefaultValueString() + "]";
    }
    help_string += arg_info + "\n";
  }
  help_string += "\n";
  const ArgumentMetadata& help_metadata =
      arguments_.at(help_index.value())->GetMetadata();
  if (help_metadata.short_name != '\0') {
    help_string += "-";
    help_string += help_metadata.short_name;
    help_string += ", ";
  } else {
    help_string += "    ";
  }
  help_string += "--" + help_metadata.name + " Display this help and exit\n";
  return help_string;
}

}  // namespace ArgumentParser
