cmake_minimum_required(VERSION 3.30)

project(TextEntry16)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_GUI
	SOURCE_FILES
	  TextEntry16Gui.cpp
)


#https://github.com/JeffMcClintock/SynthEditLib/tree/main/modules
#adding 	${se_sdk_folder}/it_enum_list.h
#	        ${se_sdk_folder}/it_enum_list.cpp
# did not work so I had to add "it_enum_list.h" and "it_enum_list.cpp" to deps_ sub-directory
# and do #include "./deps_/it_enum_list.h" in the .cpp