cmake_minimum_required(VERSION 3.15)
project(assign2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
add_executable(assign2
        subprocess-test.cc
        subprocess.cc
        subprocess.h
        subprocess-exception.h)

#add_executable(assign2
#        pipeline-test.c
#        pipeline.c
#        pipeline.h
#        simple-test1.cc
#        simple-test2.cc
#        simple-test3.cc
#        simple-test4.cc
#        simple-test5.cc
#        subprocess-exception.h
#        subprocess-test.cc
#        subprocess.h
#        trace-error-constants-test.cc
#        trace-error-constants.cc
#        trace-error-constants.h
#        trace-exception.h
#        trace-options.cc
#        trace-options.h
#        trace-system-calls-test.cc
#        trace-system-calls.cc
#        trace-system-calls.h
#        unistd_64.h pipeline.c)
