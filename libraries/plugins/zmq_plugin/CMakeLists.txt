file(GLOB HEADERS "include/graphene/zmq_plugin/*.hpp")

## load in pkg-config support
find_package(PkgConfig REQUIRED)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ REQUIRED libzmq)

## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
  NAMES zmq.hpp
  PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
  )

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
  NAMES zmq
  PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
  )

message(STATUS "[Additional Plugin] EOSIO ZeroMQ plugin enabled")

add_library( graphene_zmq_plugin 
             zmq_api.cpp
             zmq_plugin.cpp
           )

target_link_libraries( graphene_zmq_plugin graphene_chain graphene_app ${ZeroMQ_LIBRARY} )
target_include_directories( graphene_zmq_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )


install( TARGETS
   graphene_zmq_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/zmq_plugin" )

