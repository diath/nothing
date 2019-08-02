cmake_minimum_required(VERSION 3.14)
project(nothing)
include_directories(src)

option(WITH_GUI_QT "Qt GUI frontend" OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wfatal-errors -pipe -lstdc++fs")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fpie -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -fsanitize=address,undefined,leak")

# Threading
find_package(Threads)

# SQLite3
find_package(SQLite3)

# Qt
if(WITH_GUI_QT)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)

	find_package(Qt5 COMPONENTS Widgets REQUIRED)

	set(src_gui
		src/gui/qt/main.cpp
		src/gui/qt/mainwindow.cpp
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

add_executable(nothing ${src})
target_link_libraries(nothing ${CMAKE_THREAD_LIBS_INIT} sqlite3 ${lib_gui})
