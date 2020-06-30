add_definitions(-DCASCADE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models/")
set( OpenCV_LIBRARIES -lopencv_videoio -lopencv_objdetect
  -lopencv_video -lopencv_calib3d -lopencv_features2d -lopencv_flann
  -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core)
set( EXEEXT "" CACHE STRING "program suffix" )

add_executable(tag_photo${EXEEXT}
    objectcounter.cpp
    objectrec.cpp
    tagger.cpp
)
set_target_properties(tag_photo${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(tag_photo${EXEEXT} LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES} m
  tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

add_executable(image_pipe_start${EXEEXT}
    image_pipe_start.c
)
target_link_libraries(image_pipe_start${EXEEXT} LINK_PUBLIC pipeline crypto m ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(image_pipe_start${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")
add_executable(image_pipe_end${EXEEXT}
    image_pipe_end.c
)
target_link_libraries(image_pipe_end${EXEEXT} LINK_PUBLIC pipeline crypto m ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(image_pipe_end${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")

add_executable(json_join${EXEEXT}
    json_join.c
)
target_link_libraries(json_join${EXEEXT} LINK_PUBLIC pipeline crypto m ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${DL_LIB} )
set_target_properties(json_join${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=c99 -static -fPIC")

add_executable(blur_photo${EXEEXT}
    faceblur.cpp
    util.cpp
)
set_target_properties(blur_photo${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(blur_photo${EXEEXT} LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES} m
    tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

add_executable(screen_photo${EXEEXT} 
    imagescreen.cpp
    util.cpp
)
set_target_properties(screen_photo${EXEEXT} PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(screen_photo${EXEEXT} LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES}
  m tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

add_executable(layer_photo${EXEEXT} 
    imagelayer.cpp
    util.cpp
)
set_target_properties(layer_photo${EXEEXT}  PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(layer_photo${EXEEXT}  LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES}
  m tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

add_executable(normalize_photo${EXEEXT} 
    imagenormalize.cpp
    util.cpp
)
set_target_properties(normalize_photo${EXEEXT}  PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(normalize_photo${EXEEXT}  LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES}
  m tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

add_executable(search_photo${EXEEXT} 
    imagesearch.cpp
    util.cpp
)
set_target_properties(search_photo${EXEEXT}  PROPERTIES COMPILE_FLAGS "-std=gnu++11")
target_link_libraries(search_photo${EXEEXT}  LINK_PUBLIC pipeline crypto ${OpenCV_LIBRARIES}
  m tiff jasper jpeg png webp z lzma ${GLIBC-COMPAT} ${NACL_SYS_LIB} ${PTHREAD_LIB} ${DL_LIB} )

install (TARGETS tag_photo${EXEEXT}  screen_photo${EXEEXT}  layer_photo${EXEEXT} 
  normalize_photo${EXEEXT} search_photo${EXEEXT} blur_photo${EXEEXT} image_pipe_start${EXEEXT}
  json_join${EXEEXT} image_pipe_end${EXEEXT}
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
