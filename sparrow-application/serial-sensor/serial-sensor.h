// Copyright 2023 Cameron Bunce
// Licensed alike with the work from Blues that makes this all possible
// The mistakes contained herein are mine

#pragma once

// Standard Libraries
#include <stdbool.h>

// Forward Declarations
typedef struct J J;

bool serialsensorActivate (int appID, void *appContext);
bool serialsensorInit (void);
void serialsensorISR (int appID, uint16_t pins, void *appContext);
void serialsensorPoll (int appID, int state, void *appContext);
void serialsensorResponse (int appID, J *rsp, void *appContext);
