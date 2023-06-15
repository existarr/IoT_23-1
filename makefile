SRC_DIR = .
BUILD_DIR = build
EXEC_DIR = bin

CC = gcc
LDFLAGS = -lmosquitto

.PHONY: all clean

all: $(BUILD_DIR)/broker_recovery.o $(BUILD_DIR)/admin_logs.o $(BUILD_DIR)/admin_alerts.o $(BUILD_DIR)/nth_313_pub.o $(BUILD_DIR)/nth_313_sub.o

$(BUILD_DIR)/broker_recovery.o: server/broker_recovery.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@

$(BUILD_DIR)/admin_alerts.o: admin/admin_alerts.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@

$(BUILD_DIR)/admin_logs.o: admin/admin_logs.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@

$(BUILD_DIR)/nth_313_pub.o: pub/nth_313_pub.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@

$(BUILD_DIR)/nth_313_sub.o: sub/nth_313_sub.c
	@mkdir -p $(@D)
	$(CC) -c $< -o $@

all: $(EXEC_DIR)/broker_recovery $(EXEC_DIR)/admin_logs $(EXEC_DIR)/admin_alerts $(EXEC_DIR)/nth_313_pub $(EXEC_DIR)/nth_313_sub

$(EXEC_DIR)/broker_recovery: $(BUILD_DIR)/broker_recovery.o
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXEC_DIR)/admin_logs: $(BUILD_DIR)/admin_logs.o
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXEC_DIR)/admin_alerts: $(BUILD_DIR)/admin_alerts.o
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXEC_DIR)/nth_313_pub: $(BUILD_DIR)/nth_313_pub.o
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXEC_DIR)/nth_313_sub: $(BUILD_DIR)/nth_313_sub.o
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(EXEC_DIR)
