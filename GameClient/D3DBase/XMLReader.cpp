#include "Markup.h"
#include "2DShader.h"
#include "XMLReader.h"
#include "2DObject.h"
#include "SoundSystem.h"
#include "GameFramework.h"
#include "Configuration.h"

map<wstring, void*> CXMLReader::variable_map;
map<wstring, function<void()>> CXMLReader::function_map;

const map<wstring, void*>& CXMLReader::GetVariableMap()
{
    if (!variable_map.empty()) return variable_map;
    variable_map.emplace(L"skill", &(Network::g_client[Network::my_id].special_skill));
    variable_map.emplace(L"left bullet", &(Network::g_client[Network::my_id].left_bullet));
    variable_map.emplace(L"kill", &CGameFramework::GetInstance()->zombie_killed);
    variable_map.emplace(L"left zombies", &CGameFramework::GetInstance()->left_zombie);
    variable_map.emplace(L"ip", &IP_ADDRESS::SERVER_IP);
    
    return variable_map;
}
const map<wstring, function<void()>>& CXMLReader::GetFunctionMap()
{
    if (!function_map.empty()) return function_map;
    auto framework = CGameFramework::GetInstance();
    auto sig = framework->GetCurruntScene()->GetGraphicsRootSignature();
    auto scene = framework->GetCurruntScene();
    
    CSelectScene* sel_scene = dynamic_cast<CSelectScene*>(scene);
    function_map.emplace(L"login", [framework, sig]() {
        framework->ui_system->GetIPUI()->SetIP();
        //thread Timer_thread{ network.Do_Timer };

        network.worker_threads.emplace_back(network.Work);
        //network.Login();
        framework->ChangeScene(new CSelectScene(sig));
    });
    function_map.emplace(L"game start", [framework, sig]() {
        framework->ChangeScene(new CMainGameScene(sig));
    });
    function_map.emplace(L"select player 1", []() {
        auto framework = CGameFramework::GetInstance();
        auto ui = framework->ui_system;
        network.Player_Select(PlayerType::COMMANDER);
        ui->EnableButton(L"select player 1", false);
        ui->EnableButton(L"select player 2", false);
        ui->EnableButton(L"select player 3", false);
    });
    function_map.emplace(L"select player 2", []() {
        auto framework = CGameFramework::GetInstance();
        auto ui = framework->ui_system;
        network.Player_Select(PlayerType::ENGINEER);
        ui->EnableButton(L"select player 1", false);
        ui->EnableButton(L"select player 2", false);
        ui->EnableButton(L"select player 3", false);    
    });
    function_map.emplace(L"select player 3", []() {
        auto framework = CGameFramework::GetInstance();
        auto ui = framework->ui_system;
        network.Player_Select(PlayerType::MERCENARY);
        ui->EnableButton(L"select player 1", false);
        ui->EnableButton(L"select player 2", false);
        ui->EnableButton(L"select player 3", false);    
    });
    
    return function_map;
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
    xml.ResetPos();
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    while (xml.FindElem(L"BAR")) {
        width = _wtof(xml.GetAttrib(L"width"));
        height = _wtof(xml.GetAttrib(L"height"));
        x = _wtof(xml.GetAttrib(L"x"));
        y = _wtof(xml.GetAttrib(L"y"));
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        ui->AddProgressBar(width, height, x, y, image_file_name);
    }
    GetNumberUI(xml, ui);
    GetButtonUI(xml, ui);
    GetPopupUI(xml, ui);
    GetIPUI(xml, ui);
    return true;
}

