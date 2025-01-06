set (CMAKE_CXX_STANDARD 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SANITIZING_FLAGS -fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)

# ask for more warnings from the compiler
set (CMAKE_BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Wno-unqualified-std-cast-call -Wno-non-virtual-dtor")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # 设置支持的警告选项
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unqualified-std-cast-call")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")  # 禁用所有警告
