cmake_minimum_required(VERSION 3.16)
project(Othello VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(Qt6 COMPONENTS Widgets OpenGLWidgets Concurrent REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_executable(Othello
  src/Main.cpp
  src/Board.cpp
  src/Solver.cpp
  src/OthelloWidget.cpp
  src/PointsWidget.cpp
  include/OthelloWidget.hpp
  include/PointsWidget.hpp
  include/SolverSettingsDialog.hpp
)

target_link_libraries(Othello PRIVATE
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Qt6::Concurrent
  OpenGL::GL
  OpenGL::GLU
  OpenMP::OpenMP_CXX
)
target_compile_definitions(Othello PRIVATE _OPENMP)

add_executable(AutoCompare
  src/AutoCompare.cpp
  src/Board.cpp
  src/Solver.cpp
)

target_link_libraries(AutoCompare PRIVATE
  Qt6::Concurrent
  OpenMP::OpenMP_CXX
)
target_compile_definitions(AutoCompare PRIVATE _OPENMP)

enable_testing()

add_executable(TestBoard
  tests/TestBoard.cpp
  src/Board.cpp
  tests/TestUtils.hpp
)
target_link_libraries(TestBoard PRIVATE OpenMP::OpenMP_CXX)
target_compile_definitions(TestBoard PRIVATE _OPENMP)
add_test(NAME BoardTests COMMAND TestBoard)

add_executable(TestSolver
  tests/TestSolver.cpp
  src/Board.cpp
  src/Solver.cpp
  tests/TestUtils.hpp
)
target_link_libraries(TestSolver PRIVATE OpenMP::OpenMP_CXX)
target_compile_definitions(TestSolver PRIVATE _OPENMP)
add_test(NAME SolverTests COMMAND TestSolver)

