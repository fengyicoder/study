cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(chapter1 LANGUAGES C)
add_executable(hserver hello_server.c)
add_executable(hclient hello_client.c)