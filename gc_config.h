//
// Created by rafaelvaloto on 03/02/2026.
//
#pragma once

#if defined(GAMEPAD_CORE_EMBEDDED)
    #include "pico/stdlib.h"

    #ifndef gc_sleep_ms
        #define gc_sleep_ms ::sleep_ms
    #endif
#endif
