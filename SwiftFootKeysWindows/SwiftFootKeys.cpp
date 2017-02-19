#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#include <iostream>
#include <map>
using namespace std;
BLUETOOTH_FIND_RADIO_PARAMS m_bt_find_radio = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

BLUETOOTH_RADIO_INFO m_bt_info = { sizeof(BLUETOOTH_RADIO_INFO),0, };
GUID serviceID = _GUID{ 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x76 };

/*
todo make this a seperate class with enums for the values
*/
static map<int, int> androidKeyCodeToWindows;
static map<int, int> androidKeyActionToWindows;
void initWindowMaps();


void TestSendKeys();
void TestBlueTooth();
int bufferToInt(char *buffer);
void sendKey(int action, int keyCode);
void sendKey(int keyCode);
void sendKeyPress(int keyCode);
void sendKeyUp(int keyCode);

BLUETOOTH_DEVICE_SEARCH_PARAMS m_search_params = {
  sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
  1,
  0,
  1,
  1,
  1,
  15,
  NULL
};
BLUETOOTH_DEVICE_INFO m_device_info = { sizeof(BLUETOOTH_DEVICE_INFO),0, };


int main(){
  initWindowMaps();
  //TestSendKeys();
  TestBlueTooth();
}
void TestBlueTooth() {
  HANDLE m_radio = NULL;
  HBLUETOOTH_RADIO_FIND m_bt = NULL;
  HBLUETOOTH_DEVICE_FIND m_bt_dev = NULL;
  DWORD mbtinfo_ret;

  //find a bluetooth radio to use
  m_bt = BluetoothFindFirstRadio(&m_bt_find_radio, &m_radio);
  if (m_bt == NULL) {
    cout << "ERROR: Radio not found" << endl;
    return;
  }

  // Then get the radio device info....
  mbtinfo_ret = BluetoothGetRadioInfo(m_radio, &m_bt_info);
  if (mbtinfo_ret != ERROR_SUCCESS) {
    printf("ERROR: BluetoothGetRadioInfo() failed wit herror code %d\n", mbtinfo_ret);
    exit(0);
  }

  m_search_params.hRadio = m_radio;
  ZeroMemory(&m_device_info, sizeof(BLUETOOTH_DEVICE_INFO));
  m_device_info.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);

  WSADATA wsd;
  if (WSAStartup(MAKEWORD(1, 0), &wsd) != 0) {
    exit(0);
  }

  SOCKET sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
  if (sock == INVALID_SOCKET) {
    int a = 1;
  }
  SOCKADDR_BTH name;
  memset(&name, 0, sizeof(name));
  name.addressFamily = AF_BTH;
  name.btAddr = 0;
  name.serviceClassId = _GUID{ 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74 };
  name.port = BT_PORT_ANY;

  if (bind(sock, (sockaddr*)&name, sizeof(name))) {
    int a = 1;
  }

  printf("\nBinding Successful....\n");
  int length = sizeof(SOCKADDR_BTH);
  getsockname(sock, (sockaddr*)&name, &length);
  wprintf(L"Local Bluetooth device is %04x%08x \nServer channel = %d\n",
    GET_NAP(name.btAddr), GET_SAP(name.btAddr), name.port);

  int size = sizeof(SOCKADDR_BTH);
  if (0 != getsockname(sock, (sockaddr*)&name, &length)) {
    exit(0);
  }
  if (0 != listen(sock, 10))
  {
    exit(0);
  }



  WSAQUERYSET service;
  memset(&service, 0, sizeof(service));
  service.dwSize = sizeof(service);
  service.lpszServiceInstanceName = LPSTR("Accelerometer Data...");
  service.lpszComment = LPSTR("Pushing data to PC");

  //GUID serviceID = OBEXFileTransferServiceClass_UUID;//_GUID{ 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74 };


  service.lpServiceClassId = &serviceID;
  service.dwNumberOfCsAddrs = 1;
  service.dwNameSpace = NS_BTH;

  CSADDR_INFO csAddr;
  memset(&csAddr, 0, sizeof(csAddr));
  csAddr.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
  csAddr.LocalAddr.lpSockaddr = (sockaddr*)&name;
  csAddr.iSocketType = SOCK_STREAM;
  csAddr.iProtocol = BTHPROTO_RFCOMM;
  service.lpcsaBuffer = &csAddr;

  if (0 != WSASetService(&service, RNRSERVICE_REGISTER, 0))
  {
    printf("Service registration failed....");
    //printf("%d\n", GetLastErrorMessage(GetLastError()));
    exit(0);
  }
  else
  {
    printf("\nService registration Successful....\n");
  }
  while (1) {
    printf("\nBefore accept.........");
    SOCKADDR_BTH sab2;
    int ilen = sizeof(sab2);
    SOCKET s2 = accept(sock, (sockaddr*)&sab2, &ilen);
    cout << "Connection Accepted" << endl;
    if (s2 == INVALID_SOCKET)
    {
      wprintf(L"Socket bind, error %d\n", WSAGetLastError());
    }
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    while (1) {
      int r = recv(s2, (char*)buffer, sizeof(buffer), 0);
      cout << "buffer size: " << r << endl;
      cout << "buffer = ";
      for (int i = 0; i < r; ++i) {
        cout << "{ " << int(buffer[i]) << " }, ";
        if ((i + 1) % 4 == 0) {
          cout << endl;
        }
      }
      for (int i = 0; i <= r - 8; i += 8) {
        cout << bufferToInt(&buffer[i]) << ", " << bufferToInt(&buffer[i + 4]);
        cout << " : " << androidKeyActionToWindows[bufferToInt(&buffer[i])] << " ";
        cout << androidKeyCodeToWindows[bufferToInt(&buffer[i + 4])] << endl;
        sendKey(androidKeyActionToWindows[bufferToInt(&buffer[i])], androidKeyCodeToWindows[bufferToInt(&buffer[i + 4])]);
      }
      if (r == 0) {
        wprintf(L"Socket disconnected, r was 0 : %d\n", WSAGetLastError());
        cout << "Attempting to create new connection" << endl;
        break;
      }
    }
  }
  // NO more device, close the device handle
  if (BluetoothFindDeviceClose(m_bt_dev) == TRUE)
    printf("\nBluetoothFindDeviceClose(m_bt_dev) is OK!\n");
  else
    printf("\nBluetoothFindDeviceClose(m_bt_dev) failed with error code %d\n", GetLastError());
}

