OBJ=EsVideoDecPlayer.o

LDFLAGS=-lpthread -lmediahal_videodec -lm -lz -g -ldl $(EXTRA_LDFLAGS)

CFLAGS=-I$(STAGING_DIR)/usr/include/ $(EXTRA_CFLAGS)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

EsVideoDecPlayer: $(OBJ)
	$(CXX) $^ $(LDFLAGS) $(CFLAGS) -o $@
	cp -f EsVideoDecPlayer  $(STAGING_DIR)/usr/bin

.PHONY: clean

clean:
	rm -f *.o *~ EsVideoDecPlayer

.PHONY: install

install:
	cp -f EsVideoDecPlayer $(TARGET_DIR)/usr/bin/
	@echo $(STAGING_DIR)
#.PHONY: uninstall

#uninstall:
#	rm -f $(TARGET_DIR)/usr/bin/amaudi
