// High Level API for other sub-systems

#ifndef ___HIGHLEVEL_H__
#define ___HIGHLEVEL_H__

#include "HLE.h"            // HLE init
#include "Symbols.h"        // symbolic information
#include "MapLoader.h"      // *.map file loader
#include "MapSaver.h"       // save current symbols in *.map file
#include "MapMaker.h"       // OS calls search engine
#include "Bootrom.h"        // bootrom simulation
#include "DVDBanner.h"      // banner utilities for selector
#include "DSP.h"            // GC audio DSP emulation
#include "PAD.h"            // GC controller library
#include "AX.h"             // AX library
#include "JAudio.h"         // JAudio library
#include "OS.h"             // Dolphin OS
#include "Stdc.h"           // std C runtime library calls
#include "Mtx.h"            // vector/matrix library

#endif  // ___HIGHLEVEL_H__
