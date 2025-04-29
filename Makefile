CXX = g++
# 添加-mclflushopt标志以启用CLFLUSHOPT指令支持
CXXFLAGS = -std=c++11 -fPIC -Wall -O2 -mclflushopt -mclwb
INCLUDES = -Isrc -Isrc/interfaces -Isrc/details
LDFLAGS = -laccel-config

# 获取源文件列表
SRCS = $(wildcard src/*.cpp) $(wildcard src/interfaces/*.cpp) $(wildcard src/details/*.cpp) 
OBJS = $(SRCS:.cpp=.o)
TARGET = lib/libdsa_memcpy.a
SO_TARGET = lib/libdsa_memcpy.so

.PHONY: all clean dirs

all: dirs $(TARGET) $(SO_TARGET) 

dirs:
	mkdir -p lib

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(SO_TARGET): $(OBJS) 
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
 
$(OBJS): src/details/dsa_conf.hpp
 
-include $(DEPS)

clean:
	rm -f $(OBJS) $(TARGET) $(SO_TARGET)
	rm -rf lib