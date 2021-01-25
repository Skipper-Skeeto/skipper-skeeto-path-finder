TARGET_EXEC ?= a.out

BUILD_DIR ?= ./build
SRC_DIRS ?= ./skipper-skeeto-path-finder/src
INC_DIRS ?= ./skipper-skeeto-path-finder/include ./libs/json/include ./libs/parallel-hashmap ${BOOST_ROOT}/include

SRCS := $(wildcard ./skipper-skeeto-path-finder/src/*.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
LIBS = -lboost_iostreams

INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -O3 -march=native

LFLAGS = -L${BOOST_ROOT}/lib

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) -pthread $(OBJS) -o $@ $(LDFLAGS) -static $(LFLAGS) $(LIBS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
