UDEFS += "-DCH_CFG_CORE_ALLOCATOR_FAILURE_HOOK()={extern uint8_t _module_freemem_init_phase;if(_module_freemem_init_phase){chSysHalt(NULL);}}"
