include(CMTest)

cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_network)

if(NOT Boost_UNIT_TEST_FRAMEWORK_FOUND)
    cm_find_package(Boost REQUIRED COMPONENTS system unit_test_framework)
endif()

cm_test_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}

                       ${CMAKE_WORKSPACE_NAME}::core

                       ${Boost_LIBRARIES})

macro(define_network_test name)
    cm_test(NAME network_${name}_test SOURCES ${name}.cpp)

    target_include_directories(network_${name}_test PRIVATE
                               "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                               "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"

                               ${Boost_INCLUDE_DIRS})

    set_target_properties(network_${name}_test PROPERTIES
                          CXX_STANDARD 17
                          CXX_STANDARD_REQUIRED TRUE)

    get_target_property(target_type Boost::unit_test_framework TYPE)
    if(target_type STREQUAL "SHARED_LIB")
        target_compile_definitions(network_${name}_test PRIVATE BOOST_TEST_DYN_LINK)
    elseif(target_type STREQUAL "STATIC_LIB")

    endif()
endmacro()

list(APPEND TESTS_NAMES
     accept_socket
     application
     convert_ip_endpoint
     datagram_socket
     datagram_transport
     doorman
     endpoint_manager
     header
     ip
     message_queue
     multiplexer
     network_socket
     ping_pong
     pipe_socket
     socket
     socket_guard
     stream_application
     stream_socket
     stream_transport
     string_application
     tcp_sockets
     transport_worker
     transport_worker_dispatcher
     udp_datagram_socket
     worker)

foreach(TEST_NAME ${TESTS_NAMES})
    define_network_test(${TEST_NAME})
endforeach()
