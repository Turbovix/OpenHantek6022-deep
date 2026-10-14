// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "utils/printutils.h"
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <vector>

struct DSOsamples {
    std::vector< std::vector< double > > data; ///< Pointer to input data from device
    double samplerate = 0.0;                   ///< The samplerate of the input data
    unsigned char clipped = 0;                 ///< Bitmask of clipped channels
    bool liveTrigger = false;                  ///< live samples are triggered
    int triggeredPosition = 0;                 ///< position for a triggered trace, 0 = not triggered
    double pulseWidth1 = 0.0;                  ///< width from trigger point to next opposite slope
    double pulseWidth2 = 0.0;                  ///< width from next opposite slope to third slope
    Unit mathVoltageUnit = UNIT_VOLTS;         ///< unless UNIT_VOLTSQUARE for some math functions
    bool freeRunning = false;                  ///< trigger: NONE, half sample count
    unsigned tag = 0;                          ///< track individual sample blocks (debug support)
    mutable QReadWriteLock lock;
};

const int SAMPLESIZE = 20000; ///< minimal (and default) number of samples per acquisition
const int SAMPLESIZE_ROLL = 40 * 256;

/// Selectable memory depths (max. samples per channel and acquisition).
/// The acquisition uses only as many samples as needed for two screens at the
/// current samplerate, bounded by SAMPLESIZE and the selected memory depth.
inline const std::vector< unsigned > MEMORY_DEPTHS = { 20000, 50000, 100000, 200000, 500000, 1000000, 2000000, 4000000 };

/// Max. size of one USB bulk transfer. Mind the oversampling: one displayed sample
/// needs up to 200 raw samples at slow samplerates.
#ifdef Q_OS_LINUX
/// Linux limits the memory of all usbfs transfers to usbfs_memory_mb (default 16 MiB),
/// keep some margin. Gapless transfers of up to 16 MB were verified with a 6022BE.
const unsigned MAX_TRANSFER_BYTES = 15000000;
#else
/// Other platforms: stay with the 4 MB known to work without gaps.
const unsigned MAX_TRANSFER_BYTES = 4000000;
#endif

/// Longer records are averaged in blocks before the spectrum calculation (fft).
const int SPECTRUM_MAX_SAMPLES = 100000;
