all:
	install -m 0755 ./prebuilt/$(BR2_DIR)/*.so $(STAGING_DIR)/usr/lib
	install -m 0755 ./prebuilt/noarch/include/* $(STAGING_DIR)/usr/include/
	-$(MAKE) -C example/AmTsPlayerExample
clean:
	rm $(TARGET_DIR)/usr/lib/libmediahal* -rf
	rm $(STAGING_DIR)/usr/lib/libmediahal* -rf
	-$(MAKE) -C example/AmTsPlayerExample clean

install:
	install -m 0755 ./prebuilt/$(BR2_DIR)/*.so $(TARGET_DIR)/usr/lib/
	-$(MAKE) -C example/AmTsPlayerExample install

uninstall:
	rm $(TARGET_DIR)/usr/lib/libmediahal* -rf
	rm $(STAGING_DIR)/usr/lib/libmediahal* -rf
	-$(MAKE) -C example/AmTsPlayerExample uninstall
