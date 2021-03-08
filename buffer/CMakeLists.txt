project(buffer)

add_definitions(-std=c++17)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/LongAddr ${PROJECT_SOURCE_DIR}/../base)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/LongAddr LONGADDR_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/../base BASE_LIST)

message(STATUS "CODECODECODE" ${SRC_LIST})
message(STATUS "CODECODECODE" ${LONGADDR_LIST})

add_executable(buffer ${SRC_LIST} ${LONGADDR_LIST} ${BASE_LIST})

target_link_libraries(${PROJECT_NAME} pthread)