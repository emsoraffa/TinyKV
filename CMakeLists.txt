cmake_minimum_required(VERSION 3.15)
project(TinyKV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. DEPENDENCIES (Linux/Docker) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
find_program(PROTOC_EXECUTABLE protoc)

# --- 2. PROTOBUF GENERATION HELPER ---
function(generate_proto_sources PROTO_FILE OUT_H OUT_CC)
  get_filename_component(PROTO_ABS "${PROTO_FILE}" ABSOLUTE)
  get_filename_component(PROTO_DIR "${PROTO_ABS}" DIRECTORY)
  get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)

  set(_H "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
  set(_CC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
  set(_GRPC_H "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
  set(_GRPC_CC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")

  add_custom_command(
        OUTPUT "${_H}" "${_CC}" "${_GRPC_H}" "${_GRPC_CC}"
        COMMAND ${PROTOC_EXECUTABLE}
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
             --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
             -I ${PROTO_DIR}
             ${PROTO_ABS}
        DEPENDS "${PROTO_ABS}"
        COMMENT "Generating gRPC sources for ${PROTO_NAME}"
        VERBATIM)

  set(${OUT_H} "${_H}" "${_GRPC_H}" PARENT_SCOPE)
  set(${OUT_CC} "${_CC}" "${_GRPC_CC}" PARENT_SCOPE)
endfunction()

# --- 3. CREATE PROTO LIBRARY ---
# Generate files into the build directory
generate_proto_sources("protos/tinykv.proto" PROTO_HEADERS PROTO_SOURCES)

# Define the library once here
add_library(tinykv_proto_lib ${PROTO_HEADERS} ${PROTO_SOURCES})

# Link PkgConfig libraries
target_link_libraries(tinykv_proto_lib PUBLIC ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

# PUBLIC allows sub-directories to see the generated headers
target_include_directories(tinykv_proto_lib PUBLIC
    ${GRPC_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# --- 4. SUBDIRECTORIES ---
add_subdirectory(src)
