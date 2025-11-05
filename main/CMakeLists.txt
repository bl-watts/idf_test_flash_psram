idf_component_register(SRCS "spiram_flash_test.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bootloader_support spi_flash)