void CXMLReader::GetNumberUI(CMarkup& xml, UISystem* ui)
{
    xml.ResetPos();
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    const auto& map = GetVariableMap();
    while (xml.FindElem(L"Number")) {
        float width = _wtof(xml.GetAttrib(L"width"));
        float height = _wtof(xml.GetAttrib(L"height"));
        float x = _wtof(xml.GetAttrib(L"x"));
        float y = _wtof(xml.GetAttrib(L"y"));
        int digit = _wtoi(xml.GetAttrib(L"digit"));
        wstring variable_name = std::wstring(xml.GetAttrib(L"variable").operator LPCWSTR());
        auto& found = map.find(variable_name);
        if (found == map.end()) {
#ifdef _DEBUG
        cout << "Error (GetNumberUI): no such variable in map." << endl;
        wcout << "\tstring: " << variable_name << endl;
#endif
            continue; 
        }
        ui->AddNumberUI(width, height, x, y, digit, (int*)(*found).second);

    }
}

void CXMLReader::GetButtonUI(CMarkup& xml, UISystem* ui)
{
    auto& func_map = GetFunctionMap();
    float width;
    float height;
    float x;
    float y;
    wstring image_file_name;
    wstring func_name;
    xml.ResetPos();
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    while (xml.FindElem(L"Button")) {
        width = _wtof(xml.GetAttrib(L"width"));
        height = _wtof(xml.GetAttrib(L"height"));
        x = _wtof(xml.GetAttrib(L"x"));
        y = _wtof(xml.GetAttrib(L"y"));
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        func_name = std::wstring(xml.GetAttrib(L"action").operator LPCWSTR());
        auto& found = func_map.find(func_name);
        if (found == func_map.end()) {
#ifdef _DEBUG
            cout << "Error (GetButtonUI): no such function in map." << endl;
            wcout << "\tstring: " << func_name << endl;
#endif
            ui->AddButtonUI(width, height, x, y, image_file_name, []() {}, func_name);
            continue;
        }
        ui->AddButtonUI(width, height, x, y, image_file_name, (*found).second, func_name);
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

void CXMLReader::LoadPlayerModelName(const wstring& file_name)
{
    CMarkup xml;
    CString strFileName = file_name.c_str();
    if (!xml.Load(strFileName))
    {
        return;
    }
    xml.FindElem(L"PlayerModel");
    xml.IntoElem();
    while (xml.FindElem(L"Player")) {
        wstring name;
        string sound_file_name;
        bool loop;
        wstring channel;
        name = wstring{ xml.GetAttrib(L"name") };
        string model = string{ CT2CA(xml.GetAttrib(L"model")) };
        wstring texture = wstring{ xml.GetAttrib(L"texture") };

        channel = xml.GetAttrib(L"channel");
        CConfiguration::SetModel(model, texture);
    }
    return;
}

void CXMLReader::GetPopupUI(CMarkup& xml, UISystem* ui)
{
    xml.ResetPos();
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    const auto& map = GetVariableMap();
    while (xml.FindElem(L"Popup")) {
        float width = _wtof(xml.GetAttrib(L"width"));
        float height = _wtof(xml.GetAttrib(L"height"));
        float x = _wtof(xml.GetAttrib(L"x"));
        float y = _wtof(xml.GetAttrib(L"y"));
        float duration = _wtof(xml.GetAttrib(L"duration"));
        wstring image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        ui->AddPopupUI(width, height, x, y, image_file_name, duration);
    }
}

void CXMLReader::GetIPUI(CMarkup& xml, UISystem* ui)
{
    xml.ResetPos();
    xml.FindElem(L"UISystem");
    xml.IntoElem();
    const auto& map = GetVariableMap();
    while (xml.FindElem(L"IP")) {
        float width = _wtof(xml.GetAttrib(L"width"));
        float height = _wtof(xml.GetAttrib(L"height"));
        float x = _wtof(xml.GetAttrib(L"x"));
        float y = _wtof(xml.GetAttrib(L"y"));
        wstring variable_name = std::wstring(xml.GetAttrib(L"variable").operator LPCWSTR());
        auto& found = map.find(variable_name);
        if (found == map.end()) {
#ifdef _DEBUG
            cout << "Error (GetNumberUI): no such variable in map." << endl;
            wcout << "\tstring: " << variable_name << endl;
#endif
            continue;
        }
        ui->AddIPUI(width, height, x, y, (int*)(*found).second);

    }
}