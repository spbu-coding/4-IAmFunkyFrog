cmake_minimum_required(VERSION 3.7)
project(tests LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable(converter src/converter.c src/libs/bmpv3converter.c)
add_executable(comparer src/comparer.c src/libs/bmpv3converter.c)

if(MSVC)
  target_compile_options(converter PRIVATE /W4 /WX)
  target_compile_options(comparer PRIVATE /W4 /WX)
else()
  target_compile_options(converter PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(comparer PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

