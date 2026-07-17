cmake_minimum_required(VERSION 3.30)

project(TextEntry16)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_GUI
	SOURCE_FILES
	  TextEntry16Gui.cpp
        ${se_shared_folder}/it_enum_list.h
        ${se_shared_folder}/it_enum_list.cpp
)


#https://github.com/JeffMcClintock/SynthEditLib/tree/main/modules