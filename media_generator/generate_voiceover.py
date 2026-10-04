import os
import sys
import json
import asyncio
import subprocess
import edge_tts
import imageio_ffmpeg

VOICE = "en-US-ChristopherNeural"
RATE = "+5%"

SHORTS_SCRIPT = [
    {
        "id": "s0_hook",
        "text": "Still waiting for heavy CAD software just to check what's inside a file?",
        "subtitle": "Tired of heavy CAD load times?",
        "asset": "scene_1_iso_shaded.png",
    },
    {
        "id": "s1_intro",
        "text": "Meet ModelPeek — the native 3D preview extension for Windows Explorer.",
        "subtitle": "Meet ModelPeek: Native Windows 3D Preview",
        "asset": "scene_0_explorer_mockup.png",
    },
    {
        "id": "s2_thumbs",
        "text": "Instant 3D thumbnails for STEP, STL, and OBJ files directly in your folders.",
        "subtitle": "Instant 3D Thumbnails (STEP / STL / OBJ)",
        "asset": "scene_0_explorer_mockup.png",
    },
    {
        "id": "s3_preview",
        "text": "Select any model, and an interactive 3D viewport appears right in the preview pane.",
        "subtitle": "Interactive Right-Pane 3D Viewport",
        "asset": "scene_2_rotated_3d.png",
    },
    {
        "id": "s4_wireframe",
        "text": "Rotate in 3D, inspect geometry, switch to wireframe mode, and snap camera angles.",
        "subtitle": "Wireframe Mesh & Multi-Angle Orthographic Views",
        "asset": "scene_3_wireframe.png",
    },
    {
        "id": "s5_ortho",
        "text": "Check dimensions, vertex counts, and faces with zero delay.",
        "subtitle": "Real-time HUD Dimensions & Face Stats",
        "asset": "scene_4_top_ortho.png",
    },
    {
        "id": "s6_outro",
        "text": "Lightweight, portable, and blazingly fast. Try ModelPeek on GitHub today!",
        "subtitle": "Free & Open Source on GitHub",
        "asset": "scene_7_fit_overview.png",
    }
]

STANDARD_SCRIPT = [
    {
        "id": "std0_problem",
        "text": "If you work with 3D CAD models on Windows, you know the frustration. Opening SolidWorks, Fusion 360, or Blender just to inspect a single part takes minutes and eats gigabytes of memory.",
        "title": "The CAD Workflow Problem",
        "subtitle": "Opening bulky CAD tools just to check a part takes minutes.",
        "asset": "scene_1_iso_shaded.png",
    },
    {
        "id": "std1_intro",
        "text": "Welcome to ModelPeek. ModelPeek is an ultra-fast, lightweight 3D preview extension seamlessly integrated into the Windows Shell.",
        "title": "Introducing ModelPeek",
        "subtitle": "Native Windows Shell 3D Preview Extension",
        "asset": "scene_0_explorer_mockup.png",
    },
    {
        "id": "std2_thumbnails",
        "text": "First, ModelPeek automatically generates crystal-clear 3D thumbnails for STEP, STL, and OBJ formats, allowing you to identify parts at a glance without ever opening a CAD suite.",
        "title": "Native 3D Thumbnail Provider",
        "subtitle": "Instant 3D thumbnails directly in Windows Explorer folders.",
        "asset": "scene_0_explorer_mockup.png",
    },
    {
        "id": "std3_preview_pane",
        "text": "Second, click the Windows Explorer preview pane, and you immediately get a full 6-degree-of-freedom interactive 3D viewport powered by hardware-accelerated WebGL.",
        "title": "Interactive Preview Pane Handler",
        "subtitle": "Full 6-DOF Orbit Controls & Hardware-Accelerated WebGL",
        "asset": "scene_2_rotated_3d.png",
    },
    {
        "id": "std4_rendering",
        "text": "ModelPeek features studio lighting, ACES filmic tone mapping, shaded-with-edges rendering, and a dedicated wireframe mode for inspecting complex topology.",
        "title": "Professional CAD Visualizer",
        "subtitle": "Studio Lighting, ACES Tone Mapping & Wireframe Topology",
        "asset": "scene_3_wireframe.png",
    },
    {
        "id": "std5_views_stats",
        "text": "Switch instantly between Isometric, Top, Front, and Right orthographic views, or hit Fit to center your model. Plus, inspect bounding dimensions, vertex counts, and face counts directly on the HUD.",
        "title": "HUD Inspection & Camera Presets",
        "subtitle": "Top / Front / Right Views & Real-time Bounding Box Stats",
        "asset": "scene_4_top_ortho.png",
    },
    {
        "id": "std6_arch_safety",
        "text": "Best of all, ModelPeek is completely self-contained and portable. It runs locally inside a low-integrity preview sandbox, ensuring maximum security and zero background CPU overhead.",
        "title": "Zero Bloat & Sandbox Security",
        "subtitle": "Self-contained portable architecture with zero background services.",
        "asset": "scene_6_closeup.png",
    },
    {
        "id": "std7_conclusion",
        "text": "Upgrade your Windows workflow today with ModelPeek. Free, open source, and available right now on GitHub.",
        "title": "Get ModelPeek Now",
        "subtitle": "100% Free & Open Source on GitHub",
        "asset": "scene_7_fit_overview.png",
    }
]

