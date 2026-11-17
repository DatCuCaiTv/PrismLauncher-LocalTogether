# PrismLauncher Team

## Máy chủ Node.js

Máy chủ dùng Node.js 20 trở lên và lưu gói trong `server/packs`.

```powershell
$env:TEAM_UPLOAD_TOKEN = "thay-bang-mot-chuoi-bi-mat-dai"
$env:PORT = "25832"
node server.js
```

Để phát hành trực tiếp, chép ZIP vào `server/packs`. Launcher tự quét và dùng ZIP mới nhất theo ngày sửa đổi; không cần tạo file JSON/metadata. Có thể giữ lại ZIP cũ để dự phòng. Trang upload là `http://fusion.pikamc.vn:25832/admin` và cần mã `TEAM_UPLOAD_TOKEN`.

ZIP được tải về rồi launcher hỏi tên thư mục mới và giải nén toàn bộ vào `%APPDATA%\PrismLauncherTeam\instances\<tên>`. Để hiện trong danh sách và chạy được, ZIP cần có `mmc-pack.json`; nếu thiếu `instance.cfg`, launcher tự tạo file cơ bản cho Prism. Nên dùng ZIP export instance của Prism để giữ đúng cấu hình phiên bản và loader.

### Tin tức nhóm

Tạo thư mục `server/news` trên host và chép các bài UTF-8 `.md` vào đó. Launcher lấy bài mới nhất theo thời gian sửa đổi file để hiện ở thanh tin; nút **News history** cho xem các bài cũ. Đặt tiêu đề bằng dòng đầu dạng `# Tiêu đề`. Giữ các file cũ để chúng tiếp tục hiện trong lịch sử. Launcher tự làm mới tin mỗi 10 phút.

## Launcher

- Tất cả dữ liệu và instance mới được lưu trong `%APPDATA%\PrismLauncherTeam`; thư mục instance là `%APPDATA%\PrismLauncherTeam\instances`.
- Chọn **Tải mod nhóm** trên thanh công cụ để tải ZIP mới nhất, đặt tên thư mục instance rồi giải nén. Không cần chọn sẵn instance.
- Đăng nhập Microsoft và tài khoản offline đều được giữ. Vì thư mục dữ liệu mới tách riêng, lần đầu chạy có thể cần đăng nhập Microsoft lại.
- Tiêu đề cửa sổ: `PrismLauncher mod by Phan Tấn Đạt`.

## Build trên Windows

Cần Visual Studio 2022 workload **Desktop development with C++**, Qt 6.8+ MSVC x64, JDK 8 trở lên và CMake. Mở **Developer PowerShell for VS 2022** ở thư mục gốc dự án.

Cấu hình lần đầu (đường dẫn Qt bên dưới đúng với máy hiện tại):

```powershell
$toolchain = (Resolve-Path '.\cmake\vcpkg\scripts\buildsystems\vcpkg.cmake').Path
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64" -DLauncher_APP_BINARY_SUFFIX=-team-assets -DBUILD_TESTING=OFF -DENABLE_LTO=OFF
```

Build và chép thư viện Qt cạnh chương trình:

```powershell
cmake --build build-msvc --config Release --target PrismLauncher
& 'C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe' --release --compiler-runtime --dir 'build-msvc\Release' 'build-msvc\Release\prismlauncher-team-assets.exe'
```

Mở `build-msvc\Release\prismlauncher-team-assets.exe`. Nếu gửi launcher cho máy khác, gửi cả thư mục `Release`, không chỉ riêng file EXE.
