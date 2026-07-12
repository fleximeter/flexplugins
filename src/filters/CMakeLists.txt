# Create the project library
add_library(filters MODULE filters.cpp)
add_library(fir STATIC fir.cpp)
set_target_properties(fir PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(filters PRIVATE 
    fir
)

if(SUPERNOVA)
    add_library(filters_supernova MODULE filters.cpp)
    set_property(TARGET filters_supernova
                 PROPERTY COMPILE_DEFINITIONS SUPERNOVA)
    target_link_libraries(filters_supernova PRIVATE 
        fir
    )
endif()
