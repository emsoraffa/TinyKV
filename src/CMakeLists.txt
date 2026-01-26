# --- The Client ---
add_executable(tinykv_client
    client/main.cpp
    client/Client.cpp
)
# We only link the proto lib (it carries the gRPC dependencies now)
target_link_libraries(tinykv_client PRIVATE tinykv_proto_lib)

# --- The Server ---
add_executable(tinykv_server
    server/Server.cpp
)
target_link_libraries(tinykv_server PRIVATE tinykv_proto_lib)
