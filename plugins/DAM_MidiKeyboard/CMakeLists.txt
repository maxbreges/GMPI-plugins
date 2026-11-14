cmake_minimum_required(VERSION 3.30)

project(DAM_MidiKeyboard)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
FORMATS_LIST GMPI
	SOURCE_FILES
	  DAM_MidiKeyboard.cpp
	  DAM_MidiKeyboardGui.cpp
)

cmake_policy(SET CMP0169 OLD)
