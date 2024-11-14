// ProcSyg.cpp : Definiuje punkt wejścia dla aplikacji.
//

#include "pch.h"
#include "framework.h"
#include "ProcSyg.h"
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")




#define MAX_LOADSTRING 100
#define WNDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define WAVE_FORM_START_X 50
#define WAVE_FORM_START_Y 100




// Zmienne globalne:
HINSTANCE hInst;                                // bieżące wystąpienie
WCHAR szTitle[MAX_LOADSTRING];                  // Tekst paska tytułu
WCHAR szWindowClass[MAX_LOADSTRING];            // nazwa klasy okna głównego
FILE* wavFile = nullptr;                        // global file pointer
std::wstring selectedFilePath;                  // global variable to store the file path
bool FileLoaded = false;                         // Flag to indicate if a file has been loaded
HWND hButtonPlay = NULL;                         // Handle for Play button
HWND hButtonStop = NULL;                         // Handle for Stop button
HWND hButtonExit = NULL;                         // Handle for Exit button
std::vector<short> audioData;
#pragma pack(push, 1)                            // Ensure structures are packed

//Effects variables
double DELAY_GAIN = 1;                          //Gain of delay effect
double DELAY_TIME = 1000;                       //Delay time in miliseconds


struct WAVHeader {
    char riff[4]; // RIFF Header
    unsigned int fileSize; // File size
    char wave[4]; // WAVE Header
    char fmt[4]; // FMT Header
    unsigned int fmtSize; // Format size
    unsigned short audioFormat; // Audio format
    unsigned short numChannels; // Number of channels
    unsigned int sampleRate; // Sampling frequency
    unsigned int byteRate; // Bytes per second
    unsigned short blockAlign; // Block alignment
    unsigned short bitsPerSample; // Bits per sample
    char data[4]; // DATA Header
    unsigned int dataSize; // Size of data
};

#pragma pack(pop) // Restore packing
HWAVEOUT hWaveOut = nullptr;
WAVEFORMATEX waveFormat = {};
WAVEHDR waveHeader = {};

