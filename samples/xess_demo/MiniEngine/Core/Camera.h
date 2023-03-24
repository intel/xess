//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

// Modified 2022, Intel Corporation.
// Added support for camera jitter.

#pragma once

#include "VectorMath.h"
#include "Math/Frustum.h"

namespace Math
{
    class BaseCamera
    {
    public:

        // Call this function once per frame and after you've changed any state.  This
        // regenerates all matrices.  Calling it more or less than once per frame will break
        // temporal effects and cause unpredictable results.
        void Update();

        // Public functions for controlling where the camera is and its orientation
        void SetEyeAtUp( Vector3 eye, Vector3 at, Vector3 up );
        void SetLookDirection( Vector3 forward, Vector3 up );
        void SetRotation( Quaternion basisRotation );
        void SetPosition( Vector3 worldPos );
        void SetTransform( const AffineTransform& xform );
        void SetTransform( const OrthogonalTransform& xform );
        void SetReprojectMatrixJittered(bool Jittered) { m_IsReprojectMatrixJittered = Jittered; };
        
        const Quaternion GetRotation() const { return m_CameraToWorld.GetRotation(); }
        const Vector3 GetRightVec() const { return m_Basis.GetX(); }
        const Vector3 GetUpVec() const { return m_Basis.GetY(); }
        const Vector3 GetForwardVec() const { return -m_Basis.GetZ(); }
        const Vector3 GetPosition() const { return m_CameraToWorld.GetTranslation(); }

        // Accessors for reading the various matrices and frusta
        const Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
        const Matrix4& GetProjMatrix() const { return m_ProjMatrix; }
        const Matrix4& GetViewProjMatrix() const { return m_ViewProjMatrix; }
        const Matrix4& GetReprojectionMatrix() const { return m_ReprojectMatrix; }
        const Frustum& GetViewSpaceFrustum() const { return m_FrustumVS; }
        const Frustum& GetWorldSpaceFrustum() const { return m_FrustumWS; }

        bool IsReprojectMatrixJittered() const { return m_IsReprojectMatrixJittered; }

    protected:

        BaseCamera() : 
            m_CameraToWorld(kIdentity),
            m_Basis(kIdentity),
            m_ViewMatrix(kIdentity),
            m_ProjMatrix(kIdentity),
            m_ViewProjMatrix(kIdentity),
            m_ProjMatrixNoJitter(kIdentity),
            m_ViewProjMatrixNoJitter(kIdentity),
            m_PreviousViewProjMatrix(kIdentity),
            m_ReprojectMatrix(kIdentity),
            m_IsReprojectMatrixJittered(false)
        {
        }

        void SetProjMatrix( const Matrix4& ProjMat ) { m_ProjMatrixNoJitter = m_ProjMatrix = ProjMat; }

        OrthogonalTransform m_CameraToWorld;

        // Redundant data cached for faster lookups.
        Matrix3 m_Basis;

        // Transforms homogeneous coordinates from world space to view space.  In this case, view space is defined as +X is
        // to the right, +Y is up, and -Z is forward.  This has to match what the projection matrix expects, but you might
        // also need to know what the convention is if you work in view space in a shader.
        Matrix4 m_ViewMatrix;       // i.e. "World-to-View" matrix

        // The projection matrix transforms view space to clip space.  Once division by W has occurred, the final coordinates
        // can be transformed by the viewport matrix to screen space.  The projection matrix is determined by the screen aspect 
        // and camera field of view.  A projection matrix can also be orthographic.  In that case, field of view would be defined
        // in linear units, not angles.
        Matrix4 m_ProjMatrix;       // i.e. "View-to-Projection" matrix

        // A concatenation of the view and projection matrices.
        Matrix4 m_ViewProjMatrix;   // i.e.  "World-To-Projection" matrix.

        Matrix4 m_ProjMatrixNoJitter;       // m_ProjMatrix, but without jitter.
        Matrix4 m_ViewProjMatrixNoJitter;   // m_ViewProjMatrix, but without jitter.

        bool m_IsReprojectMatrixJittered;     // If Reprojection Matrix is jittered.

        // The view-projection matrix from the previous frame
        Matrix4 m_PreviousViewProjMatrix;

        // Projects a clip-space coordinate to the previous frame (useful for temporal effects).
        Matrix4 m_ReprojectMatrix;

        Frustum m_FrustumVS;		// View-space view frustum
        Frustum m_FrustumWS;		// World-space view frustum

    };

    class Camera : public BaseCamera
    {
    public:
        Camera();

        // Controls the view-to-projection matrix
        void SetPerspectiveMatrix( float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip );
        void SetFOV( float verticalFovInRadians ) { m_VerticalFOV = verticalFovInRadians; UpdateProjMatrix(); }
        void SetAspectRatio( float heightOverWidth ) { m_AspectRatio = heightOverWidth; UpdateProjMatrix(); }
        void SetZRange( float nearZ, float farZ) { m_NearClip = nearZ; m_FarClip = farZ; UpdateProjMatrix(); }
        void ReverseZ( bool enable ) { m_ReverseZ = enable; UpdateProjMatrix(); }

        float GetFOV() const { return m_VerticalFOV; }
        float GetNearClip() const { return m_NearClip; }
        float GetFarClip() const { return m_FarClip; }
        float GetClearDepth() const { return m_ReverseZ ? 0.0f : 1.0f; }

        // Set projection matrix with jitter.
        void SetProjectMatrixJitter(float ProjectionJitterX, float ProjectionJitterY);

    private:

        void UpdateProjMatrix( void );

        float m_VerticalFOV;	// Field of view angle in radians
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;
        bool m_ReverseZ;		// Invert near and far clip distances so that Z=1 at the near plane
        bool m_InfiniteZ;       // Move the far plane to infinity
    };

    inline void BaseCamera::SetEyeAtUp( Vector3 eye, Vector3 at, Vector3 up )
    {
        SetLookDirection(at - eye, up);
        SetPosition(eye);
    }

    inline void BaseCamera::SetPosition( Vector3 worldPos )
    {
        m_CameraToWorld.SetTranslation( worldPos );
    }

    inline void BaseCamera::SetTransform( const AffineTransform& xform )
    {
        // By using these functions, we rederive an orthogonal transform.
        SetLookDirection(-xform.GetZ(), xform.GetY());
        SetPosition(xform.GetTranslation());
    }

    inline void BaseCamera::SetRotation( Quaternion basisRotation )
    {
        m_CameraToWorld.SetRotation(Normalize(basisRotation));
        m_Basis = Matrix3(m_CameraToWorld.GetRotation());
    }

    inline Camera::Camera() : 
        BaseCamera(),
        m_ReverseZ(true),
        m_InfiniteZ(false)
    {
        SetPerspectiveMatrix( XM_PIDIV4, 9.0f / 16.0f, 1.0f, 1000.0f );
    }

    inline void Camera::SetPerspectiveMatrix( float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip )
    {
        m_VerticalFOV = verticalFovRadians;
        m_AspectRatio = aspectHeightOverWidth;
        m_NearClip = nearZClip;
        m_FarClip = farZClip;

        UpdateProjMatrix();

        m_PreviousViewProjMatrix = m_ViewProjMatrixNoJitter;
    }

} // namespace Math
