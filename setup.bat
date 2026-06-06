@echo off
echo ============================================
echo  Authon ImGui Loader - Setup
echo  Downloading ImGui files from GitHub...
echo ============================================
echo.

if not exist "imgui" mkdir imgui

echo Downloading imgui.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui.h' -OutFile 'imgui/imgui.h'"

echo Downloading imgui.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui.cpp' -OutFile 'imgui/imgui.cpp'"

echo Downloading imgui_draw.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui_draw.cpp' -OutFile 'imgui/imgui_draw.cpp'"

echo Downloading imgui_tables.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui_tables.cpp' -OutFile 'imgui/imgui_tables.cpp'"

echo Downloading imgui_widgets.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui_widgets.cpp' -OutFile 'imgui/imgui_widgets.cpp'"

echo Downloading imgui_demo.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui_demo.cpp' -OutFile 'imgui/imgui_demo.cpp'"

echo Downloading imgui_internal.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imgui_internal.h' -OutFile 'imgui/imgui_internal.h'"

echo Downloading imstb_rectpack.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imstb_rectpack.h' -OutFile 'imgui/imstb_rectpack.h'"

echo Downloading imstb_textedit.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imstb_textedit.h' -OutFile 'imgui/imstb_textedit.h'"

echo Downloading imstb_truetype.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imstb_truetype.h' -OutFile 'imgui/imstb_truetype.h'"

echo Downloading imconfig.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/imconfig.h' -OutFile 'imgui/imconfig.h'"

echo Downloading imgui_impl_dx9.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_dx9.h' -OutFile 'imgui/imgui_impl_dx9.h'"

echo Downloading imgui_impl_dx9.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_dx9.cpp' -OutFile 'imgui/imgui_impl_dx9.cpp'"

echo Downloading imgui_impl_dx11.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_dx11.h' -OutFile 'imgui/imgui_impl_dx11.h'"

echo Downloading imgui_impl_dx11.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_dx11.cpp' -OutFile 'imgui/imgui_impl_dx11.cpp'"

echo Downloading imgui_impl_win32.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_win32.h' -OutFile 'imgui/imgui_impl_win32.h'"

echo Downloading imgui_impl_win32.cpp...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/ocornut/imgui/v1.90.4/backends/imgui_impl_win32.cpp' -OutFile 'imgui/imgui_impl_win32.cpp'"

echo Downloading stb_image.h...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image.h' -OutFile 'imgui/stb_image.h'"

echo.
echo ============================================
echo  Done! Open AuthonLoader.sln in Visual Studio
echo  Edit config.h with your credentials
echo  Build in Release x64
echo ============================================
pause
