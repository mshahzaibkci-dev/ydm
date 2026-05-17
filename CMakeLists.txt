cmake_minimum_required(VERSION 3.22)
project(RAINAX VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 modules required
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/constants.cpp
    src/downloaditem.cpp
    src/downloadworker.cpp
    src/genericfileworker.cpp
    src/downloadmanager.cpp
    src/apiserver.cpp
    src/persistence.cpp
    src/scheduleitem.cpp
    src/utils.cpp
    src/iconhelper.cpp
    src/themeprovider.cpp

    # UI
    src/mainwindow.cpp
    src/mainwindow_build.cpp
    src/mainwindow_pages.cpp
    src/mainwindow_slots.cpp
    src/widgets/statusbadge.cpp
    src/widgets/glossyprogressbar.cpp
    src/widgets/statchip.cpp
    src/widgets/statbox.cpp
    src/widgets/sidebarbutton.cpp
    src/widgets/ydmdialog.cpp
    src/widgets/scheduledialog.cpp
    src/widgets/backgroundloader.cpp
    src/instanceserver.cpp

    # yt-dlp update workers
    src/ytdlpupdateworker.cpp
    src/ffmpegupdateworker.cpp
)

set(HEADERS
    include/constants.h
    include/downloaditem.h
    include/downloadworker.h
    include/genericfileworker.h
    include/downloadmanager.h
    include/apiserver.h
    include/persistence.h
    include/scheduleitem.h
    include/utils.h
    include/iconhelper.h
    include/themeprovider.h

    include/mainwindow.h
    include/widgets/statusbadge.h
    include/widgets/glossyprogressbar.h
    include/widgets/statchip.h
    include/widgets/statbox.h
    include/widgets/sidebarbutton.h
    include/widgets/ydmdialog.h
    include/widgets/scheduledialog.h
    include/widgets/backgroundloader.h
    include/instanceserver.h

    include/ytdlpupdateworker.h
    include/ffmpegupdateworker.h
)

qt_add_executable(RAINAX
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

target_include_directories(RAINAX PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(RAINAX PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# Windows-specific: no console window in release, embed app icon
if(WIN32)
    set_target_properties(RAINAX PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Link Windows registry API for startup management
    target_link_libraries(RAINAX PRIVATE advapi32)
endif()

# Install
install(TARGETS RAINAX
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
