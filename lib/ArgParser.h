#pragma once
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "ArgumentTypes.h"

namespace ArgumentParser {

class ArgParser {
  std::string name_;
  std::string help_keyword_;
  std::string help_description_;

  std::vector<BaseArgument*> arguments_;
  std::map<char, std::string> short_to_long_names_;

 public:
  explicit ArgParser(std::string name) : name_(name){};
  bool Parse(const std::vector<std::string>& args);
  bool Parse(int argc, char** argv);

  ExactArgument<std::string>& AddStringArgument(const char short_name,
                                                const std::string& name,
                                                std::string description = "");
  ExactArgument<std::string>& AddStringArgument(const std::string& name,
                                                std::string description = "");
  std::string GetStringValue(const std::string& name, size_t index = 0) const;

  ExactArgument<bool>& AddFlag(const char short_name, const std::string& name,
                               std::string description = "");
  ExactArgument<bool>& AddFlag(const std::string& name,
                               std::string description = "");
  bool GetFlag(std::string name, size_t index = 0) const;

  ExactArgument<int>& AddIntArgument(const char short_name,
                                     const std::string& name,
                                     std::string description = "");
  ExactArgument<int>& AddIntArgument(const std::string& name,
                                     std::string description = "");
  int GetIntValue(const std::string& name, size_t index = 0) const;

  void AddHelp(const std::string& name, std::string description = "");
  void AddHelp(const char short_name, const std::string& name,
               std::string description = "");
  bool Help() const;
  const std::string HelpDescription() const;

 private:
  std::optional<size_t> FindArgument(const std::string_view& name) const;
  std::vector<size_t> SetValuesForParameter(
      const std::string_view& name, const std::string& value,
      const std::vector<std::string>& argv, size_t index);
};

}  // namespace ArgumentParser
