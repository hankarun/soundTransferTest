set(SOUND_TESTS
    tst_opuscodec
    tst_udptransport
    tst_transmission
)

foreach(test ${SOUND_TESTS})
    add_executable(${test} ${test}.cpp TestSignal.h)
    target_link_libraries(${test}
        PRIVATE
            SoundCore
            Qt5::Test
            Qt5::Network
            Qt5::Multimedia
    )
    # tst_transmission/tst_udptransport need opus headers indirectly via SoundCore;
    # opus encode/decode is used directly in the codec tests.
    target_link_libraries(${test} PRIVATE opus)
    add_test(NAME ${test} COMMAND ${test})
endforeach()
