# Copyright 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TEST_EXECUTABLES="
bin/opencv_test_calib3d
bin/opencv_test_core
bin/opencv_test_features2d
bin/opencv_test_flann
bin/opencv_test_highgui
bin/opencv_test_imgcodecs
bin/opencv_test_imgproc
bin/opencv_test_ml
bin/opencv_test_objdetect
bin/opencv_test_photo
bin/opencv_test_shape
bin/opencv_test_stitching
bin/opencv_test_superres
bin/opencv_test_video
bin/opencv_test_videoio
"

PERF_TEST_EXECUTABLES="
bin/opencv_perf_calib3d
bin/opencv_perf_core
bin/opencv_perf_features2d
bin/opencv_perf_imgcodecs
bin/opencv_perf_imgproc
bin/opencv_perf_objdetect
bin/opencv_perf_photo
bin/opencv_perf_stitching
bin/opencv_perf_superres
bin/opencv_perf_video
bin/opencv_perf_videoio
"

EXECUTABLES="
bin/opencv_annotation
bin/opencv_createsamples
bin/opencv_traincascade
"

EXECUTABLES="$EXECUTABLES $TEST_EXECUTABLES $PERF_TEST_EXECUTABLES"

EXTRA_CMAKE_ARGS="-DBUILD_SHARED_LIBS=OFF \
           -DWITH_FFMPEG=OFF \
           -DWITH_OPENEXR=OFF \
           -DWITH_CUDA=OFF \
           -DWITH_OPENCL=OFF \
           -DWITH_1394=OFF \
           -DWITH_V4L=OFF \
           -DWITH_IPP=OFF \
           -DWITH_PTHREADS_PF=ON \
           -DWITH_GDAL=OFF \
           -DBUILD_opencv_java=OFF"

DATA_DIR=opencv_extra-3.1.0
DATA_FILE=${DATA_DIR}.tar.gz
DATA_URL=https://github.com/Itseez/opencv_extra/archive/3.1.0/${DATA_FILE}
DATA_SHA1=0b92ec7614b08ccb95330d1f9b28459bff583241

export OPENCV_TEST_DATA_PATH=${WORK_DIR}/${DATA_DIR}/testdata

DownloadStep() {
    if ! CheckHash ${NACL_PACKAGES_CACHE}/${DATA_FILE} ${DATA_SHA1}; then
        Fetch ${DATA_URL} ${NACL_PACKAGES_CACHE}/${DATA_FILE}
        if ! CheckHash ${NACL_PACKAGES_CACHE}/${DATA_FILE} ${DATA_SHA1}; then
            Banner "${DATA_FILE} failed checksum!"
            exit -1
        fi
    fi
    ChangeDir ${WORK_DIR}
    LogExecute tar -xvf ${NACL_PACKAGES_CACHE}/${DATA_FILE}
}

BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LogExecute cmake \
    -DCMAKE_INSTALL_PREFIX=${HOST_INSTALL_DIR} \
    -DCMAKE_INCLUDE_PATH=${HOST_INSTALL_DIR}/include \
    -DCMAKE_LIBRARY_PATH=${HOST_INSTALL_DIR}/lib \
    -DCMAKE_EXE_LINKER_FLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    -DCMAKE_SHARED_LINKER_FLAGS=\
    "-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    -DWITH_FFMPEG=OFF \
    -DWITH_OPENEXR=OFF \
    -DWITH_CUDA=OFF \
    -DWITH_OPENCL=OFF \
    -DWITH_1394=OFF \
    -DWITH_V4L=OFF \
    -DWITH_IPP=OFF \
    -DWITH_PTHREADS_PF=ON \
    -DWITH_GDAL=OFF \
    -DBUILD_opencv_java=OFF \
    ${SRC_DIR}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
  LogExecute cp -f bin/opencv_annotation bin/opencv_createsamples \
    bin/opencv_traincascade ${HOST_INSTALL_DIR}/bin
  cd -
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

PostBuildStep() {
   echo "SKIPPING POSTBUILD"
}

# RunTests() {
#     for nexe in $1; do
#         local basename=$(basename ${nexe})
#         ./${basename}.sh
#     done
# }
# 
# TestStep() {
#     cd ${BUILD_DIR}/bin
#     Banner "Running accuracy & regression tests"
#     RunTests ${TEST_EXECUTABLES}
#     Banner "Running performance tests"
#     cd ${BUILD_DIR}
#     python ../opencv-3.1.0/modules/ts/misc/run.py .
# }
