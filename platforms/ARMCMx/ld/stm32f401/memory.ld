MEMORY
{
    bl (rx) :             ORIGIN = 0x08000000,            LENGTH = 32K
    app (rx) :            ORIGIN = 0x08000000+64K,        LENGTH = 256K-64K-16k-16k
    param1 (rx) :         ORIGIN = 0x08000000+224K,        LENGTH = 16K
    param2 (rx) :         ORIGIN = 0x08000000+240K,        LENGTH = 16K
    ram (rwx) :           ORIGIN = 0x20000000,            LENGTH = 64K-256
    app_bl_shared (rwx) : ORIGIN = 0x20000000+(64K-256), LENGTH = 256
}
