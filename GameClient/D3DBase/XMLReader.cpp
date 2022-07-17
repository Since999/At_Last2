#include "Markup.h"
#include "2DShader.h"
#include "XMLReader.h"
#include "2DObject.h"
#include "SoundSystem.h"
map<wstring, int*> CXMLReader::variable_map;

const map<wstring, int*>& CXMLReader::GetVariable_map()
{
    if (!variable_map.empty()) return variable_map;
    variable_map.emplace(L"skill", &(Network::g_client[Network::my_id].special_skill));
    variable_map.emplace(L"left bullet", &(Network::g_client[Network::my_id].left_bullet));
    
    return variable_map;
}

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
    while (xml.FindElem(L"BAR")) {
        width = _wtof(xml.GetAttrib(L"width"));
        height = _wtof(xml.GetAttrib(L"height"));
        x = _wtof(xml.GetAttrib(L"x"));
        y = _wtof(xml.GetAttrib(L"y"));
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        ui->AddProgressBar(width, height, x, y, image_file_name);
    }
    GetNumberUI(xml, ui);
    
    return true;
}

void CXMLReader::GetNumberUI(CMarkup& xml, UISystem* ui)
{
    const auto& map = GetVariable_map();
    while (xml.FindElem(L"Number")) {
        float width = _wtof(xml.GetAttrib(L"width"));
        float height = _wtof(xml.GetAttrib(L"height"));
        float x = _wtof(xml.GetAttrib(L"x"));
        float y = _wtof(xml.GetAttrib(L"y"));
        int digit = _wtoi(xml.GetAttrib(L"digit"));
        wstring variable_name = std::wstring(xml.GetAttrib(L"variable").operator LPCWSTR());
        auto& found = map.find(variable_name);
        if (found != map.end()) {
            ui->AddNumberUI(width, height, x, y, digit, (*found).second);
        }
#ifdef _DEBUG
        else {
            cout << "Error (GetNumberUI): no such variable in map." << endl;
            wcout << "\tstring: " << variable_name << endl;
        }
#endif
    }
}

void CXMLReader::LoadParticle(const wstring& file_name, ParticleSystem* sys)
{
    CMarkup xml;
    CString strFileName = file_name.c_str();
    if (!xml.Load(strFileName))
    {
        return;
    }
    xml.FindElem(L"ParticleSystem");
    xml.IntoElem();
    while (xml.FindElem(L"Particle")) {
        float width;
        float height;
        float duration;
        wstring name;
        vector<CMaterial*>* materials = new vector<CMaterial*>;
        wstring image_file_name;
        name = wstring{ xml.GetAttrib(L"name") };
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
        xml.OutOfElem();
        materials->shrink_to_fit();
        sys->AddBuilder(name, duration, { width, height }, materials);
    }
    return;
}

void CXMLReader::LoadSound(const wstring& file_name, CSoundSystem* sys)
{
    CMarkup xml;
    CString strFileName = file_name.c_str();
    if (!xml.Load(strFileName))
    {
        return;
    }
    xml.FindElem(L"SoundSystem");
    xml.IntoElem();
    CString string_true { L"true" };
    while (xml.FindElem(L"Sound")) {
        wstring name;
        string sound_file_name;
        bool loop;
        wstring channel;
        name = wstring{ xml.GetAttrib(L"name") };
        sound_file_name = string{ CT2CA( xml.GetAttrib(L"sound")) };
        loop = xml.GetAttrib(L"loop") == string_true;
        channel = xml.GetAttrib(L"channel");
        sys->SetSound(name, sound_file_name, channel, loop);
    }
    return;
}