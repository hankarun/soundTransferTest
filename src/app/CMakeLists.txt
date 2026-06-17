add_executable(soundTransfer
    main.cpp
    MainWindow.h
    MainWindow.cpp
)

target_link_libraries(soundTransfer
    PRIVATE
        SoundCore
        Qt5::Widgets
        Qt5::Multimedia
)

# On Windows, run as a GUI app (no console window).
if(WIN32)
    set_target_properties(soundTransfer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
