# Add the RubberBand source
CPMAddPackage(NAME RUBBERBAND GITHUB_REPOSITORY breakfastquay/rubberband GIT_TAG "v4.0.0")
add_library(rubberband_lib STATIC ${RUBBERBAND_SOURCE_DIR}/single/RubberBandSingle.cpp)
if(APPLE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    target_link_libraries(rubberband_lib ${ACCELERATE_FRAMEWORK})
endif()
include_directories(${RUBBERBAND_SOURCE_DIR})

if(WIN32)
    target_compile_definitions(rubberband_lib PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Create the project library
add_library(rubberband MODULE rubberband.cpp)
add_library(rubberBandPS STATIC rubberBandPS.cpp)
add_library(rubberBandStretcher STATIC rubberBandStretcher.cpp)
add_library(rubberBandStretcherBuf STATIC rubberBandStretcherBuf.cpp)
set_target_properties(rubberband_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rubberBandPS PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rubberBandStretcher PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rubberBandStretcherBuf PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(rubberband PRIVATE 
    rubberBandPS 
    rubberBandStretcher
    rubberBandStretcherBuf
)
target_link_libraries(rubberBandPS PRIVATE rubberband_lib)
target_link_libraries(rubberBandStretcher PRIVATE rubberband_lib)
target_link_libraries(rubberBandStretcherBuf PRIVATE rubberband_lib)

if(SUPERNOVA)
    add_library(rubberband_supernova MODULE rubberband.cpp)
    target_link_libraries(rubberband_supernova PRIVATE 
        rubberband_lib 
        rubberBandPS 
        rubberBandStretcher
    )
    set_property(TARGET rubberband_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
endif()
