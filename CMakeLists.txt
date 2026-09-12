cmake_minimum_required(VERSION 3.21)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project(lynx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
# stb e so um monte de header, nao tem biblioteca pra linkar:
# por isso ele entra em target_include_directories e nao em target_link_libraries.
find_package(Stb REQUIRED)

add_executable(lynx src/main.cpp)

target_include_directories(lynx PRIVATE src ${Stb_INCLUDE_DIR})

target_link_libraries(lynx PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    SDL2::SDL2
    glad::glad
    OpenGL::GL
)

# Copia a pasta assets/ pra junto do .exe toda vez que compilamos.
# O jogo procura as imagens ao lado do executavel, entao elas precisam estar la
# (em build/Debug/assets), e nao so na raiz do projeto.
add_custom_command(TARGET lynx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:lynx>/assets"
    COMMENT "Copiando assets para a pasta do executavel"
)
