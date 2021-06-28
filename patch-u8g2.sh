#!/bin/sh

sed -i 's/.*COMPONENT_ADD_INCLUDEDIRS.*/project(u8g2)/' CMakeLists.txt
sed -i 's/.*COMPONENT_SRCS \"\(.*\)\"/add_library(u8g2 \"\1\")\ntarget_sources(u8g2 PUBLIC /' CMakeLists.txt
sed -i '/.*COMPONENT_NAME.*/d' CMakeLists.txt
sed -i 's/.*register_component.*/target_include_directories(u8g2 INTERFACE \$\{CMAKE_CURRENT_SOURCE_DIR\} )\n/' CMakeLists.txt
