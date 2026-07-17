cmake_minimum_required(VERSION 3.30)

project(TextEntry16)

# Assuming se_sdk_folder is defined somewhere before this or passed as a parameter
# Example:
# set(se_sdk_folder "/path/to/sdk")

# Include the shared folder for headers
target_include_directories(${PROJECT_NAME} PRIVATE
  ${se_sdk_folder}/shared
)

# Add only the source file(s), not headers
gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES
      TextEntry16Gui.cpp
      ${se_sdk_folder}/shared/it_enum_list.cpp
)


