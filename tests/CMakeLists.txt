add_executable(ra_common_tests
    test_main.cpp
    test_encoding.cpp
    test_util.cpp
    test_crypto.cpp
    test_identity.cpp
    test_network.cpp
    test_routing.cpp
    test_messaging.cpp
    test_service.cpp
    test_task_runner.cpp
    test_content.cpp
    test_envelope.cpp
    test_config.cpp
)
target_link_libraries(ra_common_tests PRIVATE ra_common)
find_package(Threads REQUIRED)
target_link_libraries(ra_common_tests PRIVATE Threads::Threads)

include(CTest)
add_test(NAME ra_common_tests COMMAND ra_common_tests)
