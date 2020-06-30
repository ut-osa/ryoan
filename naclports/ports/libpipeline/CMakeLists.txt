set(PIPELINEFILES pipeline.c channel.c utils.c fitting_spec.c work_spec.c json.c
    json-builder.c htable.c hash.c)

add_library(pipeline STATIC ${PIPELINEFILES})

set_target_properties(pipeline PROPERTIES
  COMPILE_FLAGS "-DNOT_SGX -g -Wall")
include_directories(${CMAKE_SOURCE_DIR}/include)

install (TARGETS pipeline ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install (DIRECTORY include/pipeline DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
