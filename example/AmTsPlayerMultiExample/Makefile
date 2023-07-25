OBJ=AmTsPlayerMultiExample.o

LD_SUPPORT_WAYLAND =
LDFLAGS =-lpthread -laudio_client -lmediahal_videodec -lmediahal_tsplayer -lmediahal_resman -lmediahal_mediasync -lm -lz -g $(EXTRA_LDFLAGS)

LDFLAGS += $(LD_SUPPORT_WAYLAND)
CFLAGS =-I$(STAGING_DIR)/usr/include/ $(EXTRA_CFLAGS)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

AmTsPlayerMultiExample: $(OBJ)
	$(CXX) $^ $(LDFLAGS) $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o *~ AmTsPlayerMultiExample

.PHONY: install
install:
	cp -f AmTsPlayerMultiExample $(TARGET_DIR)/usr/bin/

.PHONY: uninstall
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/AmTsPlayerMultiExample
