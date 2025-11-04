cmake_minimum_required(VERSION 3.30)
project(sha512_proof_of_work LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

add_executable(sha512_proof_of_work main.cpp)


target_include_directories(sha512_proof_of_work PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(sha512_proof_of_work PRIVATE OpenSSL::SSL OpenSSL::Crypto)


set_target_properties(sha512_proof_of_work PROPERTIES BUILD_RPATH "${OPENSSL_LIBRARIES}")