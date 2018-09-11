/*
 * Interfaces over Yamaha OPL3 (YMF262) chip emulators
 *
 * Copyright (C) 2017-2018 Vitaly Novichkov (Wohlstand)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef DOSBOX_OPL3_H
#define DOSBOX_OPL3_H

#include "opl_chip_base.h"

class DosBoxOPL3 final : public OPLChipBaseBufferedT<DosBoxOPL3>
{
    void *m_chip;
public:
    DosBoxOPL3();
    ~DosBoxOPL3() override;

    bool canRunAtPcmRate() const override { return true; }
    void setRate(uint32_t rate) override;
    void reset() override;
    void writeReg(uint16_t addr, uint8_t data) override;
    void nativePreGenerate() override {}
    void nativePostGenerate() override {}
    void nativeGenerateN(int16_t *output, size_t frames) override;
    const char *emulatorName() override;
};

#endif // DOSBOX_OPL3_H
