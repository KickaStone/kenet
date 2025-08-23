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

    include(GoogleTest)
    # 发现用例并附加属性/标签
    gtest_discover_tests(${NAME}
            WORKING_DIRECTORY ${AGT_WORKING_DIRECTORY}
            PROPERTIES
            LABELS "${AGT_TYPE};${AGT_LABELS}"       # 例如 unit;net;logger
            TIMEOUT 30
    )

    # 标记可并行占用的 CPU 数（可选）
#    set_tests_properties(${NAME} PROPERTIES PROCESSORS 1)
endfunction()

# 按目录组织测试
add_subdirectory(bufferTest)
add_subdirectory(gtestdemo)
