all:
	install -m 0755 ./videodec/prebuilt/$(BR2_DIR)/*.so $(STAGING_DIR)/usr/lib
	install -m 0755 ./tsplayer/prebuilt/$(BR2_DIR)/*.so $(STAGING_DIR)/usr/lib
	install -m 0755 ./resmanage-bin/prebuilt/$(BR2_DIR)/*.so $(STAGING_DIR)/usr/lib
	install -m 0755 ./videodec/prebuilt/noarch/include/* $(STAGING_DIR)/usr/include/
	install -m 0755 ./tsplayer/prebuilt/noarch/include/* $(STAGING_DIR)/usr/include/
	install -m 0755 ./resmanage-bin/prebuilt/noarch/include/* $(STAGING_DIR)/usr/include/
	-$(MAKE) -C example/AmTsPlayerExample
clean:
	rm $(TARGET_DIR)/usr/lib/libmediahal* -rf
	rm $(STAGING_DIR)/usr/lib/libmediahal* -rf
	-$(MAKE) -C example/AmTsPlayerExample clean

install:
	install -m 0755 ./videodec/prebuilt/$(BR2_DIR)/*.so $(TARGET_DIR)/usr/lib/
	install -m 0755 ./tsplayer/prebuilt/$(BR2_DIR)/*.so $(TARGET_DIR)/usr/lib/
	install -m 0755 ./resmanage-bin/prebuilt/$(BR2_DIR)/*.so $(TARGET_DIR)/usr/lib/
	-$(MAKE) -C example/AmTsPlayerExample install

uninstall:
	rm $(TARGET_DIR)/usr/lib/libmediahal* -rf
	rm $(STAGING_DIR)/usr/lib/libmediahal* -rf
	-$(MAKE) -C example/AmTsPlayerExample uninstall
