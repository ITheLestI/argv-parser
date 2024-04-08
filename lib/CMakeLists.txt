add_library(argparser ArgParser.cc ArgParser.h)
add_library(argument_types ArgumentTypes.cc ArgumentTypes.h)
target_link_libraries(argparser PRIVATE argument_types)
