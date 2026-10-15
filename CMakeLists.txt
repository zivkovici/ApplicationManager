cmake_minimum_required(VERSION 3.21)

project(ApplicationManager VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Concurrent
    Network
    REQUIRED
)

# Source files
set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/AppListModel.cpp
    src/AppListModel.h
    src/WingetBackend.cpp
    src/WingetBackend.h
    src/AsyncWorker.cpp
    src/AsyncWorker.h
    src/models/AppData.h
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
)

# Set Visual Studio properties
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=$<CONFIG>"
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
    # Enable UTF-8 source files
    add_compile_options(/utf-8)
endif()

# Install settings (optional)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
