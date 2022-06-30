#include "Markup.h"
#include "2DShader.h"
#include "XMLReader.h"
#include "2DObject.h"

bool CXMLReader::GetUISetting(const string& file_name, UISystem* ui)
{
    CMarkup xml;
    CString strFileName = file_name.c_str();
    if (!xml.Load(strFileName))
    {
        return false;
    }
    float width;
    float height;
    float x;
    float y;
    wstring image_file_name;
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    while (xml.FindElem(L"UI")) {
        width = _wtof(xml.GetAttrib(L"width"));
        height = _wtof(xml.GetAttrib(L"height"));
        x = _wtof(xml.GetAttrib(L"x"));
        y = _wtof(xml.GetAttrib(L"y"));
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        ui->AddUI(width, height, x, y, image_file_name);
    }
    return true;
}


CParticleBuilder CXMLReader::LoadParticle(const wstring& file_name, ParticleSystem* sys)
{
    CMarkup xml;
    CString strFileName = file_name.c_str();
    if (!xml.Load(strFileName))
    {
        return CParticleBuilder(0, {0, 0}, NULL);
    }
    float width;
    float height;
    float duration;
    vector<CMaterial*>* materials = new vector<CMaterial*>;
    wstring image_file_name;
    xml.FindElem(L"Particle");
    width = _wtof(xml.GetAttrib(L"width"));
    height = _wtof(xml.GetAttrib(L"height"));
    duration = _wtof(xml.GetAttrib(L"duration"));
    xml.IntoElem();
    while (xml.FindElem(L"Image")) {
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        CTexture* texture = sys->GetTexture(image_file_name);
        CMaterial* mat = new CMaterial();
        mat->SetTexture(texture);
        materials->push_back(mat);
    }
    materials->shrink_to_fit();
    CParticleBuilder builder{ duration, { width, height }, materials };
    return builder;
}