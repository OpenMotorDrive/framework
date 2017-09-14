INCLUDE memory.ld

REGION_ALIAS("PROGRAM_REGION", bl)

INCLUDE common.ld

PROVIDE(_app_sec = ORIGIN(app));
PROVIDE(_app_sec_end = ORIGIN(app)+LENGTH(app)+LENGTH(params));
