cmake_minimum_required(VERSION 3.30)

project(FileDialog16)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_GUI
	SOURCE_FILES
	  FileDialog16Gui.cpp
)
