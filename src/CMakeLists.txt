# Collect the source files in this directory
set(SRC_FILES
    base.cpp
    base.h
    globals.cpp
    globals.h
)

# Add subdirectories
add_subdirectory(globalsources)
add_subdirectory(miner)
add_subdirectory(database)
add_subdirectory(script)
add_subdirectory(shared)
add_subdirectory(network)


find_package(OpenSSL REQUIRED)
find_package(RocksDB CONFIG REQUIRED)

# Define the executable and its sources
add_executable(base 
    ${SRC_FILES}
    ${MINER_SOURCES}
    ${DB_SOURCES}
    ${SCRIPT_SOURCES}
    ${SHARED_SOURCES}
    ${NET_SOURCES}
    ${GlOBALS_SOURCES}
)
target_link_libraries(base PRIVATE OpenSSL::SSL)
target_link_libraries(base PRIVATE OpenSSL::Crypto)

target_link_libraries(base PRIVATE RocksDB::rocksdb)
# Include directories
target_include_directories(base PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/miner
    ${CMAKE_CURRENT_SOURCE_DIR}/database
    ${CMAKE_CURRENT_SOURCE_DIR}/database/addressdb
    ${CMAKE_CURRENT_SOURCE_DIR}/database/transactiondb
    ${CMAKE_CURRENT_SOURCE_DIR}/database/walletdb
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/script
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/globalsources

)

#target_precompile_headers(base PRIVATE pch.h)




