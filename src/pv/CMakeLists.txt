# Create the project library
add_library(pv MODULE pv.cpp)

if(SUPERNOVA)
    add_library(pv_supernova MODULE pv.cpp)
    set_property(TARGET pv_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
endif()
