cmake_minimum_required(VERSION 2.8)
project(bp)
set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
if (ARCH EQUAL "mips64")
    set(CMAKE_C_FLAGS "-march=mips64r2 -mabi=64 ${CMAKE_C_FLAGS}")
endif ()
set(SOURCES
        arch/aarch64.h
        arch/mips64.h
        arch/ppc64.h
        arch/s390x.h
        arch/x86_64.h
        bp.c
        os/apple.h
        os/linux.h
        )
add_executable(bp ${SOURCES})
add_custom_target(format COMMAND clang-format -style=file -i ${SOURCES})
add_custom_target(checkpatch COMMAND checkpatch.pl -f ${SOURCES})
