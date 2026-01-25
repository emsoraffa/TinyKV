# src/CMakeLists.txt

# Create the executable named 'TinyKV' from main.cpp
add_executable(tinykv_client
    client/main.cpp
    client/Client.cpp
)
target_link_libraries(tinykv_client grpc++ ...)
