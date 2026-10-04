import os
import sys
import json
import time
import math
import subprocess

# Ensure UTF-8 output on Windows console
if sys.stdout.encoding.lower() != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except Exception:
        pass

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import imageio_ffmpeg

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ASSETS_DIR = os.path.join(BASE_DIR, "assets")
OUTPUT_DIR = os.path.join(BASE_DIR, "output")
os.makedirs(OUTPUT_DIR, exist_ok=True)

FFMPEG_EXE = imageio_ffmpeg.get_ffmpeg_exe()

FONT_BOLD_PATH = "C:/Windows/Fonts/segoeuib.ttf"
FONT_REG_PATH = "C:/Windows/Fonts/segoeui.ttf"

def get_font(size, bold=True):
    path = FONT_BOLD_PATH if bold else FONT_REG_PATH
    try:
        return ImageFont.truetype(path, size)
    except Exception:
        return ImageFont.load_default()

def draw_rounded_rect(draw, bbox, radius, fill=None, outline=None, width=1):
    draw.rounded_rectangle(bbox, radius=radius, fill=fill, outline=outline, width=width)

def create_gradient_bg(width, height, c1=(10, 15, 24), c2=(18, 28, 44)):
    base = Image.new("RGBA", (width, height), (0, 0, 0, 255))
    draw = ImageDraw.Draw(base)
    for y in range(height):
        factor = y / float(height)
        r = int(c1[0] + (c2[0] - c1[0]) * factor)
        g = int(c1[1] + (c2[1] - c1[1]) * factor)
        b = int(c1[2] + (c2[2] - c1[2]) * factor)
        draw.line([(0, y), (width, y)], fill=(r, g, b, 255))
    return base

def load_and_cache_assets():
    cached = {}
    for f in os.listdir(ASSETS_DIR):
        if f.endswith(".png") or f.endswith(".jpg"):
            path = os.path.join(ASSETS_DIR, f)
            cached[f] = Image.open(path).convert("RGBA")
    return cached

