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
#include "Camera.h"
#include "Model.h"
#include "ShadowCamera.h"
#include "DemoExtraBuffers.h"
#include "DemoLog.h"

class CameraController;

enum eDemoTechnique
{
    kDemoTech_XeSS = 0,
    kDemoTech_TAANative,
    kDemoTech_TAAScaled,
    kDemoTech_Num
};

/// XeSS Demo application class.
class DemoApp : public GameCore::IGameApp
{
public:
    /// Constructor.
    DemoApp();
    /// Destructor.
    ~DemoApp();

    /// Called when application starting up.
    virtual void Startup() override;
    /// Called when application finalizing.
    virtual void Cleanup() override;
    /// Called in the update phase in the game loop.
    virtual void Update(float deltaT) override;

    /// Called after Update in the game loop.
    virtual void RenderScene() override;
    /// Called at the end of the rendering.
    virtual void RenderUI(GraphicsContext& Context) override;

    /// Get demo technique.
    eDemoTechnique GetTechnique() const;
    /// Set demo technique.
    void SetTechnique(eDemoTechnique tech);

    /// Get camera controller.
    CameraController* GetCameraController() const;

    ///  Get Log Object
    DemoLog& GetLog();

    /// Find root directory of assets.
    bool FindAssetsDir();

private:
    /// Handle display resolution change.
    void UpdateResolution();

    /// Load IBL textures for the renderer.
    void LoadIBLTextures();

    /// Render scene
    void RenderSceneImpl(GraphicsContext& gfxContext);

    /// Log object.
    DemoLog m_Log;
    /// Camera object.
    Math::Camera m_Camera;
    /// Shadow camera of the sun.
    ShadowCamera m_SunShadowCamera;
    /// Camera controller object, handles user interactions.
    std::unique_ptr<CameraController> m_CameraController;
    /// Viewport for scene rendering.
    D3D12_VIEWPORT m_MainViewport;
    /// Scissor for scene rendering.
    D3D12_RECT m_MainScissor;
    /// Extra buffers handler.
    /// It takes care of creation and destruction of extra buffers when engine handles the default buffers.
    DemoExtraBuffersHandler m_ExtraBuffersHandler;
    /// If we should show UI.
    bool m_ShowUI;
    /// Current technique.
    eDemoTechnique m_Technique;
    /// Model of the scene.
    ModelInstance m_ModeInstance;
    /// Root assets folder
    std::wstring m_AssetRootDir;
};
