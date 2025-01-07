add_library(debugger debugger.cc)
# target_sources(debugger
#     PRIVATE
#         emumanager.cpp
# )

target_include_directories(debugger
        PRIVATE
            ${CMAKE_SOURCE_DIR}/emulators
    )
