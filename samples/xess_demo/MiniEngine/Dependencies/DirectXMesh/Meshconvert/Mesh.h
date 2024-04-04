//--------------------------------------------------------------------------------------
// File: Mesh.h
//
// Mesh processing helper class
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
// http://go.microsoft.com/fwlink/?LinkID=512686
//--------------------------------------------------------------------------------------

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>


#include <d3d11_1.h>

#include <DirectXMath.h>

#include "DirectXMesh.h"

class Mesh
{
public:
    Mesh() noexcept : mnFaces(0), mnVerts(0) {}
    Mesh(Mesh&& moveFrom) noexcept;
    Mesh& operator= (Mesh&& moveFrom) noexcept;

    Mesh(Mesh const&) = delete;
    Mesh& operator= (Mesh const&) = delete;

    // Methods
    void Clear() noexcept;

    void SetMTLFileName(const std::wstring& name) noexcept { mtlFileName = name; }

    HRESULT SetIndexData(_In_ size_t nFaces, _In_reads_(nFaces * 3) const uint16_t* indices, _In_reads_opt_(nFaces) const uint32_t* attributes = nullptr) noexcept;
    HRESULT SetIndexData(_In_ size_t nFaces, _In_reads_(nFaces * 3) const uint32_t* indices, _In_reads_opt_(nFaces) const uint32_t* attributes = nullptr) noexcept;

    HRESULT SetVertexData(const DirectX::VBReader& reader, _In_ size_t nVerts) noexcept;

    HRESULT Validate(_In_ DirectX::VALIDATE_FLAGS flags, _In_opt_ std::wstring* msgs) const noexcept;

    HRESULT Clean() noexcept;

    HRESULT GenerateAdjacency(_In_ float epsilon) noexcept;

    HRESULT ComputeNormals(_In_ DirectX::CNORM_FLAGS flags) noexcept;

    HRESULT ComputeTangentFrame(_In_ bool bitangents) noexcept;

    HRESULT Optimize(bool lru) noexcept;

    HRESULT ReverseWinding() noexcept;

    HRESULT InvertUTexCoord() noexcept;
    HRESULT InvertVTexCoord() noexcept;

    HRESULT ReverseHandedness() noexcept;

    // Accessors
    const uint32_t* GetAttributeBuffer() const noexcept { return mAttributes.get(); }
    const uint32_t* GetAdjacencyBuffer() const noexcept { return mAdjacency.get(); }
    const DirectX::XMFLOAT3* GetPositionBuffer() const noexcept { return mPositions.get(); }
    const DirectX::XMFLOAT3* GetNormalBuffer() const noexcept { return mNormals.get(); }
    const DirectX::XMFLOAT2* GetTexCoordBuffer() const noexcept { return mTexCoords.get(); }
    const DirectX::XMFLOAT4* GetTangentBuffer() const noexcept { return mTangents.get(); }
    const DirectX::XMFLOAT4* GetColorBuffer() const noexcept { return mColors.get(); }

    size_t GetFaceCount() const noexcept { return mnFaces; }
    size_t GetVertexCount() const noexcept { return mnVerts; }

    bool Is16BitIndexBuffer() const noexcept;

    const uint32_t* GetIndexBuffer() const noexcept { return mIndices.get(); }
    std::unique_ptr<uint16_t[]> GetIndexBuffer16() const noexcept;

    HRESULT GetVertexBuffer(const DirectX::VBWriter& writer) const noexcept;

    // Save mesh to file
    struct Material
    {
        std::wstring        name;
        bool                perVertexColor;
        float               specularPower;
        float               alpha;
        DirectX::XMFLOAT3   ambientColor;
        DirectX::XMFLOAT3   diffuseColor;
        DirectX::XMFLOAT3   specularColor;
        DirectX::XMFLOAT3   emissiveColor;
        std::wstring        texture;
        std::wstring        normalTexture;
        std::wstring        specularTexture;
        std::wstring        emissiveTexture;
        std::wstring        rmaTexture;

        Material() noexcept :
            perVertexColor(false),
            specularPower(1.f),
            alpha(1.f),
            ambientColor{},
            diffuseColor{},
            specularColor{},
            emissiveColor{}
        {
        }

        Material(
            const wchar_t* iname,
            bool pvc,
            float power,
            float ialpha,
            const DirectX::XMFLOAT3& ambient,
            const DirectX::XMFLOAT3 diffuse,
            const DirectX::XMFLOAT3& specular,
            const DirectX::XMFLOAT3& emissive,
            const wchar_t* txtname) :
            name(iname),
            perVertexColor(pvc),
            specularPower(power),
            alpha(ialpha),
            ambientColor(ambient),
            diffuseColor(diffuse),
            specularColor(specular),
            emissiveColor(emissive),
            texture(txtname)
        {
        }
    };

    HRESULT ExportToOBJ(const wchar_t* szFileName, _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials) const;
    HRESULT ExportToVBO(_In_z_ const wchar_t* szFileName) const noexcept;
    HRESULT ExportToCMO(_In_z_ const wchar_t* szFileName, _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials) const noexcept;
    HRESULT ExportToSDKMESH(_In_z_ const wchar_t* szFileName,
        _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials,
        bool force32bit = false,
        bool version2 = false,
        DXGI_FORMAT normalFormat = DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT uvFormat = DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT colorFormat = DXGI_FORMAT_B8G8R8A8_UNORM) const noexcept;

    // Create mesh from file
    static HRESULT CreateFromVBO(_In_z_ const wchar_t* szFileName, _Inout_ std::unique_ptr<Mesh>& result) noexcept;

private:
    size_t                                      mnFaces;
    size_t                                      mnVerts;
    std::unique_ptr<uint32_t[]>                 mIndices;
    std::unique_ptr<uint32_t[]>                 mAttributes;
    std::unique_ptr<uint32_t[]>                 mAdjacency;
    std::unique_ptr<DirectX::XMFLOAT3[]>        mPositions;
    std::unique_ptr<DirectX::XMFLOAT3[]>        mNormals;
    std::unique_ptr<DirectX::XMFLOAT4[]>        mTangents;
    std::unique_ptr<DirectX::XMFLOAT3[]>        mBiTangents;
    std::unique_ptr<DirectX::XMFLOAT2[]>        mTexCoords;
    std::unique_ptr<DirectX::XMFLOAT4[]>        mColors;
    std::unique_ptr<DirectX::XMFLOAT4[]>        mBlendIndices;
    std::unique_ptr<DirectX::XMFLOAT4[]>        mBlendWeights;

    std::wstring                                mtlFileName;

    void ExportToOBJ(std::wostream& os, _In_ size_t nMaterials, _In_reads_opt_(nMaterials) const Material* materials) const;
};
