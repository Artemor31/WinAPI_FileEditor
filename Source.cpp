#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <stdio.h>
#include <locale.h>
#include <strsafe.h>

#include <shlwapi.h>
#include <string>
#include "resource.h"
#pragma comment(lib, "shlwapi.lib")


HWND hWnd = NULL;

static int sx, sy;

HWND edit = NULL;
HWND edit_2 = NULL;
HWND Button1 = NULL;

HBRUSH brush = (HBRUSH)GetStockBrush(000000);

TCHAR bufferForName[MAX_PATH];  
TCHAR bufferForNewPath[MAX_PATH];  

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);


typedef BOOL(__stdcall* LPSEARCHFUNC)(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);

BOOL __stdcall FileMove(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);

// функция, выполняющая операцию с файлом или каталогом
BOOL FileOperation(LPCTSTR lpszFileName, LPCTSTR lpTargetDirectory, LPSEARCHFUNC lpFunc);

// функция, которая выполняет поиск внутри каталога
BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR lpszDirName, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam);


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow)
{
    MSG  Massage;
    BOOL IsError;

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = MyWindowProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wcex.lpszClassName = TEXT("MyWindowClass");
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);


    if (0 == RegisterClassEx(&wcex))
    {
        return -1;
    }
    LoadLibrary(TEXT("ComCtl32.dll"));

    hWnd = CreateWindowEx(0, TEXT("MyWindowClass"), TEXT("Lab_4_3_WindowApp"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 550, 400, NULL, NULL, hInstance, NULL);

    if (NULL == hWnd)
    {
        return -1;
    }
    ShowWindow(hWnd, nCmdShow);

    while (true)
    {
        if (PeekMessage(&Massage, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            IsError = GetMessage(&Massage, NULL, 0, 0);
            if (IsError == -1 || IsError == FALSE)
            {
                return FALSE;
            }
            else
            {
                TranslateMessage(&Massage);
                DispatchMessage(&Massage);
            }
        }
    }
    return static_cast<int>(Massage.wParam);
}
LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);


    case WM_SIZE:
        sx = LOWORD(lParam); 
        sy = HIWORD(lParam);
        LPCWSTR buff;
        wchar_t str[20];
        _itow_s(sx, str, 10); 
        if (sx < 380)        
            MoveWindow(Button1, 10, 80, 130, 30, true);        
        else        
            MoveWindow(Button1, 380, 10, 130, 30, true); 
        return 0;


    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        FillRect(ps.hdc, &ps.rcPaint, brush);
        EndPaint(hWnd, &ps);
        return 0;


    case WM_DESTROY:
        PostQuitMessage(ERROR_SUCCESS);
        return 0;

    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    // выбор кталога или файла
    edit = CreateWindowEx(0, TEXT("Edit"), TEXT("Enter the folder/file path"),
        WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_BORDER,
        10, 10, 350, 20, hwnd, NULL, lpCreateStruct->hInstance, NULL);   

    // выбор куда перемещать
    edit_2 = CreateWindowEx(0, TEXT("Edit"), TEXT("Enter the destination folder path"),
            WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_BORDER,
            10, 50, 350, 20, hwnd, NULL, lpCreateStruct->hInstance, NULL);

    // qwerqwer
    Button1 = CreateWindowEx(0, TEXT("Button"), TEXT("Relocate"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 380, 10, 130, 30, hwnd, (HMENU)RELOCATE, lpCreateStruct->hInstance, NULL);

    return TRUE;
}
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case RELOCATE:

        GetWindowText(edit, bufferForName, MAX_PATH);
        GetWindowText(edit_2, bufferForNewPath, MAX_PATH);

        BOOL bRet = FileOperation(bufferForName, bufferForNewPath, FileMove);

        if (FALSE != bRet) MessageBox(hwnd, L"Nice", L"Operation result", MB_OK);
        else MessageBox(hwnd, L"Bad", L"Operation result", MB_OK);
        break;
    }
}

            
/// поиск файла и перемещение в новыое место
BOOL __stdcall FileMove(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam)
{
    LPCTSTR lpTargetDirectory = (LPCTSTR)lpvParam; // каталог, в который нужно переместить файл/каталог

    TCHAR szNewFileName[MAX_PATH]; // новое имя файла/каталога
    StringCchPrintf(szNewFileName, _countof(szNewFileName), TEXT("%s\\%s"), (LPCTSTR)lpTargetDirectory, PathFindFileName(lpszFileName));

    if (lpFileAttributeData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // нужно переместить каталог
    {
        // создаём новый каталог (атрибуты копируются)
        BOOL bRet = CreateDirectoryEx(lpszFileName, szNewFileName, NULL);

        if ((FALSE != bRet) || (GetLastError() == ERROR_ALREADY_EXISTS)) // (!) ошибка: Невозможно создать каталог, так как он уже существует.
        {
            // продолжим поиск внутри каталога
            bRet = FileSearch(TEXT("*"), lpszFileName, FileMove, szNewFileName);
        } 

        if (FALSE != bRet)
        {
            // удаляем каталог
            bRet = RemoveDirectory(lpszFileName);
        } 
        return bRet;
    } 
    return MoveFileEx(lpszFileName, szNewFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
}

BOOL FileOperation(LPCTSTR lpszFileName, LPCTSTR lpTargetDirectory, LPSEARCHFUNC lpFunc)
{
    if (NULL != lpTargetDirectory)
    {
        DWORD dwFileAttributes = GetFileAttributes(lpTargetDirectory);

        if (INVALID_FILE_ATTRIBUTES == dwFileAttributes) // (!) ошибка
        {
            return FALSE;
        } 
        else if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) // (!) не является каталогом
        {
            SetLastError(ERROR_PATH_NOT_FOUND);
            return FALSE;
        } 
    } 

    WIN32_FILE_ATTRIBUTE_DATA fad;

    // получим атрибутивную информацию файла/каталога
    BOOL bRet = GetFileAttributesEx(lpszFileName, GetFileExInfoStandard, &fad);

    if (FALSE != bRet)
    {
        // выполняем операцию с файлом/каталогом
        bRet = lpFunc(lpszFileName, &fad, (LPVOID)lpTargetDirectory);
    } 

    return bRet;
} 



BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR lpszDirName, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam)
{
    WIN32_FIND_DATA fd;
    TCHAR szFileName[MAX_PATH];

    // формируем шаблон поиска
    StringCchPrintf(szFileName, MAX_PATH, TEXT("%s\\%s"), lpszDirName, lpszFileName);

    // начинаем поиск
    HANDLE hFindFile = FindFirstFile(szFileName, &fd);
    if (INVALID_HANDLE_VALUE == hFindFile) return FALSE;

    BOOL bRet = TRUE;

    for (BOOL bFindNext = TRUE; FALSE != bFindNext; bFindNext = FindNextFile(hFindFile, &fd))
    {
        if (_tcscmp(fd.cFileName, TEXT(".")) == 0 || _tcscmp(fd.cFileName, TEXT("..")) == 0)
        {
            /* пропускаем текущий и родительский каталог */
            continue;
        } 

        // формируем полный путь к файлу
        StringCchPrintf(szFileName, MAX_PATH, TEXT("%s\\%s"), lpszDirName, fd.cFileName);

        bRet = lpSearchFunc(szFileName, (LPWIN32_FILE_ATTRIBUTE_DATA)&fd, lpvParam);
        if (FALSE == bRet) break; // прерываем поиск
    } 

    FindClose(hFindFile); // завершаем поиск
    return bRet;
}