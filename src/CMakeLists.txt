# Collect the source files in this directory
set(SRC_FILES
    base.cpp
    base.h
    globals.cpp
    globals.h
)

# Add subdirectories
add_subdirectory(miner)
add_subdirectory(database)
add_subdirectory(script)
add_subdirectory(shared)
add_subdirectory(network)


find_package(OpenSSL REQUIRED)

# Define the executable and its sources
add_executable(base 
    ${SRC_FILES}
    ${MINER_SOURCES}
    ${DB_SOURCES}
    ${SHARED_SOURCES}
    ${SCRIPT_SOURCES}
    ${NET_SOURCES}
)
target_link_libraries(base PRIVATE OpenSSL::SSL)
target_link_libraries(base PRIVATE OpenSSL::Crypto)
# Include directories
target_include_directories(base PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/miner
    ${CMAKE_CURRENT_SOURCE_DIR}/database
    ${CMAKE_CURRENT_SOURCE_DIR}/db/addressdb
    ${CMAKE_CURRENT_SOURCE_DIR}/db/transactiondb
    ${CMAKE_CURRENT_SOURCE_DIR}/db/walletdb
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/script
    ${CMAKE_CURRENT_SOURCE_DIR}/net
)

#target_precompile_headers(base PRIVATE pch.h)




