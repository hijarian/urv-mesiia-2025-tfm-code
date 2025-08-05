# CMakeList.txt: проект CMake для pm_solver; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.20)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("pm_solver")

# Проверка платформы сборки
if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 8))
    message(FATAL_ERROR "Этот проект поддерживает только сборку для x64-windows (64-битная Windows).")
endif()

# find_package(fuzzylite CONFIG REQUIRED) # fuzzylite не интегрируется с CMake при установке через vcpkg, так что придётся вручную всё подключать, а это оставить как референс.
find_package(pagmo CONFIG REQUIRED) # Установленный через vcpkg этот пакет называется именно так!

# Добавьте источник в исполняемый файл этого проекта.
add_executable (pm_solver "pm_solver.cpp" "pm_solver.h")

# Это нужно чтобы подобрать каталог заголовков из vcpkg. Это для fuzzylite, который не интегрируется с CMake при установке через vcpkg.
target_include_directories(pm_solver PRIVATE
    ${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/include
)

target_link_libraries(pm_solver
  PRIVATE
	${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/lib/fuzzylite.lib # fuzzylite не интегрируется с CMake при установке через vcpkg, так что указываем библиотеку вручную
	Pagmo::pagmo # Установленный через vcpkg пагмо называется именно так!
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET pm_solver PROPERTY CXX_STANDARD 23)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
