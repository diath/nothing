cmake_minimum_required(VERSION 3.14)
project(nothing)
include_directories(src)

option(WITH_GUI_QT "Qt GUI frontend" OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wfatal-errors -pipe")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fpie -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -ggdb -fsanitize=address,undefined,leak")

# Threading
find_package(Threads)

# SQLite3
find_package(SQLite3)

# Qt
if(WITH_GUI_QT)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTORCC ON)

	find_package(Qt5 COMPONENTS Widgets REQUIRED)

	set(src_gui
		src/gui/qt/main.cpp
		src/gui/qt/tablemodel.cpp
		src/gui/qt/mainwindow.cpp
		src/gui/qt/pathsdialog.cpp
		src/gui/res/res.qrc
	)

	set(lib_gui
		Qt5::Widgets
	)
else()
	set(src_gui
		src/main.cpp
	)
endif()

set (src
	src/core/database.cpp
	src/core/scanner.cpp
	src/core/utils.cpp
	${src_gui}
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions(-DPLATFORM_LINUX)
	set (src
		${src}
		src/core/watcher_linux.cpp
	)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-DPLATFORM_WINDOWS)
	set (src
		${src}
		src/core/watcher_windows.cpp
	)
else()
	message(FATAL_ERROR "Platform not supported.")
endif()

add_executable(nothing ${src})
target_link_libraries(nothing ${CMAKE_THREAD_LIBS_INIT} ${SQLite3_LIBRARIES} ${lib_gui})
