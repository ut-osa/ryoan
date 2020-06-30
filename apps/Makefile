CFLAGS:=$(CFLAGS) -g -O3 -Iinclude -DNOT_SGX=1 -Wall -Wno-format-security
CXXFLAGS:=$(CXXFLAGS) -std=gnu++11 $(CFLAGS)
LDFLAGS:=-lcrypto -fPIE -pthread -lm

LIBPIPELINE_OBJ:=$(addprefix libpipeline/, work_spec.o channel.o utils.o \
					 fitting_spec.o json_builder.o json.o pipeline.o)

PIPEPREFIXES:=$(addprefix pipes/, emailpipe healthpipe imagepipe translationpipe)
NATIVELIB_PIPEFILES:=$(addsuffix _nativelibs.json, $(PIPEPREFIXES))
RYOAN_PIPEFILES:=$(addsuffix _ryoan.json, $(PIPEPREFIXES))
PIPEFILES:=$(NATIVELIB_PIPEFILES) $(RYOAN_PIPEFILES)

#template macros
PROJECT="$(shell readlink -f ..)"
NACLPORTS="$(PROJECT)/naclports"
NATIVELIB_LOADER:="$(PROJECT)/native_client/scons-out/opt-linux-x86-64/staging/sel_ldr"
RYOAN_LOADER:="$(PROJECT)/native_client/scons-out/opt-ryoan-x86-64/staging/sel_ldr"
NACLPORTS_BIN:="$(NACLPORTS)/nacl_sdk/pepper_canary/toolchain/linux_pnacl/x86_64-nacl/usr/bin"
NONACL_BIN:="$(NACLPORTS)/out/build/install_host/bin"
DATA_DIR:="$(shell readlink -f ./data)"

M4_DEFS:= -DNACLPORTS_BIN="$(NACLPORTS_BIN)" \
	-DNONACL_BIN="$(NONACL_BIN)" -DDATA_DIR="$(DATA_DIR)" \
	-DLOADER_ARGS="\"-q\", \"-a\", \"-p\", \"-L\", \"7620b79b17b21b06d97019aa70e1ca105e1c03d2a0cf8b20b5a0ce5c3903e548\""
NATIVELIB_M4_DEFS= -DLOADER="$(NATIVELIB_LOADER)" $(M4_DEFS)
RYOAN_M4_DEFS= -DLOADER="$(RYOAN_LOADER)" $(M4_DEFS)

LIBSODIUM="./build-libsodium/src/libsodium/.libs/libsodium.a"

CXX_PROGS=start_pipeline email_client health_client health_server image_client \
			 image_server translation_client translation_server
CC_PROGS=email_server

all: $(CXX_PROGS) $(CC_PROGS) $(PIPEFILES)
.PHONY: all

$(NATIVELIB_PIPEFILES): pipes/%_nativelibs.json: pipe_templates/%.json.m4
	@mkdir -p pipes
	@echo "	M4 $@"
	@m4 $(NATIVELIB_M4_DEFS) $^ > $@

$(RYOAN_PIPEFILES): pipes/%_ryoan.json: pipe_templates/%.json.m4
	@mkdir -p pipes
	@echo "	M4 $@"
	@m4 $(RYOAN_M4_DEFS) $^ > $@

$(CXX_PROGS): %: %.o libpipeline.a libsodium.a
	@echo "	LINK $@"
	@$(CXX) -o $@ $(CXXFLAGS) $^ $(LDFLAGS)

$(CC_PROGS): %: %.o
	@echo "	LINK $@"
	@$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

libpipeline.a: $(LIBPIPELINE_OBJ)
	@echo "	AR $@"
	@$(AR) rs $@ $^

libsodium.a: $(LIBSODIUM)
	@echo "	INSTALL $@"
	@cp $^ $@

$(LIBSODIUM):
	@echo "	BUILD $@"
	@$(MAKE) -C build-libsodium

%.o: %.cpp
	@echo "	CXX $@"
	@$(CXX) -c -o $@ $(CXXFLAGS) $^
.PRECIOUS: %.o

%.o: %.c
	@echo "	CC $@"
	@$(CC) -c -o $@ $(CFLAGS) $^
.PRECIOUS: %.o

clean:
	rm -rf *.o */*.o pipes start_pipeline *_server *_client
.PHONY: all
