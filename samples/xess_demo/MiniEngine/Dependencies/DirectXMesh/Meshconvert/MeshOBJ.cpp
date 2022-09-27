//--------------------------------------------------------------------------------------
// File: MeshOBJ.cpp
//
// Helper code for loading Mesh data from Wavefront OBJ
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
// http://go.microsoft.com/fwlink/?LinkID=512686
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include "Mesh.h"
#include "WaveFrontReader.h"

#include <cwchar>
#include <iterator>
#include <locale>
#include <new>

using namespace DirectX;

namespace
{
    std::wstring ProcessTextureFileName(const wchar_t* inName, bool dds)
    {
        if (!inName || !*inName)
            return std::wstring();

        wchar_t txext[_MAX_EXT] = {};
        wchar_t txfname[_MAX_FNAME] = {};
        _wsplitpath_s(inName, nullptr, 0, nullptr, 0, txfname, _MAX_FNAME, txext, _MAX_EXT);

        if (dds)
        {
            wcscpy_s(txext, L".dds");
        }

        wchar_t texture[_MAX_PATH] = {};
        _wmakepath_s(texture, nullptr, nullptr, txfname, txext);
        return std::wstring(texture);
    }
}

//--------------------------------------------------------------------------------------
HRESULT LoadFromOBJ(
    const wchar_t* szFilename,
    std::unique_ptr<Mesh>& inMesh,
    std::vector<Mesh::Material>& inMaterial,
    bool ccw,
    bool dds)
{
    WaveFrontReader<uint32_t> wfReader;
    HRESULT hr = wfReader.Load(szFilename, ccw);
    if (FAILED(hr))
        return hr;

    inMesh.reset(new (std::nothrow) Mesh);
    if (!inMesh)
        return E_OUTOFMEMORY;

    if (wfReader.indices.empty() || wfReader.vertices.empty())
        return E_FAIL;

    hr = inMesh->SetIndexData(wfReader.indices.size() / 3, wfReader.indices.data(),
        wfReader.attributes.empty() ? nullptr : wfReader.attributes.data());
    if (FAILED(hr))
        return hr;

    static const D3D11_INPUT_ELEMENT_DESC s_vboLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    static const D3D11_INPUT_ELEMENT_DESC s_vboLayoutAlt[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    const D3D11_INPUT_ELEMENT_DESC* layout = s_vboLayout;
    size_t nDecl = std::size(s_vboLayout);

    if (!wfReader.hasNormals && !wfReader.hasTexcoords)
    {
        nDecl = 1;
    }
    else if (wfReader.hasNormals && !wfReader.hasTexcoords)
    {
        nDecl = 2;
    }
    else if (!wfReader.hasNormals && wfReader.hasTexcoords)
    {
        layout = s_vboLayoutAlt;
        nDecl = std::size(s_vboLayoutAlt);
    }

    VBReader vbr;
    hr = vbr.Initialize(layout, nDecl);
    if (FAILED(hr))
        return hr;

    hr = vbr.AddStream(wfReader.vertices.data(), wfReader.vertices.size(), 0, sizeof(WaveFrontReader<uint32_t>::Vertex));
    if (FAILED(hr))
        return hr;

    hr = inMesh->SetVertexData(vbr, wfReader.vertices.size());
    if (FAILED(hr))
        return hr;

    if (!wfReader.materials.empty())
    {
        inMaterial.clear();
        inMaterial.reserve(wfReader.materials.size());

        for (const auto& it : wfReader.materials)
        {
            Mesh::Material mtl = {};

            mtl.name = it.strName;
            mtl.specularPower = (it.bSpecular) ? float(it.nShininess) : 1.f;
            mtl.alpha = it.fAlpha;
            mtl.ambientColor = it.vAmbient;
            mtl.diffuseColor = it.vDiffuse;
            mtl.specularColor = (it.bSpecular) ? it.vSpecular : XMFLOAT3(0.f, 0.f, 0.f);
            mtl.emissiveColor = (it.bEmissive) ? it.vEmissive : XMFLOAT3(0.f, 0.f, 0.f);

            mtl.texture = ProcessTextureFileName(it.strTexture, dds);
            mtl.normalTexture = ProcessTextureFileName(it.strNormalTexture, dds);
            mtl.specularTexture = ProcessTextureFileName(it.strSpecularTexture, dds);
            if (it.bEmissive)
            {
                mtl.emissiveTexture = ProcessTextureFileName(it.strEmissiveTexture, dds);
            }
            mtl.rmaTexture = ProcessTextureFileName(it.strRMATexture, dds);

            inMaterial.push_back(mtl);
        }
    }

    if (wfReader.materials.size() > 1)
    {
        inMesh->SetMTLFileName(wfReader.name);
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT Mesh::ExportToOBJ(const wchar_t* szFileName, size_t nMaterials, const Material* materials) const
{
    if (!szFileName)
        return E_INVALIDARG;

    if (nMaterials > 0 && !materials)
        return E_INVALIDARG;

    std::wofstream os;
    os.open(szFileName);
    if (!os)
        return E_FAIL;

    os << L"# " << szFileName << std::endl << L"#" << std::endl << std::endl;

    ExportToOBJ(os, nMaterials, materials);

    os.close();

    return (os.bad()) ? E_FAIL : S_OK;
}

_Use_decl_annotations_
void Mesh::ExportToOBJ(std::wostream& os, size_t nMaterials, const Material* materials) const
{
    os.imbue(std::locale::classic());

    if (!mtlFileName.empty())
        os << L"mtllib ./" << mtlFileName << L".mtl" << std::endl;

    for (size_t vert = 0; vert < mnVerts; ++vert)
    {
        os << L"v " << mPositions[vert].x << L" " << mPositions[vert].y << L" " << mPositions[vert].z << std::endl;
    }
    os << std::endl;

    if (mTexCoords)
    {
        for (size_t vert = 0; vert < mnVerts; ++vert)
        {
            os << L"vt " << mTexCoords[vert].x << L" " << mTexCoords[vert].y << std::endl;
        }
        os << std::endl;
    }

    if (mNormals)
    {
        for (size_t vert = 0; vert < mnVerts; ++vert)
        {
            os << L"vn " << mNormals[vert].x << L" " << mNormals[vert].y << L" " << mNormals[vert].z << std::endl;
        }
        os << std::endl;
    }

    // Using the first material entry as they are all the same for our use cases
    if (!materials || !mAttributes)
    {
        os << L"usemtl default" << std::endl;
    }

    /// Now the faces, a face is the first 3 indexes in indexes on the faces vertex
    uint32_t lastAttribute = uint32_t(-1);
    for (size_t face = 0; face < mnFaces; ++face)
    {
        if (mAttributes && mAttributes[face] != lastAttribute)
        {
            lastAttribute = mAttributes[face];
            if (lastAttribute < nMaterials)
            {
                os << L"usemtl " << materials[lastAttribute].name << std::endl;
            }
        }

        os << L"f ";
        for (size_t point = 0; point < 3; ++point)
        {
            const uint32_t i = mIndices[face * 3 + point] + 1;

            os << i << L"/";
            if (mTexCoords)
                os << i;
            os << L"/";
            if (mNormals)
                os << i;
            os << L" ";
        }
        os << std::endl;
    }
}
