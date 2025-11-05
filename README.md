# IDF stresstest of SPI flash and PSRAM

The reason for the test was to test out how handling of flash (found on spi) and PSRAM had handling.

We expected to hit some errors [as defined in IDF docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html#restrictions) but found none, yet.

## building

```sh
idf.py build
```