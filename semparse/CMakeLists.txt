#cmake_minimum_required(VERSION 2.8)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../decoder)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../utils)

PROJECT(semparse CXX)

set(semparse_STAT_SRCS
    src/extractor.h
    src/extractor.cc
    src/functionalizer.h
    src/functionalizer.cc
    src/linearizer.h
    src/linearizer.cc
    src/nl_mrl_pair.h
    src/porter2_stemmer.cc
    src/porter2_stemmer.h
    src/read_write.h)

add_library(semparse STATIC ${semparse_STAT_SRCS})

set(extract_data_SRCS src/extract_data.cc)
add_executable(extract_data ${extract_data_SRCS})
target_link_libraries(extract_data semparse ${Boost_LIBRARIES})

set(decode_test_SRCS src/decode_test.cc)
add_executable(decode_test ${decode_test_SRCS})
target_link_libraries(decode_test semparse ${Boost_LIBRARIES} libcdec utils ${ZLIB_LIBRARIES} klm klm_util klm_util_double ${BZIP2_LIBRARIES} ${LIBLZMA_LIBRARIES} ksearch mteval ${CMAKE_DL_LIBS})

set(eval_SRCS src/eval.cc)
add_executable(eval ${eval_SRCS})
target_link_libraries(eval semparse ${Boost_LIBRARIES})

