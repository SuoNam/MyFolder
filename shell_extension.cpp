#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <initguid.h>
#include <new>
#include <algorithm>
#include <utility>
#include <vector>

// The root CLSID is also declared in packaging/windows/AppxManifest.xml.
DEFINE_GUID(CLSID_MyFolderRootCommand,
            0x43e352b9, 0x94a1, 0x4d43, 0xa1, 0x1f, 0xa5, 0x72, 0x91, 0x56, 0x35, 0x70);
DEFINE_GUID(CLSID_MyFolderUploadCommand,
            0xbeb951dc, 0xb680, 0x49ae, 0x81, 0xb0, 0x6d, 0x07, 0xe8, 0xce, 0xd1, 0xd8);
DEFINE_GUID(CLSID_MyFolderSendCommand,
            0x1624aff3, 0xe131, 0x4d08, 0x9f, 0x39, 0x6a, 0x54, 0x4b, 0x03, 0xa3, 0x16);

namespace {
LONG g_objectCount = 0;
LONG g_serverLocks = 0;

enum class CommandKind { Root, Upload, Send };

bool readInstallDirectory(wchar_t *directory, DWORD characterCount)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MyFolder", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0;
    DWORD bytes = characterCount * sizeof(wchar_t);
    const LONG result = RegQueryValueExW(key, L"InstallPath", nullptr, &type,
                                         reinterpret_cast<BYTE *>(directory), &bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)
           && directory[0] != L'\0';
}

bool commandEnabled(const wchar_t *valueName)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MyFolder\\ShellCommands", 0,
                      KEY_READ, &key) != ERROR_SUCCESS)
        return true;
    DWORD value = 1;
    DWORD type = 0;
    DWORD bytes = sizeof(value);
    const LONG result = RegQueryValueExW(key, valueName, nullptr, &type,
                                         reinterpret_cast<BYTE *>(&value), &bytes);
    RegCloseKey(key);
    return result != ERROR_SUCCESS || type != REG_DWORD || value != 0;
}

std::vector<CommandKind> enabledCommands()
{
    std::vector<CommandKind> result;
    if (commandEnabled(L"UploadServer")) result.push_back(CommandKind::Upload);
    if (commandEnabled(L"SendClient")) result.push_back(CommandKind::Send);
    return result;
}

HRESULT selectedFilePath(IShellItemArray *items, PWSTR *path)
{
    if (!items || !path) return E_INVALIDARG;
    *path = nullptr;
    DWORD count = 0;
    if (FAILED(items->GetCount(&count)) || count != 1) return E_FAIL;
    IShellItem *item = nullptr;
    HRESULT hr = items->GetItemAt(0, &item);
    if (FAILED(hr)) return hr;
    SFGAOF attributes = 0;
    hr = item->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER, &attributes);
    if (SUCCEEDED(hr) && (attributes & SFGAO_FILESYSTEM) != 0
        && (attributes & SFGAO_FOLDER) == 0) {
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, path);
    } else {
        hr = E_FAIL;
    }
    item->Release();
    return hr;
}

class ExplorerCommand;

class CommandEnumerator final : public IEnumExplorerCommand
{
public:
    explicit CommandEnumerator(std::vector<CommandKind> commands)
        : m_commands(std::move(commands)) { InterlockedIncrement(&g_objectCount); }

