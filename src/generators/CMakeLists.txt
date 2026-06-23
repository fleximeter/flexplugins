# Create the project library
add_library(generators MODULE generators.cpp)
add_library(arrayHeap STATIC arrayheap.cpp)
add_library(loopPhasor STATIC loopPhasor.cpp)
add_library(impulseDropout STATIC impulseDropout.cpp)
add_library(impulseJitter STATIC impulseJitter.cpp)
set_target_properties(arrayHeap PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(loopPhasor PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(impulseDropout PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(impulseJitter PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(generators PRIVATE
    loopPhasor
    impulseDropout
    impulseJitter
)
target_link_libraries(impulseJitter PRIVATE arrayHeap)

if(SUPERNOVA)
    add_library(generators_supernova MODULE generators.cpp)
    set_property(TARGET generators_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
    target_link_libraries(generators_supernova PRIVATE 
        loopPhasor
        impulseDropout
        impulseJitter
    )
endif()
