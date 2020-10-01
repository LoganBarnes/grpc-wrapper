##########################################################################################
# LTB Utilities
# Copyright (c) 2020 Logan Barnes - All Rights Reserved
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
##########################################################################################
function(ltb_add_library target cxx_standard)
    # Add the library with custom compile flags and link the testing library
    add_library(${target} ${ARGN})
    target_compile_options(${target} PRIVATE ${LTB_COMPILE_FLAGS})
    target_compile_definitions(${target} PRIVATE -DDOCTEST_CONFIG_DISABLE)

    ltb_set_properties(${target} ${cxx_standard})

    if (LTB_BUILD_TESTS) # BUILDING WITH TESTS
        # Create an executable to run the tests
        set(test_target test_${target})
        add_executable(${test_target} ${ARGN})

        target_link_libraries(${test_target} PRIVATE LtbExternal::DoctestWithMain ltb_testing)
        target_compile_options(${test_target} PRIVATE ${LTB_COMPILE_FLAGS})

        add_test(NAME ${target}_tests COMMAND ${test_target})

        if (COMMAND SETUP_TARGET_FOR_COVERAGE)
            # Create a build that generates coverage reports
            setup_target_for_coverage(${test_target}_coverage ${test_target} ${test_target}_coverage)
            apply_coverage_dependencies(${test_target})
        endif ()

        ltb_set_properties(${test_target} ${cxx_standard})
    endif ()
endfunction()

function(ltb_link_libraries target)
    target_link_libraries(${target} ${ARGN})

    if (LTB_BUILD_TESTS)
        set(test_target test_${target})
        target_link_libraries(${test_target} ${ARGN})
    endif ()
endfunction()

function(ltb_include_directories target)
    target_include_directories(${target} ${ARGN})

    if (LTB_BUILD_TESTS)
        set(test_target test_${target})
        target_include_directories(${test_target} ${ARGN})
    endif ()
endfunction()
