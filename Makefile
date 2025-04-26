CXX = g++
# 添加-mclflushopt标志以启用CLFLUSHOPT指令支持
CXXFLAGS = -std=c++11 -fPIC -Wall -O2 -mclflushopt -laccel-config
INCLUDES = -Isrc -Isrc/interfaces -Isrc/details

# 获取源文件列表
SRCS = $(wildcard src/*.cpp) $(wildcard src/interfaces/*.cpp) $(wildcard src/details/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = lib/libdsa_memcpy.a

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
	mkdir -p lib

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf lib