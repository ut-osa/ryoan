set( COMMON_CXXFLAGS "-Wall -Werror -Wextra -fno-threadsafe-statics -fvisibility-inlines-hidden -std=gnu++11 -fno-rtti -fno-exceptions -fno-strict-aliasing -static -fPIC")
set( EXEEXT "" CACHE STRING "program suffix" )

add_executable(svmsgd_train${EXEEXT}
    svmsgd.cpp
    data.cpp
    vectors.cpp
    timer.cpp
    svmsgd_train.cpp
)
target_link_libraries(svmsgd_train${EXEEXT} LINK_PUBLIC ${C++} ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB})
set_target_properties(svmsgd_train${EXEEXT} PROPERTIES COMPILE_FLAGS ${COMMON_CXXFLAGS})

add_executable(svmsgd_test${EXEEXT}
    svmsgd.cpp
    data.cpp
    vectors.cpp
    timer.cpp
    svmsgd_test.cpp
)
target_link_libraries(svmsgd_test${EXEEXT} LINK_PUBLIC ${C++} ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB})
set_target_properties(svmsgd_test${EXEEXT} PROPERTIES COMPILE_FLAGS ${COMMON_CXXFLAGS})

add_executable(svmsgd_classify${EXEEXT}
    svmsgd.cpp
    data.cpp
    vectors.cpp
    svmsgd_classify.cpp
)
target_link_libraries(svmsgd_classify${EXEEXT} LINK_PUBLIC ${C++} ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB})
set_target_properties(svmsgd_classify${EXEEXT} PROPERTIES COMPILE_FLAGS ${COMMON_CXXFLAGS})

add_executable(svmsgd_classify_pipeline${EXEEXT}
    svmsgd.cpp
    vectors.cpp
    svmsgd_classify_pipeline.cpp
)
target_link_libraries(svmsgd_classify_pipeline${EXEEXT} LINK_PUBLIC pipeline crypto m ${C++} ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB}  ${DL_LIB})
set_target_properties(svmsgd_classify_pipeline${EXEEXT} PROPERTIES COMPILE_FLAGS ${COMMON_CXXFLAGS})

add_executable(svmsgd_model_loader_pipeline${EXEEXT}
    svmsgd.cpp
    vectors.cpp
    svmsgd_model_loader_pipeline.cpp
)
target_link_libraries(svmsgd_model_loader_pipeline${EXEEXT} LINK_PUBLIC pipeline crypto m ${C++} ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )
set_target_properties(svmsgd_model_loader_pipeline${EXEEXT} PROPERTIES COMPILE_FLAGS ${COMMON_CXXFLAGS})

add_executable(svmsgd_end_pipeline${EXEEXT}
    svmsgd_end_pipeline.c
)
target_link_libraries(svmsgd_end_pipeline${EXEEXT} LINK_PUBLIC pipeline crypto m ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(svmsgd_end_pipeline${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")

install (TARGETS svmsgd_train${EXEEXT} svmsgd_test${EXEEXT} svmsgd_classify${EXEEXT} svmsgd_classify_pipeline${EXEEXT} svmsgd_model_loader_pipeline${EXEEXT} svmsgd_end_pipeline${EXEEXT}
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
