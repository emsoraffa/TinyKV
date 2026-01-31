# --- CLIENT EXECUTABLE ---
add_executable(tinykv_client
    client/main.cpp
    client/Client.cpp
    Utils.cpp
)
# Link to the library defined in the Root CMake
target_link_libraries(tinykv_client PRIVATE tinykv_proto_lib)
target_include_directories(tinykv_client PRIVATE client .)


# --- SERVER EXECUTABLE ---
add_executable(tinykv_server
    server/Server.cpp
    server/HashRing.cpp
    client/Client.cpp
    Utils.cpp
)
target_link_libraries(tinykv_server PRIVATE tinykv_proto_lib)

# Include "client" folder to find Client.h
# Include "." (current src dir) to find Utils.h
target_include_directories(tinykv_server PRIVATE client .)
