cmake_minimum_required(VERSION 3.22)
project(stm_lcd_tests LANGUAGES C CXX)
enable_testing()
add_compile_options(-Wall -Wextra -Werror -UNDEBUG)
add_subdirectory(.. component)
add_executable(test_contracts test_contracts.c)
target_link_libraries(test_contracts PRIVATE stm_lcd)
add_test(NAME generic_lcd_contracts COMMAND test_contracts)
add_library(headers OBJECT headers.c headers.cpp headers_io.c headers_panel.c headers_touch.c)
target_compile_features(headers PRIVATE c_std_11 cxx_std_17)
target_link_libraries(headers PRIVATE stm_lcd)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../stm_lcd_axs15231b/src/stm_lcd_axs15231b.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../stm_lcd_touch_axs15231b/src/stm_lcd_touch_axs15231b.c")
    # Fault injection affects only chip constructor allocations, never production libraries.
    add_library(allocation_drivers OBJECT
        ../../stm_lcd_axs15231b/src/stm_lcd_axs15231b.c
        ../../stm_lcd_touch_axs15231b/src/stm_lcd_touch_axs15231b.c)
    target_compile_definitions(allocation_drivers PRIVATE calloc=allocation_calloc free=allocation_free)
    target_include_directories(allocation_drivers PRIVATE
        ../../stm_lcd_axs15231b/include ../../stm_lcd_touch_axs15231b/include)
    target_link_libraries(allocation_drivers PRIVATE stm_lcd)
    add_executable(test_driver_allocations test_driver_allocations.c $<TARGET_OBJECTS:allocation_drivers>)
    target_include_directories(test_driver_allocations PRIVATE
        ../../stm_lcd_axs15231b/include ../../stm_lcd_touch_axs15231b/include)
    target_link_libraries(test_driver_allocations PRIVATE stm_lcd)
    add_test(NAME driver_allocation_failures COMMAND test_driver_allocations)
else()
    message(STATUS "AXS constructor integration tests require sibling panel and touch components")
endif()

add_executable(test_framebuffer test_framebuffer.c)
target_link_libraries(test_framebuffer PRIVATE stm_lcd)
add_test(NAME framebuffer_and_register_contracts COMMAND test_framebuffer)
