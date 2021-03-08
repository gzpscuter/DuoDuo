project(duoduo)

add_definitions(-std=c++17)

include_directories(.. ../buffer ../base ../buffer/LongAddr ../timer/delay_queue ../timer/thread_pool ../timer)

aux_source_directory(.. SRC_LIST)
aux_source_directory(../buffer BUFFER_LIST)
list(REMOVE_ITEM BUFFER_LIST "../buffer/test.cpp")
aux_source_directory(../buffer/LongAddr LONGADDR_LIST)
aux_source_directory(../base BASE_LIST)
aux_source_directory(../timer TIMER_LIST)
aux_source_directory(../timer/thread_pool THREADPOOL_LIST)
aux_source_directory(../timer/delay_queue DELAYQUEUE_LIST)


message(STATUS "CODECODECODE" ${SRC_LIST})
message(STATUS "CODECODECODE" ${BUFFER_LIST})

#add_executable(${PROJECT_NAME}  testTcpServer.cpp ${SRC_LIST}
#                ${BUFFER_LIST} ${BASE_LIST} ${LONGADDR_LIST} ${TIMER_LIST} ${THREADPOOL_LIST} ${DELAYQUEUE_LIST} )

add_executable(${PROJECT_NAME}  testTimer.cpp ${SRC_LIST}
                ${BUFFER_LIST} ${BASE_LIST} ${LONGADDR_LIST} ${TIMER_LIST} ${THREADPOOL_LIST} ${DELAYQUEUE_LIST} )

target_link_libraries(${PROJECT_NAME} pthread)