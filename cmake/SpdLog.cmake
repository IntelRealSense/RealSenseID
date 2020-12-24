add_library(spdlog_iterface INTERFACE)
target_include_directories(spdlog_iterface INTERFACE "${THIRD_PARTY_DIRECTORY}/spdlog_1_8_0/include")
add_library(spdlog::spdlog ALIAS spdlog_iterface)
