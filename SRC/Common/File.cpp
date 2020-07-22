#include "pch.h"

namespace Util
{
    size_t FileSize(std::wstring& filename)
    {
        FILE* f;
        _wfopen_s(&f, filename.c_str(), L"rb");
        if (!f)
            return 0;

        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fclose(f);

        return size;
    }

    size_t FileSize(std::string& filename)
    {
        std::wstring wstr = StringToWstring(filename);
        return FileSize(wstr);
    }

    size_t FileSize(TCHAR* filename)
    {
        std::wstring wstr = TcharToWstring(filename);
        return FileSize(wstr);
    }

    bool FileExists(std::wstring& filename)
    {
        FILE* f;
        _wfopen_s(&f, filename.c_str(), L"rb");
        if (!f)
            return false;
        fclose(f);
        return true;
    }

    bool FileExists(std::string& filename)
    {
        std::wstring wstr = StringToWstring(filename);
        return FileExists(wstr);
    }

    bool FileExists(TCHAR* filename)
    {
        std::wstring wstr = TcharToWstring(filename);
        return FileExists(wstr);
    }

    std::vector<uint8_t> FileLoad(std::wstring& filename)
    {
        if (!FileExists(filename))
        {
            return std::vector<uint8_t>();
        }
        
        size_t size = FileSize(filename);

        uint8_t* data = new uint8_t[size];

        FILE* f; 
        _wfopen_s(&f, filename.c_str(), L"rb");

        fread(data, 1, size, f);
        fclose(f);

        return std::vector<uint8_t>(data, data + size);
    }

    std::vector<uint8_t> FileLoad(std::string& filename)
    {
        std::wstring wstr = StringToWstring(filename);
        return FileLoad(wstr);
    }

    std::vector<uint8_t> FileLoad(TCHAR* filename)
    {
        std::wstring wstr = TcharToWstring(filename);
        return FileLoad(wstr);
    }

    bool FileSave(std::wstring& filename, std::vector<uint8_t>& data)
    {
        FILE* f;
        _wfopen_s(&f, filename.c_str(), L"wb");
        if (!f)
            return false;

        fwrite(data.data(), 1, data.size(), f);
        fclose(f);

        return true;
    }

    bool FileSave(std::string& filename, std::vector<uint8_t>& data)
    {
        std::wstring wstr = StringToWstring(filename);
        return FileSave(wstr, data);
    }

    bool FileSave(TCHAR* filename, std::vector<uint8_t>& data)
    {
        std::wstring wstr = TcharToWstring(filename);
        return FileSave(wstr, data);
    }

}