def get_audio_duration(file_path):
    ffmpeg_exe = imageio_ffmpeg.get_ffmpeg_exe()
    cmd = [ffmpeg_exe, "-i", file_path]
    res = subprocess.run(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, text=True, errors="ignore")
    import re
    m = re.search(r"Duration:\s*(\d+):(\d+):(\d+\.\d+)", res.stderr)
    if m:
        hours, mins, secs = m.groups()
        return int(hours) * 3600 + int(mins) * 60 + float(secs)
    return 3.0

async def generate_speech_segment(text, out_file):
    communicate = edge_tts.Communicate(text, VOICE, rate=RATE)
    await communicate.save(out_file)

async def build_timeline(script_list, category_name, base_dir):
    audio_dir = os.path.join(base_dir, "audio", category_name)
    os.makedirs(audio_dir, exist_ok=True)
    
    current_time = 0.5
    timeline = []
    segment_files = []

    print(f"\n--- Generating Voiceovers for {category_name} ---")
    for i, item in enumerate(script_list):
        seg_audio = os.path.join(audio_dir, f"{item['id']}.mp3")
        print(f"[{i+1}/{len(script_list)}] Generating: {item['id']} ...")
        await generate_speech_segment(item["text"], seg_audio)
        dur = get_audio_duration(seg_audio)
        
        start_t = current_time
        end_t = current_time + dur + 0.35
        
        timeline_entry = {
            "id": item["id"],
            "text": item["text"],
            "subtitle": item.get("subtitle", item["text"]),
            "title": item.get("title", ""),
            "asset": item["asset"],
            "start": round(start_t, 2),
            "end": round(end_t, 2),
            "duration": round(dur, 2),
            "audio_file": seg_audio
        }
        timeline.append(timeline_entry)
        segment_files.append((seg_audio, dur))
        current_time = end_t

    total_duration = round(current_time + 1.0, 2)
    print(f"Total {category_name} duration: {total_duration}s")

    concat_txt = os.path.join(audio_dir, "concat.txt")
    with open(concat_txt, "w", encoding="utf-8") as f:
        for seg_audio, _ in segment_files:
            p = seg_audio.replace("\\", "/")
            f.write(f"file '{p}'\n")

    master_audio = os.path.join(audio_dir, f"master_{category_name}.mp3")
    ffmpeg_exe = imageio_ffmpeg.get_ffmpeg_exe()
    subprocess.run([
        ffmpeg_exe, "-y", "-f", "concat", "-safe", "0", "-i", concat_txt,
        "-c:a", "libmp3lame", "-q:a", "2", master_audio
    ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    timeline_meta = {
        "category": category_name,
        "total_duration": total_duration,
        "master_audio": master_audio,
        "segments": timeline
    }
    json_path = os.path.join(base_dir, f"timeline_{category_name}.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(timeline_meta, f, indent=2, ensure_ascii=False)
    print(f"Saved timeline metadata to: {json_path}")
    return timeline_meta

async def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    await build_timeline(SHORTS_SCRIPT, "shorts", base_dir)
    await build_timeline(STANDARD_SCRIPT, "standard", base_dir)
    print("\nAll clean English voiceovers and timelines generated successfully!")

if __name__ == "__main__":
    asyncio.run(main())