bool InitializeWaveFormat(const WAVHeader& header) {
    // Set up the wave format structure with file header data
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = header.numChannels;
    waveFormat.nSamplesPerSec = header.sampleRate;
    waveFormat.wBitsPerSample = header.bitsPerSample;
    waveFormat.nBlockAlign = (waveFormat.wBitsPerSample * waveFormat.nChannels) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    // Close the existing waveOut device, if any, before opening a new one
    if (hWaveOut) {
        waveOutClose(hWaveOut);
        hWaveOut = nullptr;
    }

    // Open the wave output device
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        MessageBox(NULL, L"Failed to open audio output device!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}
void PlayAudioData() {
    if (audioData.empty()) {
        MessageBox(NULL, L"No audio data loaded!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (hWaveOut == nullptr) {
        MessageBox(NULL, L"Audio device is not initialized!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Prepare the WAVEHDR with audioData
    waveHeader.lpData = reinterpret_cast<LPSTR>(audioData.data());
    waveHeader.dwBufferLength = static_cast<DWORD>(audioData.size() * sizeof(short)); // Size in bytes
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    // Prepare and write audio data for playback
    MMRESULT result = waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR) {
        result = waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            MessageBox(NULL, L"Failed to play audio!", L"Error", MB_OK | MB_ICONERROR);
        }
    }
    else {
        MessageBox(NULL, L"Failed to prepare audio header!", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Przekaż dalej deklaracje funkcji dołączone w tym module kodu:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: W tym miejscu umieść kod.

    // Inicjuj ciągi globalne
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROCSYG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Wykonaj inicjowanie aplikacji:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROCSYG));

    MSG msg;

    // Główna pętla komunikatów:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


//
//  FUNKCJA: MyRegisterClass()
//
//  PRZEZNACZENIE: Rejestruje klasę okna.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROCSYG));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(RGB(50, 50, 50));
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PROCSYG);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNKCJA: InitInstance(HINSTANCE, int)
//
//   PRZEZNACZENIE: Zapisuje dojście wystąpienia i tworzy okno główne
//
//   KOMENTARZE:
//
//        W tej funkcji dojście wystąpienia jest zapisywane w zmiennej globalnej i
//        jest tworzone i wyświetlane okno główne programu.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Przechowuj dojście wystąpienia w naszej zmiennej globalnej

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNKCJA: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PRZEZNACZENIE: Przetwarza komunikaty dla okna głównego.
//
//  WM_COMMAND  - przetwarzaj menu aplikacji
//  WM_PAINT    - Maluj okno główne
//  WM_DESTROY  - opublikuj komunikat o wyjściu i wróć
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Analizuj zaznaczenia menu:
            switch (wmId)
            {

            case ID_EFFECTS_COMPRESSOR:
                break;
            case ID_EFFECTS_DISTORTION:
                break;
            case ID_EFFECTS_PHASER:
                break;
            case ID_EFFECTS_FLANGER:
                break;
            case ID_EFFECTS_CHORUS:
                break;
            case ID_EFFECTS_VINTAGE:
                break;
            case ID_EFFECTS_REVERSE:
                break;
            case ID_EFFECTS_REVERB:
                break;

            case ID_EFFECTS_DELAY:
                if (FileLoaded) {
                    std::vector<short> newAudioData;
                    double inverse = DELAY_TIME/ 1000;
                    double DelayFactor = round(44100 / inverse); //number of samples to delay
                    for (double i = DelayFactor; i < audioData.size(); i++) {
                        newAudioData.push_back(audioData[i] + DELAY_GAIN * audioData[i - DelayFactor] );
                    }

                    audioData.clear();
                    audioData = newAudioData;

                }
                break;

            case ID_PLIK_OPEN:
            {
                OPENFILENAME file;            // common dialog box structure
                WCHAR szFile[MAX_PATH] = { 0 }; // buffer for file name

                // Initialize OPENFILENAME structure
                ZeroMemory(&file, sizeof(file));
                file.lStructSize = sizeof(file);
                file.hwndOwner = hWnd;
                file.lpstrFile = szFile;
                file.nMaxFile = sizeof(szFile);
                file.lpstrFilter = L"WAV Files\0*.wav\0All Files\0*.*\0";
                file.nFilterIndex = 1;
                file.lpstrFileTitle = NULL;
                file.nMaxFileTitle = 0;
                file.lpstrInitialDir = NULL;
                file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                // Display the Open dialog box
                if (GetOpenFileName(&file) == TRUE) {
                    selectedFilePath = file.lpstrFile; // Save file path globally
                    FileLoaded = true;
                    InvalidateRect(hWnd, NULL, TRUE); // Force repaint to update screen


                    // Optionally, open the file
                    FILE* wavFile;

                    if (_wfopen_s(&wavFile, selectedFilePath.c_str(), L"rb") != 0) {
                        MessageBox(hWnd, L"Failed to open file!", L"Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    WAVHeader Header;
                    fread(&Header, sizeof(Header), 1, wavFile);

                    audioData.resize(Header.dataSize / sizeof(short));
                    fread(audioData.data(), sizeof(short), audioData.size(), wavFile);
                    fclose(wavFile);

                    if (!InitializeWaveFormat(Header)) {
                        MessageBox(hWnd, L"Audio playback initialization failed.", L"Error", MB_OK | MB_ICONERROR);
                    }

                }
            }
                break;

            case ID_BUTTON_PLAY:
                //PlaySound(selectedFilePath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
                PlayAudioData();
                break;


            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
        hButtonPlay = CreateWindow(
            L"BUTTON",
            L"Play",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            100,
            450,
            150,
            50,
            hWnd,
            (HMENU)ID_BUTTON_PLAY,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);

        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        if (!FileLoaded) {
            SetTextColor(hdc, RGB(0, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 50, 50, L"Welcome to ProcSyg!", 20);
            TextOut(hdc, 50, 80, L"Please open a file to begin.", 27);
        }
        else {
            SetTextColor(hdc, RGB(0, 128, 0));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 50, 50, L"File loaded successfully!", 25);
            TextOut(hdc, 50, 80, L"You can now process the file.", 28);

            // Draw the waveform
            int width = 1200; // Set the width for the drawing area
            int height = 1200; // Set the height for the drawing area
            int samplesPerPixel = audioData.size() / width; // Calculate samples to draw per pixel

            MoveToEx(hdc, 0, 0, NULL); // Start in the middle of the height

            for (int x = 0; x < width; ++x) {
                // Calculate the index in audioData
                int index = x * samplesPerPixel;
                if (index < audioData.size()) {
                    int y = (audioData[index] / 32768.0) * (height / 2); // Normalize and scale the value
                    LineTo(hdc, x, (height / 2) - y); // Draw line to the calculated point
                }
            }
        
        }
        EndPaint(hWnd, &ps);
    }
    break;



    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Procedura obsługi komunikatów dla okna informacji o programie.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
