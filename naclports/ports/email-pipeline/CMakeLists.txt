set( EXEEXT "" CACHE STRING "program suffix" )

add_executable(email_pipe_start${EXEEXT}
    email_pipe_start.c
)
target_link_libraries(email_pipe_start${EXEEXT} LINK_PUBLIC pipeline crypto m ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(email_pipe_start${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")

add_executable(email_pipe_end${EXEEXT}
    email_pipe_end.c
)
target_link_libraries(email_pipe_end${EXEEXT} LINK_PUBLIC pipeline crypto m  ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(email_pipe_end${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")

install (TARGETS email_pipe_start${EXEEXT}  email_pipe_end${EXEEXT}
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
