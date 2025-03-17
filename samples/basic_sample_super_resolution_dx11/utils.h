/*******************************************************************************
 * Copyright (C) 2021 Intel Corporation
 * 
 * This software and the related documents are Intel copyrighted materials, and
 * your use of them is governed by the express license under which they were
 * provided to you ("License"). Unless the License provides otherwise, you may
 * not use, modify, copy, publish, distribute, disclose or transmit this
 * software or the related documents without Intel's prior written permission.
 * 
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <utility>

namespace Utils
{
/**
 * Generates halton sequence with optional point shift using van der Corput sequence
 * @param base1 - base for first Corput sequence (X coordinate)
 * @param base2 - base for second Corput sequence (Y coordinate)
 * @param start_index - initial index for halton set
 * @param offset1 - offset for first Corput sequence
 * @param offset2 - offset for second Corput sequence
 * @return vector of pairs with two sets
 */
std::vector<std::pair<float, float>> GenerateHalton(std::uint32_t base1, std::uint32_t base2,
    std::uint32_t start_index, std::uint32_t count, float offset1 = -0.5f, float offset2 = -0.5f);
}
