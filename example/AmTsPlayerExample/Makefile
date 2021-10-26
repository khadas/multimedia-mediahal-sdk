OBJ=AmTsPlayerExample.o

LDFLAGS =-lpthread -laudio_client -lmediahal_videodec -lmediahal_tsplayer -lmediahal_resman -lmediahal_mediasync -lwayland-client -lm -lz -g $(EXTRA_LDFLAGS)

CFLAGS =-I$(STAGING_DIR)/usr/include/ $(EXTRA_CFLAGS)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

AmTsPlayerExample: $(OBJ)
	$(CXX) $^ $(LDFLAGS) $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o *~ AmTsPlayerExample

.PHONY: install
install:
	cp -f AmTsPlayerExample $(TARGET_DIR)/usr/bin/

.PHONY: uninstall
uninstall:
	rm -f $(TARGET_DIR)/usr/bin/AmTsPlayerExample