    IFACEMETHODIMP QueryInterface(REFIID iid, void **object) override
    {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IEnumExplorerCommand))
            *object = static_cast<IEnumExplorerCommand *>(this);
        if (!*object) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&m_refs)); }
    IFACEMETHODIMP_(ULONG) Release() override
    {
        const LONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0) delete this;
        return static_cast<ULONG>(refs);
    }
    IFACEMETHODIMP Next(ULONG count, IExplorerCommand **commands, ULONG *fetched) override;
    IFACEMETHODIMP Skip(ULONG count) override
    {
        m_index = std::min(m_index + static_cast<size_t>(count), m_commands.size());
        return m_index < m_commands.size() ? S_OK : S_FALSE;
    }
    IFACEMETHODIMP Reset() override { m_index = 0; return S_OK; }
    IFACEMETHODIMP Clone(IEnumExplorerCommand **clone) override
    {
        if (!clone) return E_POINTER;
        auto *copy = new (std::nothrow) CommandEnumerator(m_commands);
        if (!copy) return E_OUTOFMEMORY;
        copy->m_index = m_index;
        *clone = copy;
        return S_OK;
    }

private:
    ~CommandEnumerator() { InterlockedDecrement(&g_objectCount); }
    LONG m_refs = 1;
    std::vector<CommandKind> m_commands;
    size_t m_index = 0;
};

class ExplorerCommand final : public IExplorerCommand
{
public:
    explicit ExplorerCommand(CommandKind kind) : m_kind(kind) { InterlockedIncrement(&g_objectCount); }

    IFACEMETHODIMP QueryInterface(REFIID iid, void **object) override
    {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IExplorerCommand))
            *object = static_cast<IExplorerCommand *>(this);
        if (!*object) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&m_refs)); }
    IFACEMETHODIMP_(ULONG) Release() override
    {
        const LONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0) delete this;
        return static_cast<ULONG>(refs);
    }
    IFACEMETHODIMP GetTitle(IShellItemArray *, PWSTR *title) override
    {
        const wchar_t *value = L"MyFolder";
        if (m_kind == CommandKind::Upload) value = L"\u4e0a\u4f20\u5230 MyFolder \u670d\u52a1\u5668";
        else if (m_kind == CommandKind::Send) value = L"\u53d1\u9001\u5230\u6307\u5b9a MyFolder \u5ba2\u6237\u7aef";
        return SHStrDupW(value, title);
    }
    IFACEMETHODIMP GetIcon(IShellItemArray *, PWSTR *icon) override
    {
        wchar_t directory[MAX_PATH] = {};
        wchar_t value[MAX_PATH + 16] = {};
        if (!readInstallDirectory(directory, ARRAYSIZE(directory))) return E_NOTIMPL;
        if (FAILED(StringCchPrintfW(value, ARRAYSIZE(value), L"%s\\appMyFolder.exe,0", directory)))
            return E_FAIL;
        return SHStrDupW(value, icon);
    }
    IFACEMETHODIMP GetToolTip(IShellItemArray *, PWSTR *tooltip) override
    {
        if (!tooltip) return E_POINTER;
        *tooltip = nullptr;
        return E_NOTIMPL;
    }
    IFACEMETHODIMP GetCanonicalName(GUID *guid) override
    {
        if (!guid) return E_POINTER;
        if (m_kind == CommandKind::Root) *guid = CLSID_MyFolderRootCommand;
        else if (m_kind == CommandKind::Upload) *guid = CLSID_MyFolderUploadCommand;
        else *guid = CLSID_MyFolderSendCommand;
        return S_OK;
    }
    IFACEMETHODIMP GetState(IShellItemArray *items, BOOL, EXPCMDSTATE *state) override
    {
        if (!state) return E_POINTER;
        PWSTR path = nullptr;
        const HRESULT hr = selectedFilePath(items, &path);
        if (path) CoTaskMemFree(path);
        const bool enabled = SUCCEEDED(hr) && (m_kind != CommandKind::Root || !enabledCommands().empty());
        *state = enabled ? ECS_ENABLED : ECS_HIDDEN;
        return S_OK;
    }
    IFACEMETHODIMP Invoke(IShellItemArray *items, IBindCtx *) override
    {
        if (m_kind == CommandKind::Root) return S_OK;
        PWSTR path = nullptr;
        HRESULT hr = selectedFilePath(items, &path);
        if (FAILED(hr)) return hr;

        wchar_t directory[MAX_PATH] = {};
        wchar_t executable[MAX_PATH] = {};
        wchar_t parameters[32768] = {};
        if (!readInstallDirectory(directory, ARRAYSIZE(directory))
            || FAILED(StringCchPrintfW(executable, ARRAYSIZE(executable), L"%s\\appMyFolder.exe", directory))
            || FAILED(StringCchPrintfW(parameters, ARRAYSIZE(parameters), L"%s \"%s\"",
                                       m_kind == CommandKind::Upload ? L"--upload-server" : L"--send-client",
                                       path))) {
            CoTaskMemFree(path);
            return E_FAIL;
        }
        CoTaskMemFree(path);
        const HINSTANCE result = ShellExecuteW(nullptr, L"open", executable, parameters, directory, SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(result) > 32 ? S_OK : HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
    }
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS *flags) override
    {
        if (!flags) return E_POINTER;
        *flags = m_kind == CommandKind::Root ? ECF_HASSUBCOMMANDS : ECF_DEFAULT;
        return S_OK;
    }
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand **commands) override
    {
        if (!commands) return E_POINTER;
        *commands = nullptr;
        if (m_kind != CommandKind::Root) return E_NOTIMPL;
        auto *enumerator = new (std::nothrow) CommandEnumerator(enabledCommands());
        if (!enumerator) return E_OUTOFMEMORY;
        *commands = enumerator;
        return S_OK;
    }

