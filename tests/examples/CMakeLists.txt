# Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(lexy_test_email email.cpp)
target_link_libraries(lexy_test_email PRIVATE lexy_test_base)

add_executable(lexy_test_ip_address ip_address.cpp)
target_link_libraries(lexy_test_ip_address PRIVATE lexy_test_base)

add_executable(lexy_test_json json.cpp)
target_link_libraries(lexy_test_json PRIVATE lexy_test_base)

add_executable(lexy_test_protobuf protobuf.cpp)
target_link_libraries(lexy_test_protobuf PRIVATE lexy_test_base)

add_executable(lexy_test_shell shell.cpp)
target_link_libraries(lexy_test_shell PRIVATE lexy_test_base)

add_executable(lexy_test_xml xml.cpp)
target_link_libraries(lexy_test_xml PRIVATE lexy_test_base)

add_executable(lexy_test_turing turing.cpp)
target_link_libraries(lexy_test_turing PRIVATE lexy_test_base)

