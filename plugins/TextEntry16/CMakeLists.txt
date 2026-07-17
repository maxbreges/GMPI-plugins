cmake_minimum_required(VERSION 3.30)

project(TextEntry16)

set(SOURCE_FILES
    TextEntry16Gui.cpp
    it_enum_list.cpp
)

gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES ${SOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


#https://github.com/JeffMcClintock/SynthEditLib/tree/main/modules