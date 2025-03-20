# db/CMakeLists.txt
# Add subdirectories first
add_subdirectory(addressdb)
add_subdirectory(transactiondb)
add_subdirectory(walletdb)

# Collect all DB sources including the main db files
set(DB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/db.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db.h
    ${ADDRESSDB_SOURCES}
    ${TRANSACTIONDB_SOURCES}
    ${WALLETDB_SOURCES}
    PARENT_SCOPE
)