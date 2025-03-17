/*******************************************************************************
 * Copyright (C) 2024 Intel Corporation
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

#ifndef XEFG_SWAPCHAIN_DEBUG_H
#define XEFG_SWAPCHAIN_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xefg_swapchain.h"

/**
 * @brief XeSS FG Swap Chain debug features list.
 */
typedef enum _xefg_swapchain_debug_feature_t
{
    /** Present only interpolated frames. If interpolation fails, current back buffer will be presented. */
    XEFG_SWAPCHAIN_DEBUG_FEATURE_SHOW_ONLY_INTERPOLATION = 0,
    /** Adds a quads in the corners of interpolated frame. */
    XEFG_SWAPCHAIN_DEBUG_FEATURE_TAG_INTERPOLATED_FRAMES = 1,
    /** Present black image instead of skipping failed interpolation. */
    XEFG_SWAPCHAIN_DEBUG_FEATURE_PRESENT_FAILED_INTERPOLATION = 2,
    XEFG_SWAPCHAIN_DEBUG_FEATURE_RES_COUNT
} xefg_swapchain_debug_feature_t;

/**
 * @addtogroup xefgswapchain_debug XeSS FG Swap Chain debug features
 * @{
 */

/**
 * @brief Controls for debug features of XeSS FG Swap Chain API library.
 *
 * @param hSwapChain The XeSS FG Swap Chain context handle.
 *
 * @param featureId The debug feature to enable or disable.
 *
 * @param enable Non-zero to enable the feature, zero to disable it.
 *
 * @param pArgument Feature-defined arguments. Please refer to the debug feature documentation.
 *
 * @return XeSS FG Swap Chain return status code.
 */
XEFG_SWAPCHAIN_API xefg_swapchain_result_t xefgSwapChainEnableDebugFeature(xefg_swapchain_handle_t hSwapChain,
    xefg_swapchain_debug_feature_t featureId, uint32_t enable, void* pArgument);

/** @}*/

/* Enum size checks. All enums must be 4 bytes */
XEFG_SWAPCHAIN_STATIC_ASSERT(sizeof(xefg_swapchain_debug_feature_t) == 4);

#ifdef __cplusplus
}
#endif

#endif  /* XEFG_SWAPCHAIN_DEBUG_H */
