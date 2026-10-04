#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ModelPeek - YouTube Official Upload Automation Tool
Uses official Google OAuth 2.0 & YouTube Data API v3.
"""

import os
import sys
import argparse
import json

if sys.stdout.encoding.lower() != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except Exception:
        pass

from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials

SCOPES = ["https://www.googleapis.com/auth/youtube.upload"]
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CLIENT_SECRETS_FILE = os.path.join(BASE_DIR, "client_secret.json")
TOKEN_FILE = os.path.join(BASE_DIR, "token.json")

VIDEO_PRESETS = {
    "shorts": {
        "file": os.path.join(BASE_DIR, "output", "ModelPeek_Shorts_9x16.mp4"),
        "title": "ModelPeek - Instant 3D CAD Preview for Windows Explorer #shorts #cad",
        "description": (
            "Tired of opening heavy CAD software just to check 3D files?\n"
            "Meet ModelPeek — the native 3D preview extension for Windows Explorer!\n\n"
            "🔥 Instant 3D thumbnails for STEP, STL, OBJ directly in folders\n"
            "⚡ Full 6-DOF interactive 3D preview pane with zero lag\n"
            "🎯 Wireframe topology, orthographic views & real-time HUD dimensions\n"
            "⭐ 100% Free & Open Source on GitHub: https://github.com/MIke-Slim/ModelPeek\n\n"
            "#ModelPeek #CAD #3DPrinting #Windows11 #Engineering #OpenSource #ThreeJS"
        ),
        "tags": [
            "ModelPeek", "3D CAD", "Windows Explorer", "STEP preview", "STL viewer",
            "3D model", "Windows 11", "Threejs", "WebGL", "Open Source", "Shorts"
        ],
        "category_id": "28"
    },
    "standard": {
        "file": os.path.join(BASE_DIR, "output", "ModelPeek_Product_16x9.mp4"),
        "title": "ModelPeek: Native 3D CAD Preview Extension for Windows Explorer (Overview & Features)",
        "description": (
            "ModelPeek is an ultra-fast, lightweight 3D preview extension seamlessly integrated into the Windows Shell.\n\n"
            "Inspect STEP, STL, OBJ, 3MF, and GLTF models directly within Windows Explorer folders without launching bulky CAD suites like SolidWorks, Fusion 360, or Blender.\n\n"
            "⏱️ TIMESTAMPS & CHAPTERS:\n"
            "00:00 - The CAD Workflow Problem\n"
            "00:13 - Introducing ModelPeek\n"
            "00:22 - Native 3D Thumbnail Provider (STEP/STL/OBJ)\n"
            "00:35 - Interactive Right-Pane 3D Viewport\n"
            "00:46 - Professional CAD Visualizer & Wireframe Mode\n"
            "00:58 - HUD Dimensions & Camera Presets\n"
            "01:10 - Zero Bloat & Sandbox Security Architecture\n"
            "01:23 - Get ModelPeek on GitHub\n\n"
            "🔗 Official GitHub Repository (Free & Open Source):\n"
            "https://github.com/MIke-Slim/ModelPeek\n\n"
            "✨ KEY CAPABILITIES:\n"
            "• Native Windows Shell thumbnail provider (IThumbnailProvider)\n"
            "• High-performance interactive preview pane (IPreviewHandler + WebView2)\n"
            "• Full 6-DOF orbit, pan, and zoom controls\n"
            "• Multi-angle orthographic presets (Top, Front, Right, Isometric)\n"
            "• Low-integrity sandbox safe architecture\n"
            "• Single-click portable installer\n\n"
            "#3DCAD #WindowsExplorer #STEP #STL #WebGL #OpenSourceSoftware #Engineering"
        ),
        "tags": [
            "ModelPeek", "3D CAD", "Windows Explorer 3D", "STEP file viewer",
            "STL preview", "OBJ viewer", "CAD tools", "Windows 10", "Windows 11",
            "WebView2", "Threejs", "CAD Viewer", "Open Source CAD"
        ],
        "category_id": "28"
    }
}

def get_authenticated_service():
    creds = None
    if os.path.exists(TOKEN_FILE):
        try:
            creds = Credentials.from_authorized_user_file(TOKEN_FILE, SCOPES)
        except Exception as e:
            print(f"[!] Existing token could not be loaded: {e}")

    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            print("[*] Refreshing access token...")
            creds.refresh(Request())
        else:
            if not os.path.exists(CLIENT_SECRETS_FILE):
                print(f"❌ Missing client_secret.json at: {CLIENT_SECRETS_FILE}")
                sys.exit(1)

            print("[*] 启动本地授权服务进行 Google OAuth 2.0 授权...")
            os.environ["OAUTHLIB_INSECURE_TRANSPORT"] = "1"
            flow = InstalledAppFlow.from_client_secrets_file(CLIENT_SECRETS_FILE, SCOPES)

            class AuthPromptHook:
                def format(self, url, **kwargs):
                    try:
                        with open(os.path.join(BASE_DIR, "auth_url.txt"), "w", encoding="utf-8") as f:
                            f.write(url.strip())
                    except Exception:
                        pass
                    try:
                        os.system(f'start "" "{url}"')
                    except Exception:
                        pass
                    msg = f"\n======================================================\n🔑 授权链接 (请在浏览器中打开并登录)：\n{url}\n======================================================\n"
                    print(msg, flush=True)
                    return msg

            creds = flow.run_local_server(
                port=8080,
                authorization_prompt_message=AuthPromptHook(),
                success_message="✅ 授权成功！您可以关闭此浏览器标签页，终端已开始自动上传视频。",
                open_browser=True,
                prompt="consent",
                access_type="offline"
            )

        with open(TOKEN_FILE, "w", encoding="utf-8") as token_f:
            token_f.write(creds.to_json())
        print(f"[*] 授权凭证已永久保存至: {TOKEN_FILE}", flush=True)

    return build("youtube", "v3", credentials=creds)

def upload_video(youtube, video_path, title, description, tags, category_id="28", privacy_status="public"):
    if not os.path.exists(video_path):
        print(f"❌ Video file does not exist: {video_path}")
        return None

    body = {
        "snippet": {
            "title": title,
            "description": description,
            "tags": tags,
            "categoryId": category_id
        },
        "status": {
            "privacyStatus": privacy_status,
            "selfDeclaredMadeForKids": False
        }
    }

    print("\n" + "=" * 60, flush=True)
    print("🚀 Initiating YouTube Upload...", flush=True)
    print(f"File:      {os.path.basename(video_path)} ({os.path.getsize(video_path) / 1024 / 1024:.2f} MB)", flush=True)
    print(f"Title:     {title}", flush=True)
    print(f"Privacy:   {privacy_status.upper()}", flush=True)
    print("=" * 60, flush=True)

    chunk_size = 10 * 1024 * 1024
    media = MediaFileUpload(video_path, chunksize=chunk_size, resumable=True, mimetype="video/mp4")

    request = youtube.videos().insert(
        part="snippet,status",
        body=body,
        media_body=media
    )

    response = None
    while response is None:
        status, response = request.next_chunk()
        if status:
            pct = int(status.progress() * 100)
            print(f"[*] Upload Progress: {pct}% ...", flush=True)

    video_id = response.get("id")
    video_url = f"https://youtu.be/{video_id}"
    print("\n" + "=" * 60, flush=True)
    print(f"🎉 VIDEO UPLOADED SUCCESSFULLY!", flush=True)
    print(f"Video ID:  {video_id}", flush=True)
    print(f"Watch URL: {video_url}", flush=True)
    print("=" * 60 + "\n", flush=True)
    return response

def main():
    parser = argparse.ArgumentParser(description="Automated YouTube Upload Tool for ModelPeek")
    parser.add_argument("--video", choices=["shorts", "standard", "all"], default="all")
    parser.add_argument("--privacy", choices=["unlisted", "public", "private"], default="public")
    args = parser.parse_args()

    youtube = get_authenticated_service()

    targets = []
    if args.video == "all":
        targets.append(VIDEO_PRESETS["shorts"])
        targets.append(VIDEO_PRESETS["standard"])
    else:
        targets.append(VIDEO_PRESETS[args.video])

    for target in targets:
        upload_video(
            youtube=youtube,
            video_path=target["file"],
            title=target["title"],
            description=target["description"],
            tags=target["tags"],
            category_id=target["category_id"],
            privacy_status=args.privacy
        )

if __name__ == "__main__":
    main()
