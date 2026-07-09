cmake_minimum_required(VERSION 3.30)

project(Gain_with_resource_xml)

# HAS_XML adds ${PROJECT_NAME}.xml to the project, and on Windows compiles
# ${PROJECT_NAME}.rc which embeds the XML as a GMPXML resource (the legacy
# SDK3-style metadata technique).
# GMPI format only: the VST3/CLAP/AU wrappers read metadata solely from the
# factory (Register<>::withXml), they don't read the GMPXML resource.
gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    HAS_XML
    FORMATS_LIST GMPI
    SOURCE_FILES
        ${PROJECT_NAME}.cpp
        resource.h
 )
