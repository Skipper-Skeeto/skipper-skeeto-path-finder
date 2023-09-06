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

ifeq ($(game_version),1-1)
GAME_VERSION_ID := 0
else ifeq ($(game_version),1-2)
GAME_VERSION_ID := 1
else ifeq ($(game_version),2)
GAME_VERSION_ID := 2
endif

CHECK_GAME_VERSION:=$(BUILD_DIR)/.last-game-version.$(game_version)

LFLAGS = -L${BOOST_ROOT}/lib

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) -pthread $(OBJS) -o $@ $(LDFLAGS) -static $(LFLAGS) $(LIBS) \
		-lrt -Wl,--whole-archive -lpthread -Wl,--no-whole-archive  # Avoid segmentation fault when joining the threads. See https://stackoverflow.com/a/45271521

$(BUILD_DIR)/%.cpp.o: %.cpp $(CHECK_GAME_VERSION)
	@if [ -z ${GAME_VERSION_ID} ]; then >&2 echo Please specify a valid game_version, for instance \"make game_version=1-2\"; exit 1; fi
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) -D GAME_VERSION=$(GAME_VERSION_ID) $(CXXFLAGS) -c $< -o $@

# https://stackoverflow.com/q/65400877/2761541
$(BUILD_DIR)/.last-game-version.$(game_version):
	@if [ -z ${game_version} ]; then >&2 echo Please specify a game_version, for instance \"make game_version=1-2\"; exit 1; fi
	-rm -f $(BUILD_DIR)/.last-game-version.*
	$(MKDIR_P) $(dir $@)
	touch $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
