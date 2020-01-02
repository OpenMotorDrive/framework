MEMORY
{
    bl (rx) :             ORIGIN = 0x08000000,            LENGTH = 32K
    app (rx) :            ORIGIN = 0x08000000+64K,        LENGTH = 2M-64K
    param1 (rx) :         ORIGIN = 0x08000000+32K,        LENGTH = 16K
    param2 (rx) :         ORIGIN = 0x08000000+48K,        LENGTH = 16K
    ram (rwx) :           ORIGIN = 0x20000000,            LENGTH = 192K-256
    app_bl_shared (rwx) : ORIGIN = 0x20000000+(192K-256), LENGTH = 256
}
