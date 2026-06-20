# Create the project library
add_library(pv MODULE pv.cpp)
add_library(pvCFreeze STATIC pvCFreeze.cpp)
add_library(pvOther STATIC pvOther.cpp)
add_library(peakFinder STATIC peakFinder.cpp)
add_library(pvStretch STATIC pvStretch.cpp)
set_target_properties(pvCFreeze PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(pvOther PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(peakFinder PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(pvStretch PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pvStretch PRIVATE peakFinder)
target_link_libraries(pv PRIVATE pvCFreeze pvOther pvStretch)

if(SUPERNOVA)
    add_library(pv_supernova MODULE pv.cpp)
    target_link_libraries(pv_supernova PRIVATE pvCFreeze pvOther pvStretch)
    set_property(TARGET pv_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
endif()
