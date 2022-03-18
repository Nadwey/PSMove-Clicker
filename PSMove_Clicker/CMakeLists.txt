cmake_minimum_required(VERSION 3.3)

# Project
project(PSMove_Clicker)
find_package(xtd REQUIRED)
add_sources(
  src/main.cpp
)
target_type(GUI_APPLICATION)

# Install
install_component()
