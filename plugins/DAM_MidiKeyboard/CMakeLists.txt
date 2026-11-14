cmake_minimum_required(VERSION 3.30)

project(DAM_MidiKeyboard)

build_gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
	SOURCE_FILES
	  DAM_MidiKeyboard.cpp
	  DAM_MidiKeyboardGui.cpp
)