void TestSendKeys() {
  Sleep(5000);
  cout << 0x41 << endl;
  sendKey(0x41);
  cout << 0x42 << endl;
  sendKeyPress(0x42);
  Sleep(500);
  sendKeyUp(0x42);
  return;

}

void sendKey(int keyCode) {
  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  ip.ki.wVk = keyCode;
  ip.ki.dwFlags = 0;
  SendInput(1, &ip, sizeof(INPUT));
  Sleep(100);
  ip.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &ip, sizeof(INPUT));
}
void sendKey(int action, int keyCode) {
  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  ip.ki.wVk = keyCode;
  ip.ki.dwFlags = action;
  SendInput(1, &ip, sizeof(INPUT));
  //Sleep(10);
}
void sendKeyPress(int keyCode) {

  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  ip.ki.wVk = keyCode;
  ip.ki.dwFlags = 0;
  SendInput(1, &ip, sizeof(INPUT));
}
void sendKeyUp(int keyCode) {
  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  ip.ki.wVk = keyCode;
  ip.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &ip, sizeof(INPUT));
}

int bufferToInt(char *buffer) {
  return static_cast<int> ((buffer[0] << 24) | (buffer[1] << 16) |
    (buffer[2] << 8) | buffer[3]);
}



void initWindowMaps() {
  androidKeyActionToWindows[1] = KEYEVENTF_KEYUP;
  androidKeyActionToWindows[0] = 0;

  //set number keys
  const int ANDROID_NUM_START = 7;
  const int WINDOWS_NUM_START = 0x30;
  for (int i = 0; i < 10; ++i) {
    androidKeyCodeToWindows.insert(pair<int, int>(ANDROID_NUM_START + i, WINDOWS_NUM_START + i));
  }
  //- key
  androidKeyCodeToWindows.insert(pair<int, int>(69, 0xbd));
  //= key
  androidKeyCodeToWindows.insert(pair<int, int>(70, 0xbb));


  //set numpad keys
  const int ANDROID_NUMPAD_START = 144;
  const int WINDOWS_NUMPAD_START = 0x60;
  for (int i = 0; i < 10; ++i) {
    androidKeyCodeToWindows.insert(pair<int, int>(ANDROID_NUMPAD_START + i, WINDOWS_NUMPAD_START + i));
  }


  //set alphabet
  const int ANDROID_ALPHA_START = 29;
  const int WINDOWS_ALPHA_START = 0x41;
  for (int i = 0; i < 26; ++i) {
    androidKeyCodeToWindows.insert(pair<int, int>(ANDROID_ALPHA_START + i, WINDOWS_ALPHA_START + i));
  }
  //set function keys
  const int ANDROID_F_START = 131;
  const int WINDOWS_F_START = 0x70;
  for (int i = 0; i < 12; ++i) {
    androidKeyCodeToWindows.insert(pair<int, int>(ANDROID_F_START + i, WINDOWS_F_START + i));
  }
  //`~ key
  androidKeyCodeToWindows.insert(pair<int, int>(68, 0xC0));

  //[{ key
  androidKeyCodeToWindows.insert(pair<int, int>(71, 0xDB));
  //]} key
  androidKeyCodeToWindows.insert(pair<int, int>(72, 0xDD));
  //\| key
  androidKeyCodeToWindows.insert(pair<int, int>(73, 0xDC));
  //;: key
  androidKeyCodeToWindows.insert(pair<int, int>(74, 0xBA));
  //"' key
  androidKeyCodeToWindows.insert(pair<int, int>(75, 0xDE));
  //,< key
  androidKeyCodeToWindows.insert(pair<int, int>(55, 0xBC));
  //.>
  androidKeyCodeToWindows.insert(pair<int, int>(56, 0xBE));
  ///? key
  androidKeyCodeToWindows.insert(pair<int, int>(76, 0xBF));



  //left shift
  androidKeyCodeToWindows.insert(pair<int, int>(59, 0xA0));
  //right shift
  androidKeyCodeToWindows.insert(pair<int, int>(60, 0xA1));
  //space bar
  androidKeyCodeToWindows.insert(pair<int, int>(62, 0x20));
  //backspace key
  androidKeyCodeToWindows.insert(pair<int, int>(67, 0x08));
  //right control
  androidKeyCodeToWindows.insert(pair<int, int>(114, 0xA3));
  //left  control
  androidKeyCodeToWindows.insert(pair<int, int>(113, 0xA2));
  //alt left
  androidKeyCodeToWindows.insert(pair<int, int>(103, 0x12));
  //alr right
  androidKeyCodeToWindows.insert(pair<int, int>(102, 0x12));
  //apostrophe
  //androidKeyCodeToWindows.insert(pair<int, int>(75, ));
  //up arrow
  androidKeyCodeToWindows.insert(pair<int, int>(19, 0x26));
  //down arrow
  androidKeyCodeToWindows.insert(pair<int, int>(20, 0x28));
  //right arrow
  androidKeyCodeToWindows.insert(pair<int, int>(22, 0x27));
  //left arrow
  androidKeyCodeToWindows.insert(pair<int, int>(21, 0x25));
  //escape key
  androidKeyCodeToWindows.insert(pair<int, int>(111, 0x1B));
  //tab key
  androidKeyCodeToWindows.insert(pair<int, int>(61, 0x09));
  //caps lock
  //androidKeyCodeToWindows.insert(pair<int, int>(115, 0x14));
  //enter
  androidKeyCodeToWindows.insert(pair<int, int>(66, 0x0D));
  //print screen
  androidKeyCodeToWindows.insert(pair<int, int>(120, 0x2A));
  //scroll lock
  androidKeyCodeToWindows.insert(pair<int, int>(116, 0x91));
  //pause break
  androidKeyCodeToWindows.insert(pair<int, int>(121, 0x13));
  //insert
  androidKeyCodeToWindows.insert(pair<int, int>(124, 0x2D));
  //delete key
  androidKeyCodeToWindows.insert(pair<int, int>(112, 0x2E));
  //home key
  androidKeyCodeToWindows.insert(pair<int, int>(61, 0x24));
  //end key
  androidKeyCodeToWindows.insert(pair<int, int>(123, 0x23));

  ///////////
  //page down
  androidKeyCodeToWindows.insert(pair<int, int>(93, 0x22));
  //page up
  androidKeyCodeToWindows.insert(pair<int, int>(92, 0x21));
  //num lock 143
  //androidKeyCodeToWindows.insert(pair<int, int>(143, 0x90));

  //escape
  androidKeyCodeToWindows.insert(pair<int, int>(105, 0x1B));
  //windows left
  androidKeyCodeToWindows.insert(pair<int, int>(104, 0x5B));

  //windows right
  androidKeyCodeToWindows.insert(pair<int, int>(106, 0x5C));

  //menu key
  androidKeyCodeToWindows.insert(pair<int, int>(82, 0xA5));


} 