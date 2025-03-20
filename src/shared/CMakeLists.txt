# Collect miner module sources
set(SHARED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/uint256.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bignum.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sha.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sha.cpp
    PARENT_SCOPE
)