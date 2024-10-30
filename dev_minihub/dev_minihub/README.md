# Upload when using Secure Boot

- Build project:\
  `idf.py build`
- Upload bootloader:\
  `esptool.py --chip esp32c3 --before=default_reset --after=no_reset --no-stub write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 ./build/bootloader/bootloader.bin`
- Upload partition table:\
  `idf.py partition-table-flash`
- Upload app:\
  `idf.py encrypted-flash monitor`

