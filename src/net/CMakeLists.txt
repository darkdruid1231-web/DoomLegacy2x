set(SRC_FILES
n_interface.cpp
n_connection.cpp
sv_main.cpp
sv_cmds.cpp
cl_main.cpp
EnetNetworkAdapter.cpp
)

add_library(net STATIC ${SRC_FILES})

target_include_directories(net PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
