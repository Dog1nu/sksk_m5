#!/usr/bin/env python3
import os
import re
import shutil
from pathlib import Path

# PlatformIOの環境から情報を取得
Import("env")

def post_build_action(source, target, env):
    # ビルド成果物のパス
    build_dir = Path(env.subst("${BUILD_DIR}"))
    firmware_file = build_dir / "firmware.bin"

    print(f"[POST_BUILD] Checking firmware at: {firmware_file}")

    # UIV3_*.ino からデバイス情報を抽出
    device_type = "unknown"
    device_no = "01"
    version = "0.0.0"

    ino_files = list(Path("src").glob("UIV3_*.ino"))
    if ino_files:
        with open(ino_files[0], 'r', encoding='utf-8') as f:
            content = f.read()
            
            # device_type を抽出 (const char* device_type = "geothermal";)
            match = re.search(r'const char\* device_type\s*=\s*"([^"]+)"', content)
            if match:
                device_type = match.group(1)
            
            # deviceNO を抽出 (const char* deviceNO = "02";)
            match = re.search(r'const char\* deviceNO\s*=\s*"([^"]+)"', content)
            if match:
                device_no = match.group(1)
            
            # version を抽出 (const char* version = "v1.0.1";)
            match = re.search(r'const char\* version\s*=\s*"v?([^"]+)"', content)
            if match:
                version = match.group(1)

    # デバイスラベルを作成 (最初の3文字)
    device_label = device_type[:3].lower()

    # ファイル名を作成
    new_filename = f"{device_label}-{device_no}-v{version}.bin"

    # プロジェクトルート(platformio.ini がある場所)を取得
    project_dir = Path(env.subst("${PROJECT_DIR}"))
    local_firmware_dir = project_dir.parent / "FIRMWARE"

    # ファイルをコピー
    try:
        if firmware_file.exists():
            local_firmware_dir.mkdir(parents=True, exist_ok=True)
            
            # 同じデバイスラベルの古いファイルを削除
            pattern = f"{device_label}-{device_no}-v*.bin"
            for old_file in local_firmware_dir.glob(pattern):
                if old_file.name != new_filename:
                    old_file.unlink()
                    print(f"[BUILD] 🗑️  Removed old version: {old_file.name}")
            
            dest_file = local_firmware_dir / new_filename
            shutil.copy2(firmware_file, dest_file)

            print(f"\n[BUILD] ✓ Firmware copied to: {dest_file}")
            print(f"[BUILD] Device: {device_label}-{device_no}")
            print(f"[BUILD] Version: {version}")
        else:
            print(f"\n[BUILD ERROR] Firmware file not found: {firmware_file}")
    except Exception as e:
        print(f"\n[BUILD ERROR] Failed to copy firmware: {e}")
        print(f"[BUILD ERROR] Source: {firmware_file}")
        print(f"[BUILD ERROR] Destination: {local_firmware_dir / new_filename}")

# ビルド後に実行
env.AddPostAction("$BUILD_DIR/firmware.bin", post_build_action)
