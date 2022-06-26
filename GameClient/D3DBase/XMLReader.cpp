#include "Markup.h"
#include "2DShader.h"
#include "XMLReader.h"
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
    while (xml.FindElem(L"UI")) {
        width = _wtoi(xml.GetAttrib(L"width"));
        height = _wtoi(xml.GetAttrib(L"height"));
        x = _wtoi(xml.GetAttrib(L"x"));
        y = _wtoi(xml.GetAttrib(L"y"));
        image_file_name = std::wstring(xml.GetAttrib(L"image").operator LPCWSTR());
        ui->AddUI(width, height, x, y, image_file_name);
    }
    return true;
}