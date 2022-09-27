/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "GameCore.h"
#include "CameraController.h"
#include "imgui.h"

/// Camera controller for the demo, handles user interactions.
class DemoCameraController : public FlyingFPSCamera
{
public:
    /// Constructor.
    DemoCameraController(Camera& Camera_, Vector3 WorldUp_);
    /// Update interface.
    void Update(float DeltaTime) override;

    /// Get heading, pitch and position values.
    void GetHeadingPitchAndPosition(float& Heading, float& Pitch, Vector3& Position);
    /// Set heading and pitch for the camera.
    void SetHeadingAndPitch(float Heading, float Pitch);
    /// Set camera position.
    void SetPosition(const Vector3& Position);

private:
    ImVec2 m_LastMousePos;
};
