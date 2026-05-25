# Add the RubberBand source
CPMAddPackage(NAME RUBBERBAND GITHUB_REPOSITORY breakfastquay/rubberband GIT_TAG "v4.0.0")
add_library(rubberband_lib STATIC ${RUBBERBAND_SOURCE_DIR}/single/RubberBandSingle.cpp)
include_directories(${RUBBERBAND_SOURCE_DIR})

# Create the project library
add_library(rubberband MODULE rubberband.cpp)
target_link_libraries(rubberband PRIVATE rubberband_lib)
set_target_properties(rubberband_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(SUPERNOVA)
    add_library(rubberband_supernova MODULE rubberband.cpp)
    target_link_libraries(rubberband_supernova PRIVATE rubberband_lib)
    set_target_properties(rubberband_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET rubberband_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
endif()
