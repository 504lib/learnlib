# CMake generated Testfile for 
# Source directory: C:/learnlib/public_lib/Finite_State_Machine
# Build directory: C:/learnlib/public_lib/Finite_State_Machine/build-ninja
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(fsm_test_start_entry_action "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "1")
set_tests_properties(fsm_test_start_entry_action PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;39;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_nullable_entry_exit "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "2")
set_tests_properties(fsm_test_nullable_entry_exit PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;40;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_transition_order "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "4")
set_tests_properties(fsm_test_transition_order PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;41;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_queue_single_slot "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "8")
set_tests_properties(fsm_test_queue_single_slot PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;42;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_data_package_fifo "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "16")
set_tests_properties(fsm_test_data_package_fifo PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;43;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_invalid_inputs "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "32")
set_tests_properties(fsm_test_invalid_inputs PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;44;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
add_test(fsm_test_all "C:/learnlib/public_lib/Finite_State_Machine/build-ninja/finite_state_machine_test.exe" "all")
set_tests_properties(fsm_test_all PROPERTIES  _BACKTRACE_TRIPLES "C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;45;add_test;C:/learnlib/public_lib/Finite_State_Machine/CMakeLists.txt;0;")
