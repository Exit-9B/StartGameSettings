cmake_minimum_required(VERSION 3.24)

project(
	StartGameSettings
	VERSION 1.0.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
include(SKSEPlugin)

set(SKSE_USE_XBYAK ON)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		.clang-format
		.editorconfig
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.hpp
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

find_path(ENCHANTUM_INCLUDE_DIRS "enchantum/enchantum.hpp")
find_package(glaze CONFIG REQUIRED)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${ENCHANTUM_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		glaze::glaze
)

set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")

install(
	FILES
		"${DATA_DIR}/StartGameSettings_Difficulty.esp"
		"${DATA_DIR}/StartGameSettings_SurvivalMode.esp"
	DESTINATION "."
)

install(
	FILES
		"${DATA_DIR}/Interface/StartMenu.swf"
	DESTINATION "Interface"
)

install(
	FILES
		"${DATA_DIR}/Interface/Translations/StartGameSettings_ENGLISH.txt"
		"${DATA_DIR}/Interface/Translations/StartGameSettings_Difficulty_ENGLISH.txt"
		"${DATA_DIR}/Interface/Translations/StartGameSettings_SurvivalMode_ENGLISH.txt"
	DESTINATION "Interface/Translations"
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/StartGameSettings_Difficulty.json"
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/StartGameSettings_SurvivalMode.json"
	DESTINATION "SKSE/Plugins/${PROJECT_NAME}"
)

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  file(WRITE "${CMAKE_BINARY_DIR}/.gitignore" "*")
  file(WRITE "${CMAKE_BINARY_DIR}/.hgignore" "syntax: glob\n**/*")
endif()

set(CPACK_GENERATOR "7Z")
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
if(BUILD_SKYRIMVR)
	string(APPEND CPACK_PACKAGE_FILE_NAME "-VR")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
