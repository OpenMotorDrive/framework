#include <common/ctor.h>
#include <modules/driver_ms5525dso/driver_ms5525dso.h>

struct ms5525dso_instance_s ms5525dso;
RUN_AFTER(INIT_END) {
    ms5525dso_init(&ms5525dso, 3, BOARD_PAL_LINE_SPI_CS_MS5525DSO);
}
