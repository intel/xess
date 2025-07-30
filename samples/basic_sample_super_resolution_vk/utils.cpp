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

#include "utils.h"

namespace
{
    float GetCorput(std::uint32_t index, std::uint32_t base)
    {
        float result = 0;
        float bk = 1.f;

        while (index > 0) 
        {
            bk /= (float)base;
            result += (float)(index % base) * bk;
            index /= base;
        }

        return result;
    }
}


std::vector<std::pair<float, float>> Utils::GenerateHalton(std::uint32_t base1, std::uint32_t base2,
    std::uint32_t start_index, std::uint32_t count, float offset1, float offset2)
{
    std::vector<std::pair<float, float>> result;
    result.reserve(count);

    for (std::uint32_t a = start_index; a < count + start_index; ++a)
    {
        result.emplace_back(GetCorput(a, base1) + offset1, GetCorput(a, base2) + offset2);
    }
    return result;
}
