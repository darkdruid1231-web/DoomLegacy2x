# CMake generated Testfile for 
# Source directory: /home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk
# Build directory: /home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(fixed_t_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_fixed_t")
set_tests_properties(fixed_t_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;418;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(save_load_unit_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_save_load_unit")
set_tests_properties(save_load_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;430;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(serialization_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_serialization")
set_tests_properties(serialization_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;436;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(actor_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_actor")
set_tests_properties(actor_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;447;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(component_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_components")
set_tests_properties(component_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;453;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(console_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_console")
set_tests_properties(console_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;458;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(vector_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_vector")
set_tests_properties(vector_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;463;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(demos_integration_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_demos_integration")
set_tests_properties(demos_integration_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;469;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(network_integration_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_network_integration")
set_tests_properties(network_integration_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;475;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
add_test(parity_tests "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/build/test_parity")
set_tests_properties(parity_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;481;add_test;/home/geoffrey/openclaw/workspace/DoomLegacy/legacy/trunk/CMakeLists.txt;0;")
subdirs("engine")
subdirs("audio")
subdirs("video")
subdirs("util")
subdirs("net")
subdirs("interface")
subdirs("grammars")
subdirs("_deps/catch2-build")
