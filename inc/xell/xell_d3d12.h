/*******************************************************************************
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/
#pragma once
#include <d3d12.h>
#include "xell.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the XeLL DX12 context .
 * @param[in] device: DX12 device
 * @param[out] out_context: Returned XeLL context handle.
 * @return XeLL return status code.
 */
XELL_EXPORT xell_result_t xellD3D12CreateContext(ID3D12Device* device, xell_context_handle_t* out_context);

#ifdef __cplusplus
}
#endif
