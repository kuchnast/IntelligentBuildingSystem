CPP      := g++
CPPFLAGS := -Wall -pedantic -std=c++14
LDFLAGS  := -L/usr/lib -lm -L/usr/lib/arm-linux-gnueabihf/ -lmariadb -lwiringPi
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
SRV_DIR  := ./service
INCLUDE  := -Iinclude/ -I/usr/include/mariadb/mysql -I/usr/include/mariadb -I/usr/include/mysql
TARGET_I2C := obsluga_urzadzen_i2c
TARGET_RUCH := obsluga_zdarzen_ruchu
TARGET_ARDUINO := obsluga_arduino
TARGET_KOTLOWNIA := obsluga_kotlownia
TARGET_BASEN := obsluga_basen
TARGET_SOLARY := obsluga_solary
TARGET_CZASOWE := obsluga_zdarzenia_czasowe

SRC      :=                      \
   $(wildcard src/komunikacja/*.cpp) \
   $(wildcard src/sprzet/*.cpp) \
   src/programy/obsluga_zadan.cpp

SRC_I2C := src/programy/obsluga_urzadzen_i2c.cpp
SRC_RUCH := $(wildcard src/programy/zdarzenia_ruchu/*.cpp)
SRC_ARDUINO := src/programy/obsluga_arduino.cpp
SRC_KOTLOWNIA := $(wildcard src/programy/kotlownia/*.cpp)
SRC_BASEN := $(wildcard src/programy/basen/*.cpp)
SRC_SOLARY := $(wildcard src/programy/solary/*.cpp)
SRC_CZASOWE := src/programy/obsluga_zdarzen_czasowych.cpp


OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_I2C := $(SRC_I2C:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_RUCH := $(SRC_RUCH:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_ARDUINO := $(SRC_ARDUINO:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_KOTLOWNIA := $(SRC_KOTLOWNIA:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_BASEN := $(SRC_BASEN:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_SOLARY := $(SRC_SOLARY:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS_CZASOWE := $(SRC_CZASOWE:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET_I2C) $(APP_DIR)/$(TARGET_RUCH) $(APP_DIR)/$(TARGET_ARDUINO) $(APP_DIR)/$(TARGET_KOTLOWNIA) $(APP_DIR)/$(TARGET_BASEN) $(APP_DIR)/$(TARGET_SOLARY) $(APP_DIR)/$(TARGET_CZASOWE)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET_I2C): $(OBJECTS) $(OBJECTS_I2C)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_I2C) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_I2C) $(TARGET_I2C).app

$(APP_DIR)/$(TARGET_RUCH): $(OBJECTS) $(OBJECTS_RUCH)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_RUCH) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_RUCH) $(TARGET_RUCH).app

$(APP_DIR)/$(TARGET_ARDUINO): $(OBJECTS) $(OBJECTS_ARDUINO)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_ARDUINO) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_ARDUINO) $(TARGET_ARDUINO).app

$(APP_DIR)/$(TARGET_KOTLOWNIA): $(OBJECTS) $(OBJECTS_KOTLOWNIA)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_KOTLOWNIA) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_KOTLOWNIA) $(TARGET_KOTLOWNIA).app

$(APP_DIR)/$(TARGET_BASEN): $(OBJECTS) $(OBJECTS_BASEN)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_BASEN) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_BASEN) $(TARGET_BASEN).app

$(APP_DIR)/$(TARGET_SOLARY): $(OBJECTS) $(OBJECTS_SOLARY)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_SOLARY) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_SOLARY) $(TARGET_SOLARY).app

$(APP_DIR)/$(TARGET_CZASOWE): $(OBJECTS) $(OBJECTS_CZASOWE)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET_CZASOWE) $^ $(LDFLAGS)
	@ln -svf $(APP_DIR)/$(TARGET_CZASOWE) $(TARGET_CZASOWE).app

.PHONY: all build clean arduino debug release script_kotlownia script_serwerownia script_schody

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CPPFLAGS += -ggdb -g
debug: all

release: CPPFLAGS += -O2
release: all

arduino:
	-@sudo avrdude -C/etc/avrdude.conf -v -patmega2560 -cwiring -P/dev/ttyACM0 -b115200 -D -Uflash:w:arduino/.pio/build/megaatmega2560/firmware.hex:i

script_kotlownia:
	-@ln -svf service/run_kotlownia.sh run.sh
	-@chmod +x run.sh

script_serwerownia:
	-@ln -svf service/run_serwerownia.sh run.sh
	-@chmod +x run.sh

script_schody:
	-@ln -svf service/run_schody.sh run.sh
	-@chmod +x run.sh

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -vf $(TARGET_I2C).app
	-@rm -vf $(TARGET_RUCH).app
	-@rm -vf $(TARGET_ARDUINO).app
	-@rm -vf $(TARGET_KOTLOWNIA).app
	-@rm -vf $(TARGET_CZASOWE).app
	-@rm -vf run.sh
