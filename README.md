<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="program_info/org.prismlauncher.PrismLauncher.logo-darkmode.svg">
    <source media="(prefers-color-scheme: light)" srcset="program_info/org.prismlauncher.PrismLauncher.logo.svg">
    <img alt="Prism Launcher" src="program_info/org.prismlauncher.PrismLauncher.logo.svg" width="40%">
  </picture>
</p>

<h1 align="center">PrismLauncher Team</h1>

<p align="center">
  Bản tùy biến Prism Launcher dành cho nhóm chơi Minecraft nội bộ.<br>
  Tên hiển thị trong ứng dụng: <b>PrismLauncher mod by Phan Tấn Đạt</b>.
</p>

> Đây là bản fork do cộng đồng/team tự phát triển, không phải bản phát hành chính thức và không được Prism Launcher hoặc MultiMC bảo trợ.

## Giới thiệu

PrismLauncher Team kế thừa nền tảng quản lý nhiều phiên bản Minecraft của Prism Launcher, đồng thời bổ sung các tiện ích cho nhóm:

- Đăng nhập bằng tài khoản Microsoft; hỗ trợ thêm hồ sơ offline để thử nghiệm hoặc chơi trên máy chủ cho phép offline.
- Tải gói phiên bản của nhóm từ máy chủ, đặt tên instance mới rồi giải nén vào thư mục riêng.
- Hiển thị tin tức nhóm từ các tệp Markdown và xem lại bài cũ trong lịch sử.
- Tìm và quản lý mod qua các nguồn được launcher hỗ trợ, gồm Modrinth và CurseForge.
- Lưu dữ liệu tách biệt khỏi Prism Launcher bản thường tại `%APPDATA%\PrismLauncherTeam`.
- Giao diện và tiêu đề được tùy biến cho nhóm.

### Hồ sơ offline

Hồ sơ offline chỉ là tên người chơi cục bộ; nó không xác thực quyền sở hữu Minecraft và không thay thế tài khoản Microsoft. Máy chủ bật xác thực online có thể từ chối hồ sơ offline.

## Tải gói phiên bản của nhóm

Máy chủ nội bộ: [`http://fusion.pikamc.vn:25832`](http://fusion.pikamc.vn:25832)

1. Trên máy chủ Node.js, đặt tệp ZIP phiên bản vào thư mục `packs/` và để API của máy chủ truy cập được tệp đó.
2. Trong launcher, chọn **Tải mod nhóm** rồi nhấn **Refresh** để tải danh sách mới.
3. Chọn ZIP, nhập tên instance mới và tải xuống. Launcher giải nén gói vào thư mục instances riêng.

ZIP dùng cho tính năng này cần chứa **một Prism Launcher instance hoàn chỉnh** ở ngay bên trong gói. Không dùng ZIP chỉ có thư mục `mods/`; tính năng này cũng không phải trình nhập `.mrpack`. Giữ các tệp cấu hình và thư mục game cần thiết trong gói để instance có thể khởi chạy.

Instances của Team được lưu tại:

```text
%APPDATA%\PrismLauncherTeam\instances
```

Thư viện, tài nguyên và cấu hình Team cũng được lưu trong thư mục dữ liệu riêng `PrismLauncherTeam`; chúng không dùng chung thư mục dữ liệu của Prism Launcher thường.

## Tin tức nhóm

Đặt các bài viết `.md` vào thư mục `news/` trên máy chủ Node.js. Launcher hiển thị bài mới nhất; các bài cũ vẫn có thể mở từ **News history**. Dùng tên tệp dễ nhận biết, chẳng hạn `update1.md`, `update2.md`.

## Cài đặt và build

Bản Windows đóng gói có tên `PrismLauncherTeam-Setup.exe`. Mặc định bộ cài đặt vào:

```text
%LOCALAPPDATA%\Programs\PrismLauncherTeam
```

Thư mục ứng dụng này độc lập với dữ liệu người dùng ở `%APPDATA%\PrismLauncherTeam`. Khi gỡ ứng dụng, không xóa thư mục dữ liệu nếu bạn muốn giữ instance, mod và hồ sơ.

Để tự build từ mã nguồn, tham khảo [hướng dẫn build Prism Launcher](https://prismlauncher.org/wiki/development/build-instructions). Bản Windows cần cấu hình Qt, MSVC và vcpkg tương ứng với máy build; hãy build trong thư mục riêng, không đặt thư mục build bên trong cây mã nguồn.

## Nguồn gốc, bản quyền và giấy phép

PrismLauncher Team là một bản fork dựa trên [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher). Prism Launcher được phát triển từ MultiMC. Bản tùy biến này không đại diện cho các dự án upstream.

- Giữ nguyên các thông báo bản quyền và ghi công có trong mã nguồn, thư viện, tài liệu và tài sản được kế thừa. Các tác giả upstream vẫn giữ bản quyền đối với phần đóng góp của họ.
- Mã nguồn launcher được phát hành theo **GPL-3.0-only**. Xem [`LICENSE`](LICENSE) và [`COPYING.md`](COPYING.md); khi phân phối bản sửa đổi, hãy tuân thủ đầy đủ điều khoản GPL.
- Logo và tài sản liên quan của Prism Launcher thuộc **CC BY-SA 4.0** theo thông tin cấp phép upstream. Xem thông báo giấy phép đi kèm từng tài sản.
- Không tuyên bố đây là sản phẩm chính thức, được chứng nhận hoặc được Prism Launcher, MultiMC, Microsoft, Mojang hay CurseForge bảo trợ.
- Nếu phân phối bản build có dùng API key mặc định từ upstream, hãy đọc [chính sách fork và custom build của Prism Launcher](https://github.com/PrismLauncher/PrismLauncher#forkingredistributingcustom-builds-policy). Chính sách này yêu cầu tuân thủ điều khoản của Microsoft Identity Platform và CurseForge; có thể đặt key tương ứng thành chuỗi rỗng để tắt tính năng phụ thuộc vào key đó.

Tên Minecraft và các nhãn hiệu liên quan thuộc về chủ sở hữu tương ứng. Đây là dự án cộng đồng dành cho nhóm, không liên kết với Mojang hoặc Microsoft.
