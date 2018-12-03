#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include "Offsets.h"
#include "Vector.h"
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

using namespace std;
const char* Game = "SoTGame.exe";
const char* hName = "Overlay";
const char* tName = "Sea of Thieves";
int Width = 1920, Height = 1080;

DWORD_PTR GNames, UWorld;
Vector3 CameraLocation, CameraRotation;
Vector2 ScreenPoint;
float FOV;
bool Debug, Misc, Init;
string Name, Term;

HANDLE hProcess;
RECT hWindow;
RECT tWindow;
HWND hWnd, tWnd;

auto Type = D2D1::ColorF(D2D1::ColorF::Gray, 1.0f);
auto Player = D2D1::ColorF(D2D1::ColorF::Firebrick, 1.0f);
auto Ship = D2D1::ColorF(D2D1::ColorF::LimeGreen, 1.0f);
auto Common = D2D1::ColorF(D2D1::ColorF::White, 1.0f);
auto Rare = D2D1::ColorF(D2D1::ColorF::Blue, 1.0f);
auto Legendary = D2D1::ColorF(D2D1::ColorF::Purple, 1.0f);
auto Mythical = D2D1::ColorF(D2D1::ColorF::Orange, 1.0f);
auto Cloud = D2D1::ColorF(D2D1::ColorF::SlateBlue, 1.0f);
auto Crate = D2D1::ColorF(D2D1::ColorF::Silver, 1.0f);
auto Enemy = D2D1::ColorF(D2D1::ColorF::Red, 1.0f);
auto Other = D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f);

DWORD_PTR FindPattern(DWORD_PTR Base, DWORD Size, const char* Signature, const char* Mask) {
	DWORD_PTR Current = Base;
	size_t BytesRead;

	while (Base + Size > Current) {
		byte Buffer[4096];
		ReadProcessMemory(hProcess, (LPCVOID)Current, &Buffer, sizeof(Buffer), &BytesRead);

		if (!BytesRead)
			return 0;

		for (int i = 0; i < BytesRead; i++) {
			bool Found = true;
			for (int j = 0; j < strlen(Mask); j++)
				if (Mask[j] != '?' && Signature[j] != *(const char*)(Buffer + i + j)) {
					Found = false;
					break;
				}

			if (Found)
				return Current + i;
		}

		Current += BytesRead;
	}

	return 0;
}

void ReadData() {
	DWORD_PTR Base, Address;
	DWORD PID, Size, Offset;

	PROCESSENTRY32 ProcEntry;
	ProcEntry.dwSize = sizeof(ProcEntry);
	HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	while (Process32Next(hPID, &ProcEntry))
		if (!lstrcmpi(ProcEntry.szExeFile, Game))
			PID = ProcEntry.th32ProcessID;

	CloseHandle(hPID);

	MODULEENTRY32 ModEntry;
	ModEntry.dwSize = sizeof(ModEntry);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	while (Module32Next(hSnapshot, &ModEntry))
		if (!lstrcmpi(ModEntry.szModule, Game)) {
			Base = (DWORD_PTR)ModEntry.modBaseAddr;
			Size = ModEntry.modBaseSize;
		}

	CloseHandle(hSnapshot);
	hProcess = OpenProcess(MAXIMUM_ALLOWED, 0, PID);

	Address = FindPattern(Base, Size, "\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\x00\x75\x3A", "xxx????xx?xx");
	ReadProcessMemory(hProcess, (LPCVOID)(Address + 3), &Offset, sizeof(Offset), 0);
	GNames = Address + Offset + 7;

	Address = FindPattern(Base, Size, "\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\x48\x8B\xF8", "xxx????xxxxx????xxx");
	ReadProcessMemory(hProcess, (LPCVOID)(Address + 3), &Offset, sizeof(Offset), 0);
	UWorld = Address + Offset + 7;
}

class Read {
public:
	template <class Data>
	Data Memory(DWORD_PTR Address) {
		Data Buffer;
		ReadProcessMemory(hProcess, (LPVOID)Address, &Buffer, sizeof(Buffer), 0);
		return Buffer;
	}
};

class Text {
public:
	char word[64];
};

class TextX {
public:
	wchar_t word[64];
};

#define Assert( _exp ) ((void)0)
struct vMatrix {
	vMatrix() {}
	vMatrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23) {
		m_flMatVal[0][0] = m00; m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10; m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20; m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	float *operator[](int i) {
		Assert((i >= 0) && (i < 3));
		return m_flMatVal[i];
	}

	const float *operator[](int i) const {
		Assert((i >= 0) && (i < 3));
		return m_flMatVal[i];
	}

	float *Base() {
		return &m_flMatVal[0][0];
	}

	const float *Base() const {
		return &m_flMatVal[0][0];
	}

	float m_flMatVal[3][4];
};

vMatrix Matrix(Vector3 rot, Vector3 Origin) {
	Origin = Vector3(0, 0, 0);
	float radPitch = (rot.x * float(PI) / 180.f);
	float radYaw = (rot.y * float(PI) / 180.f);
	float radRoll = (rot.z * float(PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	vMatrix matrix;
	matrix[0][0] = CP * CY;
	matrix[0][1] = CP * SY;
	matrix[0][2] = SP;
	matrix[0][3] = 0.f;

	matrix[1][0] = SR * SP * CY - CR * SY;
	matrix[1][1] = SR * SP * SY + CR * CY;
	matrix[1][2] = -SR * CP;
	matrix[1][3] = 0.f;

	matrix[2][0] = -(CR * SP * CY + SR * SY);
	matrix[2][1] = CY * SR - CR * SP * SY;
	matrix[2][2] = CR * CP;
	matrix[2][3] = 0.f;

	return matrix;
}

bool WorldToScreen(Vector3 Origin, Vector2 * Out) {
	Vector3 Screenlocation = Vector3(0, 0, 0);
	vMatrix tempMatrix = Matrix(CameraRotation, Vector3(0, 0, 0));
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix[0][0], tempMatrix[0][1], tempMatrix[0][2]);
	vAxisY = Vector3(tempMatrix[1][0], tempMatrix[1][1], tempMatrix[1][2]);
	vAxisZ = Vector3(tempMatrix[2][0], tempMatrix[2][1], tempMatrix[2][2]);

	Vector3 vDelta = Origin - CameraLocation;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float ScreenCenterX = Width / 2.0f;
	float ScreenCenterY = Height / 2.0f;

	Out->x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FOV * static_cast<float>(PI) / 360.f)) / vTransformed.z;
	Out->y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FOV * static_cast<float>(PI) / 360.f)) / vTransformed.z;

	return true;
}