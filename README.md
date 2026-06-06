# Authon ImGui Loader

Premium ImGui-based software loader with Authon authentication integration. DirectX 11 rendering, modern dark UI, process hollowing execution.

![Authon](https://authon.pro/logo.png)


## Menu İmage

![Authon]([https://authon.pro/logo.png](https://github.com/authonpro/imgui-Keyauth-alternative-Loader/blob/main/Menu.png))
![Authon]([https://authon.pro/logo.png](https://github.com/authonpro/imgui-Keyauth-alternative-Loader/blob/main/Menu2.png))

## Features

- **Authentication** — Login / Register with username + password + license key
- **HWID Locking** — Automatic hardware ID validation
- **Process Hollowing** — Fileless execution, invisible to Task Manager
- **Encrypted Delivery** — Files downloaded via authenticated session (AES-256)
- **Modern UI** — Dark theme, animated transitions, toast notifications
- **DirectX 11** — Hardware-accelerated rendering
- **Segoe UI Font** — Clean Windows system font with icons

## Screenshots

### Login Page
- Split layout: left form + right banner image
- Username / Password / License Key inputs with icons
- Sign In / Sign Up toggle with staggered animations
- Toast notifications for errors and success

### Dashboard
- Top bar with AUTHON.PRO branding
- Left sidebar with icon navigation (Main / Profile / Logout)
- Account overview with subscription & security info
- Card-based modern layout

## Setup

1. **Clone this repository**
2. **Run `setup.bat`** — downloads ImGui + stb_image from GitHub
3. **Edit `config.h`** — set your APP_ID, API_KEY, FILE_ID
4. **Open `AuthonLoader.sln`** in Visual Studio 2022
5. **Build** (Release x64)
6. **Run** the executable

## Configuration

Edit `config.h`:
```cpp
#define APP_ID   "your-app-id-from-dashboard"
#define API_KEY  "your-api-key-from-dashboard"
#define FILE_ID  "abc123"  // 6-char file ID
```

Get these values from [authon.pro/dashboard](https://authon.pro/dashboard)

## Requirements

- Windows 10/11
- Visual Studio 2022 (v143 toolset)
- DirectX 11 (included in Windows)
- C++17

## How It Works

1. User enters credentials → Authon API validates
2. Session token received → used for file download
3. Protected file downloaded as raw bytes
4. Process Hollowing: new hidden process created (svchost.exe)
5. PE mapped into remote process memory
6. Entry point executed → module runs invisibly
7. Loader closes, module continues in background

## Security

- No file written to disk
- Process invisible in Task Manager
- HWID locked to specific hardware
- Session-based authentication
- Anti-debug compatible

## Tech Stack

- C++17
- DirectX 11
- Dear ImGui (v1.90.4)
- WinHTTP (native Windows networking)
- stb_image (texture loading)

## Links

- **Website:** [authon.pro](https://authon.pro)
- **Documentation:** [authon.pro/docs](https://authon.pro/docs)
- **Dashboard:** [authon.pro/dashboard](https://authon.pro/dashboard)
- **Discord:** [Join](https://discord.gg/MTY79JDFm6)

## License

This loader template is provided for Authon platform users. Use with your own Authon application credentials.

---

Built with [Authon](https://authon.pro) — Modern Software Licensing & Authentication Platform
