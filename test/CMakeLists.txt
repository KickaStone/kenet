# tests/CMakeLists.txt
function(add_gtest NAME)
    set(options)
    set(oneValueArgs TYPE WORKING_DIRECTORY)       # TYPE=unit|integration|e2e...
    set(multiValueArgs SRCS LIBS ARGS LABELS)      # LABELS 用于 ctest -L
    cmake_parse_arguments(AGT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${NAME} ${AGT_SRCS})
    target_link_libraries(${NAME} PRIVATE GTest::gtest_main ${AGT_LIBS})
    # 测试默认工作目录（便于找测试数据）
    if (NOT AGT_WORKING_DIRECTORY)
        set(AGT_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    # 使用 add_test 而不是 gtest_discover_tests
    add_test(NAME ${NAME} COMMAND ${NAME})
    set_tests_properties(${NAME} PROPERTIES
        WORKING_DIRECTORY ${AGT_WORKING_DIRECTORY}
        LABELS "${AGT_TYPE};${AGT_LABELS}"
        TIMEOUT 30
        # 确保输出可见
        ENVIRONMENT "GTEST_COLOR=1"
    )

endfunction()

# 按目录组织测试
add_subdirectory(bufferTest)
add_subdirectory(unit)