def crop_and_scale_asset(img, target_w, target_h, zoom=1.0, pan_x=0.0):
    src_w, src_h = img.size
    aspect_target = target_w / target_h
    aspect_src = src_w / src_h

    if aspect_src > aspect_target:
        crop_h = src_h
        crop_w = int(src_h * aspect_target)
    else:
        crop_w = src_w
        crop_h = int(src_w / aspect_target)

    crop_w = max(10, int(crop_w / zoom))
    crop_h = max(10, int(crop_h / zoom))

    cx = src_w // 2 + int(pan_x * (src_w - crop_w) * 0.5)
    cy = src_h // 2

    x1 = max(0, min(src_w - crop_w, cx - crop_w // 2))
    y1 = max(0, min(src_h - crop_h, cy - crop_h // 2))
    x2 = x1 + crop_w
    y2 = y1 + crop_h

    cropped = img.crop((x1, y1, x2, y2))
    return cropped.resize((target_w, target_h), Image.Resampling.BILINEAR)

# ==============================================================================
# 1. YouTube Shorts (9:16 Vertical 1080x1920)
# ==============================================================================
def render_shorts():
    print("\n=======================================================")
    print("[*] Rendering Clean YouTube Shorts (1080x1920 Vertical, 30 FPS)...")
    print("=======================================================")

    timeline_path = os.path.join(BASE_DIR, "timeline_shorts.json")
    with open(timeline_path, "r", encoding="utf-8") as f:
        timeline = json.load(f)

    total_dur = timeline["total_duration"]
    segments = timeline["segments"]
    master_audio = timeline["master_audio"]
    out_video = os.path.join(OUTPUT_DIR, "ModelPeek_Shorts_9x16.mp4")

    width, height = 1080, 1920
    fps = 30
    total_frames = int(total_dur * fps)
    assets = load_and_cache_assets()

    # Pre-render base background with header & CTA
    bg_base = create_gradient_bg(width, height, c1=(10, 15, 24), c2=(18, 28, 44))
    bdraw = ImageDraw.Draw(bg_base)

    # Top pill
    pill_w, pill_h = 440, 50
    pill_x = (width - pill_w) // 2
    pill_y = 90
    draw_rounded_rect(bdraw, (pill_x, pill_y, pill_x + pill_w, pill_y + pill_h), radius=25, fill=(2, 132, 199, 230))
    bdraw.text((width // 2, pill_y + 25), "⚡ MODELPEEK 3D EXTENSION", fill=(255, 255, 255), font=get_font(24, bold=True), anchor="mm")

    # Big App Title
    bdraw.text((width // 2, 185), "Windows 3D CAD Preview", fill=(248, 250, 252), font=get_font(44, bold=True), anchor="mm")
    bdraw.text((width // 2, 235), "Native Explorer Integration • Zero Software Lag", fill=(148, 163, 184), font=get_font(24, bold=False), anchor="mm")

    # Static bottom CTA button
    cta_w, cta_h = 760, 72
    cta_x = (width - cta_w) // 2
    cta_y = 1760
    draw_rounded_rect(bdraw, (cta_x, cta_y, cta_x + cta_w, cta_y + cta_h), radius=36, fill=(14, 165, 233, 230), outline=(56, 189, 248), width=2)
    bdraw.text((width // 2, cta_y + 36), "⭐ github.com/MIke-Slim/ModelPeek", fill=(255, 255, 255), font=get_font(28, bold=True), anchor="mm")

    # Pre-render info card layers per segment
    info_cards = {}
    info_w, info_h = 960, 310
    info_x = (width - info_w) // 2
    info_y = 1410

    card_mask = Image.new("L", (960, 1080), 0)
    mdraw = ImageDraw.Draw(card_mask)
    draw_rounded_rect(mdraw, (0, 0, 960, 1080), radius=28, fill=255)

    bullets = [
        "✔ 100% Native Shell Integration",
        "✔ Instant Click-and-View 3D Viewport",
        "✔ STEP • STL • OBJ • 3MF Supported",
        "✔ Real-Time 6-DOF Orbit & Wireframe Mode",
        "✔ Precise Dimension & Triangle Stats",
        "✔ Low-Integrity Explorer Sandbox Safe",
        "✔ Free & Open Source on GitHub"
    ]

    for idx, seg in enumerate(segments):
        card_img = Image.new("RGBA", (info_w, info_h), (0, 0, 0, 0))
        cdraw = ImageDraw.Draw(card_img)
        draw_rounded_rect(cdraw, (0, 0, info_w, info_h), radius=24, fill=(15, 23, 42, 240), outline=(51, 65, 85, 220), width=2)

        tag_text = f"FEATURE 0{idx + 1} / 0{len(segments)}"
        cdraw.text((30, 35), tag_text, fill=(56, 189, 248), font=get_font(20, bold=True))

        sub_title = seg["subtitle"]
        if len(sub_title) > 30 and " " in sub_title:
            mid = sub_title.rfind(" ", 0, 30)
            if mid == -1: mid = 30
            line1 = sub_title[:mid]
            line2 = sub_title[mid+1:]
        else:
            line1 = sub_title
            line2 = ""

        cdraw.text((30, 90), line1, fill=(255, 255, 255), font=get_font(38, bold=True))
        if line2:
            cdraw.text((30, 140), line2, fill=(255, 255, 255), font=get_font(38, bold=True))
            desc_y = 205
        else:
            desc_y = 165

        bullet_text = bullets[idx % len(bullets)]
        cdraw.text((30, desc_y), bullet_text, fill=(148, 163, 184), font=get_font(24, bold=False))
        info_cards[idx] = card_img

    # Launch FFmpeg
    cmd = [
        FFMPEG_EXE, "-y",
        "-f", "rawvideo",
        "-vcodec", "rawvideo",
        "-s", f"{width}x{height}",
        "-pix_fmt", "rgb24",
        "-r", str(fps),
        "-i", "-",
        "-i", master_audio,
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-crf", "20",
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "192k",
        "-shortest",
        out_video
    ]
    pipe = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    card_w, card_h = 960, 1080
    card_x = (width - card_w) // 2
    card_y = 280

    start_wall_time = time.time()
    for frame_idx in range(total_frames):
        t = frame_idx / float(fps)

        # Active segment
        cur_seg = segments[0]
        seg_idx = 0
        for i, s in enumerate(segments):
            if s["start"] <= t < s["end"]:
                cur_seg = s
                seg_idx = i
                break
            elif t >= s["end"]:
                cur_seg = s
                seg_idx = i

        seg_dur = max(0.1, cur_seg["end"] - cur_seg["start"])
        seg_progress = min(1.0, max(0.0, (t - cur_seg["start"]) / seg_dur))

        zoom = 1.0 + 0.05 * seg_progress
        pan_x = 0.03 * math.sin(seg_progress * math.pi)

        frame = bg_base.copy()

        # Paste 3D Card
        asset_name = cur_seg["asset"]
        if asset_name in assets:
            scaled_asset = crop_and_scale_asset(assets[asset_name], card_w, card_h, zoom=zoom, pan_x=pan_x)
            frame.paste(scaled_asset, (card_x, card_y), card_mask)

        draw = ImageDraw.Draw(frame)
        draw_rounded_rect(draw, (card_x, card_y, card_x + card_w, card_y + card_h), radius=28, outline=(56, 189, 248), width=3)

        # Viewport Status Tag
        hud_w, hud_h = 240, 38
        draw_rounded_rect(draw, (card_x + 20, card_y + 20, card_x + 20 + hud_w, card_y + 20 + hud_h), radius=10, fill=(15, 23, 42, 220), outline=(51, 65, 85), width=1)
        pulse = int(180 + 75 * math.sin(t * 6))
        draw.ellipse((card_x + 35, card_y + 33, card_x + 47, card_y + 45), fill=(34, pulse, 94))
        draw.text((card_x + 58, card_y + 39), "NATIVE 3D PREVIEW", fill=(226, 232, 240), font=get_font(16, bold=True), anchor="lm")

        # Paste Pre-rendered Info Card
        frame.paste(info_cards[seg_idx], (info_x, info_y), info_cards[seg_idx])

        # Bottom Progress Bar
        progress_w = int(width * (t / total_dur))
        draw.rectangle([(0, 1910), (progress_w, 1920)], fill=(56, 189, 248))

        pipe.stdin.write(frame.convert("RGB").tobytes())

        if (frame_idx + 1) % 200 == 0 or frame_idx == total_frames - 1:
            pct = int((frame_idx + 1) / total_frames * 100)
            elapsed = time.time() - start_wall_time
            fps_speed = (frame_idx + 1) / elapsed
            print(f"Shorts Progress: [{frame_idx + 1}/{total_frames}] {pct}% ({fps_speed:.1f} fps)")

    pipe.stdin.close()
    pipe.wait()
    print(f"[OK] Shorts Video Successfully Generated: {out_video}")
    return out_video

# ==============================================================================
# 2. Standard Product Video (16:9 Landscape 1920x1080)
# ==============================================================================
def render_standard():
    print("\n=======================================================")
    print("[*] Rendering Clean Standard Product Video (1920x1080 Landscape, 30 FPS)...")
    print("=======================================================")

    timeline_path = os.path.join(BASE_DIR, "timeline_standard.json")
    with open(timeline_path, "r", encoding="utf-8") as f:
        timeline = json.load(f)

    total_dur = timeline["total_duration"]
    segments = timeline["segments"]
    master_audio = timeline["master_audio"]
    out_video = os.path.join(OUTPUT_DIR, "ModelPeek_Product_16x9.mp4")

    width, height = 1920, 1080
    fps = 30
    total_frames = int(total_dur * fps)
    assets = load_and_cache_assets()

    # Pre-render base background
    bg_base = create_gradient_bg(width, height, c1=(10, 14, 22), c2=(17, 25, 40))
    bdraw = ImageDraw.Draw(bg_base)

    # Top Bar Header Strip
    bdraw.rectangle([(0, 0), (width, 76)], fill=(15, 23, 42, 240))
    bdraw.line([(0, 76), (width, 76)], fill=(30, 41, 59), width=2)

    # Brand Logo Pill Left
    draw_rounded_rect(bdraw, (36, 16, 86, 60), radius=12, fill=(2, 132, 199))
    bdraw.text((61, 38), "3D", fill=(255, 255, 255), font=get_font(20, bold=True), anchor="mm")
    bdraw.text((98, 38), "ModelPeek", fill=(255, 255, 255), font=get_font(26, bold=True), anchor="lm")
    bdraw.text((246, 38), "|   Native 3D CAD Preview for Windows", fill=(148, 163, 184), font=get_font(18, bold=False), anchor="lm")

    # Header tags right
    tag_x = width - 420
    draw_rounded_rect(bdraw, (tag_x, 18, tag_x + 384, 58), radius=10, fill=(30, 41, 59, 200), outline=(51, 65, 85), width=1)
    bdraw.text((tag_x + 192, 38), "⭐ Free & Open Source on GitHub", fill=(56, 189, 248), font=get_font(17, bold=True), anchor="mm")

    stage_w, stage_h = 1680, 820
    stage_x = (width - stage_w) // 2
    stage_y = 96

    stage_mask = Image.new("L", (stage_w, stage_h), 0)
    mdraw = ImageDraw.Draw(stage_mask)
    draw_rounded_rect(mdraw, (0, 0, stage_w, stage_h), radius=20, fill=255)

    lower_cards = {}
    header_chips = {}
    lower_w, lower_h = 1680, 110
    lower_x = (width - lower_w) // 2
    lower_y = 936

    for idx, seg in enumerate(segments):
        card_img = Image.new("RGBA", (lower_w, lower_h), (0, 0, 0, 0))
        cdraw = ImageDraw.Draw(card_img)
        draw_rounded_rect(cdraw, (0, 0, lower_w, lower_h), radius=16, fill=(15, 23, 42, 245), outline=(51, 65, 85, 220), width=1)
        cdraw.text((32, 36), seg.get("title", "Feature Overview"), fill=(255, 255, 255), font=get_font(30, bold=True), anchor="lm")
        cdraw.text((32, 78), seg["subtitle"], fill=(148, 163, 184), font=get_font(21, bold=False), anchor="lm")
        cdraw.text((lower_w - 32, 55), "github.com/MIke-Slim/ModelPeek", fill=(56, 189, 248), font=get_font(20, bold=True), anchor="rm")
        lower_cards[idx] = card_img

        chip_img = Image.new("RGBA", (500, 40), (0, 0, 0, 0))
        chdraw = ImageDraw.Draw(chip_img)
        chapter_str = f"CHAPTER 0{idx + 1} / 0{len(segments)} : {seg.get('title', '').upper()}"
        chdraw.text((250, 20), chapter_str, fill=(56, 189, 248), font=get_font(18, bold=True), anchor="mm")
        header_chips[idx] = chip_img

    # Launch FFmpeg
    cmd = [
        FFMPEG_EXE, "-y",
        "-f", "rawvideo",
        "-vcodec", "rawvideo",
        "-s", f"{width}x{height}",
        "-pix_fmt", "rgb24",
        "-r", str(fps),
        "-i", "-",
        "-i", master_audio,
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-crf", "20",
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "192k",
        "-shortest",
        out_video
    ]
    pipe = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    start_wall_time = time.time()
    for frame_idx in range(total_frames):
        t = frame_idx / float(fps)

        # Active segment
        cur_seg = segments[0]
        seg_idx = 0
        for i, s in enumerate(segments):
            if s["start"] <= t < s["end"]:
                cur_seg = s
                seg_idx = i
                break
            elif t >= s["end"]:
                cur_seg = s
                seg_idx = i

        seg_dur = max(0.1, cur_seg["end"] - cur_seg["start"])
        seg_progress = min(1.0, max(0.0, (t - cur_seg["start"]) / seg_dur))

        zoom = 1.0 + 0.04 * seg_progress
        pan_x = 0.02 * math.sin(seg_progress * math.pi)

        frame = bg_base.copy()

        # Paste Stage
        asset_name = cur_seg["asset"]
        if asset_name in assets:
            scaled_asset = crop_and_scale_asset(assets[asset_name], stage_w, stage_h, zoom=zoom, pan_x=pan_x)
            frame.paste(scaled_asset, (stage_x, stage_y), stage_mask)

        draw = ImageDraw.Draw(frame)
        draw_rounded_rect(draw, (stage_x, stage_y, stage_x + stage_w, stage_y + stage_h), radius=20, outline=(51, 65, 85), width=2)

        # Stage HUD Top Right
        hud_box_w = 340
        draw_rounded_rect(draw, (stage_x + stage_w - hud_box_w - 20, stage_y + 20, stage_x + stage_w - 20, stage_y + 60), radius=8, fill=(15, 23, 42, 220), outline=(51, 65, 85))
        draw.text((stage_x + stage_w - hud_box_w // 2 - 20, stage_y + 40), "WebGL 2.0 • 60 FPS • Sandboxed", fill=(148, 163, 184), font=get_font(15, bold=True), anchor="mm")

        # Paste Dynamic Header Chip
        frame.paste(header_chips[seg_idx], (width // 2 - 250, 18), header_chips[seg_idx])

        # Paste Lower Third Card
        frame.paste(lower_cards[seg_idx], (lower_x, lower_y), lower_cards[seg_idx])

        # Bottom Progress Bar
        progress_w = int(width * (t / total_dur))
        draw.rectangle([(0, 1072), (progress_w, 1080)], fill=(56, 189, 248))

        pipe.stdin.write(frame.convert("RGB").tobytes())

        if (frame_idx + 1) % 400 == 0 or frame_idx == total_frames - 1:
            pct = int((frame_idx + 1) / total_frames * 100)
            elapsed = time.time() - start_wall_time
            fps_speed = (frame_idx + 1) / elapsed
            print(f"Standard Progress: [{frame_idx + 1}/{total_frames}] {pct}% ({fps_speed:.1f} fps)")

    pipe.stdin.close()
    pipe.wait()
    print(f"[OK] Standard Product Video Successfully Generated: {out_video}")
    return out_video

def main():
    shorts_file = render_shorts()
    standard_file = render_standard()
    print("\n=======================================================")
    print("[SUCCESS] ALL CLEAN PRESENTATION VIDEOS GENERATED!")
    print(f"1. YouTube Shorts (9:16):         {shorts_file}")
    print(f"2. Standard Product Video (16:9): {standard_file}")
    print("=======================================================")

if __name__ == "__main__":
    main()
