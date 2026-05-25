# Create the project library
add_library(generators MODULE generators.cpp)

if(SUPERNOVA)
    add_library(generators_supernova MODULE generators.cpp)
    set_property(TARGET generators_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
endif()