private:
    ~ExplorerCommand() { InterlockedDecrement(&g_objectCount); }
    LONG m_refs = 1;
    CommandKind m_kind;
};

IFACEMETHODIMP CommandEnumerator::Next(ULONG count, IExplorerCommand **commands, ULONG *fetched)
{
    if (!commands) return E_POINTER;
    if (count > 1 && !fetched) return E_POINTER;
    ULONG produced = 0;
    while (produced < count && m_index < m_commands.size()) {
        commands[produced] = new (std::nothrow) ExplorerCommand(m_commands[m_index]);
        if (!commands[produced]) break;
        ++produced;
        ++m_index;
    }
    if (fetched) *fetched = produced;
    return produced == count ? S_OK : S_FALSE;
}

class CommandFactory final : public IClassFactory
{
public:
    CommandFactory() { InterlockedIncrement(&g_objectCount); }
    IFACEMETHODIMP QueryInterface(REFIID iid, void **object) override
    {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
            *object = static_cast<IClassFactory *>(this);
        if (!*object) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&m_refs)); }
    IFACEMETHODIMP_(ULONG) Release() override
    {
        const LONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0) delete this;
        return static_cast<ULONG>(refs);
    }
    IFACEMETHODIMP CreateInstance(IUnknown *outer, REFIID iid, void **object) override
    {
        if (outer) return CLASS_E_NOAGGREGATION;
        auto *command = new (std::nothrow) ExplorerCommand(CommandKind::Root);
        if (!command) return E_OUTOFMEMORY;
        const HRESULT hr = command->QueryInterface(iid, object);
        command->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL lock) override
    {
        lock ? InterlockedIncrement(&g_serverLocks) : InterlockedDecrement(&g_serverLocks);
        return S_OK;
    }

private:
    ~CommandFactory() { InterlockedDecrement(&g_objectCount); }
    LONG m_refs = 1;
};
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(instance);
    return TRUE;
}

extern "C" STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **object)
{
    if (!IsEqualCLSID(clsid, CLSID_MyFolderRootCommand)) return CLASS_E_CLASSNOTAVAILABLE;
    auto *factory = new (std::nothrow) CommandFactory();
    if (!factory) return E_OUTOFMEMORY;
    const HRESULT hr = factory->QueryInterface(iid, object);
    factory->Release();
    return hr;
}

extern "C" STDAPI DllCanUnloadNow()
{
    return g_objectCount == 0 && g_serverLocks == 0 ? S_OK : S_FALSE;
}
