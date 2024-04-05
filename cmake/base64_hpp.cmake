add_library(mvorbrodt_base64 INTERFACE)
add_library(mvorbrodt_base64::headeronly ALIAS mvorbrodt_base64)
target_include_directories(mvorbrodt_base64 INTERFACE "${THIRD_PARTY_DIRECTORY}/mvorbrodt_base64")

