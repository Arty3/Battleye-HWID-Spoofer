#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

/* Simple visualizer of success */

class SystemInfo
{
private:
	IWbemLocator* pLoc = nullptr;
	IWbemServices* pSvc = nullptr;

	bool InitializeCOM()
	{
		HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres))
		{
			std::cerr << "Failed to initialize COM library. Error code = 0x" << std::hex << hres << std::endl;
			return false;
		}

		hres = CoInitializeSecurity(
			nullptr,
			-1,
			nullptr,
			nullptr,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr,
			EOAC_NONE,
			nullptr
		);

		if (FAILED(hres))
		{
			std::cerr << "Failed to initialize security. Error code = 0x" << std::hex << hres << std::endl;
			CoUninitialize();
			return false;
		}

		hres = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator,
			(LPVOID*)&pLoc
		);

		if (FAILED(hres))
		{
			std::cerr << "Failed to create IWbemLocator object. Error code = 0x" << std::hex << hres << std::endl;
			CoUninitialize();
			return false;
		}

		hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"),
			nullptr,
			nullptr,
			0,
			NULL,
			0,
			0,
			&pSvc
		);

		if (FAILED(hres))
		{
			std::cerr << "Could not connect to WMI. Error code = 0x" << std::hex << hres << std::endl;
			pLoc->Release();
			CoUninitialize();
			return false;
		}

		hres = CoSetProxyBlanket(
			pSvc,
			RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE,
			nullptr,
			RPC_C_AUTHN_LEVEL_CALL,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr,
			EOAC_NONE
		);

		if (FAILED(hres))
		{
			std::cerr << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return false;
		}

		return true;
	}

	std::vector<std::string> ExecuteWMIQuery(const std::wstring& query, const std::wstring& property)
	{
		std::vector<std::string> results;

		if (!pSvc)
			return results;

		IEnumWbemClassObject* pEnumerator = nullptr;
		HRESULT hres = pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t(query.c_str()),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr,
			&pEnumerator
		);

		if (FAILED(hres))
		{
			std::cerr << "Query failed. Error code = 0x" << std::hex << hres << std::endl;
			return results;
		}

		IWbemClassObject* pclsObj = nullptr;
		ULONG uReturn = 0;

		while (pEnumerator)
		{
			hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if (uReturn == 0)
				break;

			VARIANT vtProp;
			VariantInit(&vtProp);

			hres = pclsObj->Get(property.c_str(), 0, &vtProp, 0, 0);

			if (SUCCEEDED(hres) && (vtProp.vt == VT_BSTR) && vtProp.bstrVal)
			{
				_bstr_t bstr_t_prop(vtProp.bstrVal, false);
				results.push_back(static_cast<const char*>(bstr_t_prop));
			}
			else if (SUCCEEDED(hres) && vtProp.vt == VT_BOOL)
			{
				results.push_back(vtProp.boolVal ? "True" : "False");
			}

			VariantClear(&vtProp);
			pclsObj->Release();
		}

		pEnumerator->Release();
		return results;
	}

public:
	SystemInfo()
	{
		InitializeCOM();
	}

	~SystemInfo()
	{
		if (pSvc)
			pSvc->Release();

		if (pLoc)
			pLoc->Release();

		CoUninitialize();
	}

	void GetDiskInfo()
	{
		std::cout << "\n=== DISK INFORMATION ===\n";

		std::vector<std::string> diskModels = ExecuteWMIQuery(L"SELECT Model FROM Win32_DiskDrive", L"Model");
		std::vector<std::string> diskSerials = ExecuteWMIQuery(L"SELECT SerialNumber FROM Win32_DiskDrive", L"SerialNumber");

		for (size_t i = 0; i < diskModels.size(); ++i)
		{
			std::cout << "Disk " << i + 1 << ":\n";
			std::cout << "  Model: " << diskModels[i] << "\n";

			if (i < diskSerials.size())
				std::cout << "  Serial Number: " << diskSerials[i] << "\n";
			else
				std::cout << "  Serial Number: Not available\n";
		}
	}

	void GetBaseBoardInfo()
	{
		std::cout << "\n=== BASEBOARD INFORMATION ===\n";

		std::vector<std::string> serialNumbers = ExecuteWMIQuery(L"SELECT SerialNumber FROM Win32_BaseBoard", L"SerialNumber");

		if (!serialNumbers.empty())
			std::cout << "Baseboard Serial Number: " << serialNumbers[0] << "\n";
		else
			std::cout << "Baseboard Serial Number: Not available\n";
	}

	void GetBiosInfo()
	{
		std::cout << "\n=== BIOS INFORMATION ===\n";

		std::vector<std::string> serialNumbers = ExecuteWMIQuery(L"SELECT SerialNumber FROM Win32_BIOS", L"SerialNumber");

		if (!serialNumbers.empty())
			std::cout << "BIOS Serial Number: " << serialNumbers[0] << "\n";
		else
			std::cout << "BIOS Serial Number: Not available\n";
	}

	void GetTPMStatus()
	{
		std::cout << "\n=== TPM STATUS ===\n";

		std::vector<std::string> tpmPresent = ExecuteWMIQuery(L"SELECT IsEnabled_InitialValue FROM Win32_Tpm", L"IsEnabled_InitialValue");

		if (!tpmPresent.empty())
			std::cout << "TPM Enabled: " << tpmPresent[0] << "\n";
		else
			std::cout << "TPM not detected or information not available\n";

		std::vector<std::string> tpmVersion = ExecuteWMIQuery(L"SELECT SpecVersion FROM Win32_Tpm", L"SpecVersion");

		if (!tpmVersion.empty())
			std::cout << "TPM Version: " << tpmVersion[0] << "\n";
	}
};

int main()
{
	try
	{
		SystemInfo sysInfo;

		sysInfo.GetDiskInfo();
		sysInfo.GetBaseBoardInfo();
		sysInfo.GetBiosInfo();
		sysInfo.GetTPMStatus();

		std::cout << "\nPress Enter to exit...";
		std::cin.get();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
