add_library(SoundCore STATIC
    AudioFormat.h
    OpusCodec.h        OpusCodec.cpp
    UdpTransport.h     UdpTransport.cpp
    AudioCapture.h     AudioCapture.cpp
    AudioPlayback.h    AudioPlayback.cpp
    AudioEngine.h      AudioEngine.cpp
)

target_include_directories(SoundCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SoundCore
    PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Multimedia
    PRIVATE
        opus
)
