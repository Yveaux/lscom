// lscom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <Setupapi.h>
#include <iostream>

using namespace std;

// listup serial port driver 
// cf. http://www.codeproject.com/system/setupdi.asp?df=100&forumid=4368&exp=0&select=479661
// (2007.8.17 yutaka)
static void ListupSerialPort(LPWORD ComPortTable, int comports, char **ComPortDesc, int ComPortMax)
{
	GUID ClassGuid[1];
	DWORD dwRequiredSize;
	BOOL bRet;
	HDEVINFO DeviceInfoSet = NULL;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD dwMemberIndex = 0;
	int i;

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (i = 0 ; i < ComPortMax ; i++) {
		free(ComPortDesc[i]);
		ComPortDesc[i] = NULL;
	}

	// Get ClassGuid from ClassName for PORTS class
	bRet =
		SetupDiClassGuidsFromName(_T("PORTS"), (LPGUID) & ClassGuid, 1,
		                          &dwRequiredSize);
	if (!bRet) {
		goto cleanup;
	}

	// Get class devices
	DeviceInfoSet =
		SetupDiGetClassDevs(&ClassGuid[0], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);

	if (DeviceInfoSet) {
		// Enumerate devices
		dwMemberIndex = 0;
		while (SetupDiEnumDeviceInfo
		       (DeviceInfoSet, dwMemberIndex++, &DeviceInfoData)) {
			TCHAR szFriendlyName[MAX_PATH];
			TCHAR szPortName[MAX_PATH];
			//TCHAR szMessage[MAX_PATH];
			DWORD dwReqSize = 0;
			DWORD dwPropType;
			DWORD dwType = REG_SZ;
			HKEY hKey = NULL;

			// Get friendlyname
			bRet = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
			                                        &DeviceInfoData,
			                                        SPDRP_FRIENDLYNAME,
			                                        &dwPropType,
			                                        (LPBYTE)
			                                        szFriendlyName,
			                                        sizeof(szFriendlyName),
			                                        &dwReqSize);

			// Open device parameters reg key
			hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
			                            &DeviceInfoData,
			                            DICS_FLAG_GLOBAL,
			                            0, DIREG_DEV, KEY_READ);
			if (hKey) {
				// Qurey for portname
				long lRet;
				dwReqSize = sizeof(szPortName);
				lRet = RegQueryValueEx(hKey,
				                       _T("PortName"),
				                       0,
				                       &dwType,
				                       (LPBYTE) & szPortName,
				                       &dwReqSize);

				// Close reg key
				RegCloseKey(hKey);
			}

#if 0
			sprintf(szMessage, _T("Name: %s\nPort: %s\n"), szFriendlyName,
			        szPortName);
			printf("%s\n", szMessage);
#endif

			if (_strnicmp(const_cast<const char*>(reinterpret_cast<char*>(szPortName)), "COM", 3) == 0) {
				int port = atoi(const_cast<const char*>(reinterpret_cast<char*>(&szPortName[3])));
				int i;

				for (i = 0 ; i < comports ; i++) {
					if (ComPortTable[i] == port) {
						ComPortDesc[i] = _strdup(const_cast<const char*>(reinterpret_cast<char*>(szFriendlyName)));
						break;
					}
				}
			}

		}
	}

cleanup:
// Destroy device info list
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
}


int DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc)
{
	HMODULE h;
	TCHAR   devicesBuff[65535];
	TCHAR   *p;
	int     comports = 0;
	int     i, j, min;
	WORD    s;

	if (((h = GetModuleHandle(reinterpret_cast<LPCSTR>("kernel32.dll"))) != NULL) &&
	    (GetProcAddress(h, "QueryDosDeviceA") != NULL) &&
	    (QueryDosDevice(NULL, devicesBuff, 65535) != 0)) {
		p = devicesBuff;
		while (*p != '\0') {
			if (strncmp(const_cast<const char*>(reinterpret_cast<char*>(p)), "COM", 3) == 0 && p[3] != '\0') {
				ComPortTable[comports++] = atoi(const_cast<const char*>(reinterpret_cast<char*>(p+3)));
				if (comports >= ComPortMax)
					break;
			}
			p += (strlen(const_cast<const char*>(reinterpret_cast<char*>(p)))+1);
		}

		for (i=0; i<comports-1; i++) {
			min = i;
			for (j=i+1; j<comports; j++) {
				if (ComPortTable[min] > ComPortTable[j]) {
					min = j;
				}
			}
			if (min != i) {
				s = ComPortTable[i];
				ComPortTable[i] = ComPortTable[min];
				ComPortTable[min] = s;
			}
		}
	}
	else {
#if 1
		for (i=1; i<=ComPortMax; i++) {
			FILE *fp;
			char buf[12]; // \\.\COMxxxx + NULL
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "\\\\.\\COM%d", i);
			if (0 != fopen_s(&fp,buf, "r")) {
				fclose(fp);
				ComPortTable[comports++] = i;
			}
		}
#else
		comports = -1;
#endif
	}

	ListupSerialPort(ComPortTable, comports, ComPortDesc, ComPortMax);

	return comports;
}

#define MAXCOMPORT 4096

int _tmain(int argc, _TCHAR* argv[])
{
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int numComports;

  cout << "Lscom v1.0 - Ivo Pullens, 2010 - www.Emmission.nl" << endl << endl;

	numComports = DetectComPorts(ComPortTable, MAXCOMPORT, ComPortDesc);

	for (int i = 0; i < numComports; i++)
	{
		if (ComPortDesc[i] != NULL)
    {
      cout << "COM" << ComPortTable[i] << ": " << ComPortDesc[i] << endl;
		}
	}
    return 0;
}

