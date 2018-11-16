/*
 * Interfaces over Yamaha OPN2 (YM2612) chip emulators
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

#ifndef OPN_CHIP_FAMILY_H
#define OPN_CHIP_FAMILY_H

enum OPNFamily
{
    OPNChip_OPN2,
    OPNChip_OPNA
};

template <OPNFamily F>
struct OPNFamilyTraits;

template <>
struct OPNFamilyTraits<OPNChip_OPN2>
{
    enum {
        nativeRate = 53267,
        nativeClockRate = 7670454,
    };
};

template <>
struct OPNFamilyTraits<OPNChip_OPNA>
{
    enum {
        nativeRate = 55466,
        nativeClockRate = 7987200,
    };
};

#endif // OPN_CHIP_FAMILY_H