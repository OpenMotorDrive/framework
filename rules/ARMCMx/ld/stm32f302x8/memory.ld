MEMORY
{
    bl (rx) :             ORIGIN = 0x08000000,            LENGTH = 12K
    app (rx) :            ORIGIN = 0x08000000+12K,        LENGTH = 84K-12K
/*     params (rx) :         ORIGIN = 0x08000000+62K,        LENGTH = 2K */
    ram (rwx) :           ORIGIN = 0x20000000,            LENGTH = 16K-256
    app_bl_shared (rwx) : ORIGIN = 0x20000000+(16K-256),  LENGTH = 256
}
