# CMakeList.txt : CMake project for motion_photo_video_extractor, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("motion_photo_video_extractor" VERSION 1.0)

set (CMAKE_CXX_STANDARD 11)

include_directories (tinyxml2)
include_directories (TinyEXIF)
include_directories (argparse)
include_directories (heic)


# Local source files here
SET(TARGET_SRC
    TinyEXIF/TinyEXIF.cpp
    tinyxml2/tinyxml2.cpp
    heic/heifreader.cpp
    heic/heifboxes.cpp
    main.cpp
    )

# Add source to this project's executable.
add_executable (mopho_video_extractor ${TARGET_SRC})
