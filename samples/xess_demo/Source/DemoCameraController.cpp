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

#include "pch.h"
#include "DemoCameraController.h"
#include "Camera.h"
#include "XeSS/XeSSDebug.h"

namespace Graphics
{
    extern EnumVar DebugZoom;
}

DemoCameraController::DemoCameraController(Camera& Camera_, Vector3 WorldUp)
    : FlyingFPSCamera(Camera_, WorldUp)
    , m_LastMousePos(0.0f, 0.0f)
{
    m_MoveSpeed = 500.0f;
    m_StrafeSpeed = 500.0f;
    m_MouseSensitivityX = 0.5f;
    m_MouseSensitivityY = 0.5f;
}

void DemoCameraController::Update(float DeltaTime)
{
    // Clamp delta time in case stutter happens.
    DeltaTime = Math::Clamp(DeltaTime, 0.0f, 0.2f);

    float timeScale = Graphics::DebugZoom == 0 ? 1.0f :
        Graphics::DebugZoom == 1 ? 0.5f : 0.25f;

    ImGuiIO& io = ImGui::GetIO();

    bool speedUp = io.KeyShift;

    float speedScale = (speedUp ? 2.0f : 1.0f) * timeScale;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float forward = 0.0f;
    float strafe = 0.0f;
    float ascent = 0.0f;

    // Update key interaction
    if (!io.WantCaptureKeyboard && !XeSSDebug::IsFrameDumpOn())
    {
        forward = m_MoveSpeed * speedScale * ((ImGui::IsKeyDown('W') ? DeltaTime : 0.0f) + (ImGui::IsKeyDown('S') ? -DeltaTime : 0.0f));
        strafe = m_StrafeSpeed * speedScale * ((ImGui::IsKeyDown('D') ? DeltaTime : 0.0f) + (ImGui::IsKeyDown('A') ? -DeltaTime : 0.0f));
        ascent = m_StrafeSpeed * speedScale * ((ImGui::IsKeyDown('E') ? DeltaTime : 0.0f) + (ImGui::IsKeyDown('Q') ? -DeltaTime : 0.0f));

        // Mouse wheel interaction
        if (!io.WantCaptureMouse)
        {
            const float mouseWheel = ImGui::GetIO().MouseWheel;
            if (mouseWheel > 0.0f)
            {
                forward += m_MoveSpeed * speedScale * DeltaTime * 3.0f;
            }
            else if (mouseWheel < 0.0f)
            {
                forward -= m_MoveSpeed * speedScale * DeltaTime * 3.0f;
            }
        }

        if (m_Momentum)
        {
            ApplyMomentum(m_LastYaw, yaw, DeltaTime);
            ApplyMomentum(m_LastPitch, pitch, DeltaTime);
            ApplyMomentum(m_LastForward, forward, DeltaTime);
            ApplyMomentum(m_LastStrafe, strafe, DeltaTime);
            ApplyMomentum(m_LastAscent, ascent, DeltaTime);
        }
    }

    // Update mouse interaction
    if (!io.WantCaptureMouse && !XeSSDebug::IsFrameDumpOn())
    {
        if (ImGui::IsAnyMouseDown())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);

            ImVec2 mousePos = ImGui::GetMousePos();

            ImVec4 bounds(2.0f, io.DisplaySize.x - 2.0f, 2.0f, io.DisplaySize.y - 2.0f);

            ImVec2 clampedPos = mousePos;
            if (clampedPos.x < bounds.x)
            {
                clampedPos.x = bounds.y;
            }
            else if (clampedPos.x > bounds.y)
            {
                clampedPos.x = bounds.x;
            }

            if (clampedPos.y < bounds.z)
            {
                clampedPos.y = bounds.w;
            }
            else if (clampedPos.y > bounds.w)
            {
                clampedPos.y = bounds.z;
            }

            if (clampedPos.x != mousePos.x || clampedPos.y != mousePos.y)
            {
                io.WantSetMousePos = true;
                io.MousePos = clampedPos;
            }
            else
            {
                io.WantSetMousePos = false;

                ImVec2 mouseDelta = ImVec2(clampedPos.x - m_LastMousePos.x, clampedPos.y - m_LastMousePos.y);

                yaw += mouseDelta.x * m_MouseSensitivityX * timeScale * 0.001f;
                pitch += -mouseDelta.y * m_MouseSensitivityY * timeScale * 0.001f;
            }
        }
    }

    // Correct pitch
    m_CurrentPitch += pitch;
    m_CurrentPitch = XMMin(XM_PIDIV2, m_CurrentPitch);
    m_CurrentPitch = XMMax(-XM_PIDIV2, m_CurrentPitch);

    // Add some debug camera move.
    if (XeSSDebug::IsFrameDumpOn())
    {
        XeSSDebug::UpdateCameraYaw(yaw);
    }

    // Correct yaw
    m_CurrentHeading -= yaw;
    if (m_CurrentHeading > XM_PI)
        m_CurrentHeading -= XM_2PI;
    else if (m_CurrentHeading <= -XM_PI)
        m_CurrentHeading += XM_2PI;

    // Update camera transform
    Matrix3 orientation = Matrix3(m_WorldEast, m_WorldUp, -m_WorldNorth) * Matrix3::MakeYRotation(m_CurrentHeading) * Matrix3::MakeXRotation(m_CurrentPitch);
    Vector3 position = orientation * Vector3(strafe, ascent, -forward) + m_TargetCamera.GetPosition();

    m_TargetCamera.SetTransform(AffineTransform(orientation, position));
    m_TargetCamera.Update();

    m_LastMousePos = ImGui::GetMousePos();
}

void DemoCameraController::GetHeadingPitchAndPosition(float& Heading, float& Pitch, Vector3& Position)
{
    Heading = m_CurrentHeading;
    Pitch = m_CurrentPitch;
    Position = m_TargetCamera.GetPosition();
}

void DemoCameraController::SetHeadingAndPitch(float Heading, float Pitch)
{
    m_CurrentHeading = Heading;
    m_CurrentPitch = Pitch;

    m_CurrentPitch = XMMin(XM_PIDIV2, m_CurrentPitch);
    m_CurrentPitch = XMMax(-XM_PIDIV2, m_CurrentPitch);
    if (m_CurrentHeading > XM_PI)
        m_CurrentHeading -= XM_2PI;
    else if (m_CurrentHeading <= -XM_PI)
        m_CurrentHeading += XM_2PI;

    Quaternion rotation(Pitch, Heading, 0.0f);
    m_TargetCamera.SetRotation(rotation);

    m_TargetCamera.Update();
}

void DemoCameraController::SetPosition(const Vector3& Position)
{
    m_TargetCamera.SetPosition(Position);
    m_TargetCamera.Update();
}
