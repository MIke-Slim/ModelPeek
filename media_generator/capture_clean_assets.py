import os
import sys
import time
import threading
from http.server import SimpleHTTPRequestHandler, HTTPServer
from playwright.sync_api import sync_playwright
from PIL import Image, ImageDraw, ImageFont

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ASSETS_DIR = os.path.join(BASE_DIR, "assets")
os.makedirs(ASSETS_DIR, exist_ok=True)

ROOT_DIR = os.path.abspath(os.path.join(BASE_DIR, ".."))

def start_local_server(port, root_dir):
    class QuietHandler(SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=root_dir, **kwargs)
        def log_message(self, format, *args):
            pass

    httpd = HTTPServer(('127.0.0.1', port), QuietHandler)
    server_thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    server_thread.start()
    return httpd

def capture_viewer_scenes():
    port = 8877
    httpd = start_local_server(port, ROOT_DIR)
    print(f"[*] Local WebGL server started on port {port}")

    viewer_url = f"http://127.0.0.1:{port}/dist/viewer/index.html?file=/sample_models/test_flange.stl"
    print(f"[*] Loading clean viewer at: {viewer_url}")

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page(viewport={"width": 1920, "height": 1080})
        page.goto(viewer_url)

        # Wait for model to load and render
        time.sleep(3)
        page.wait_for_selector("#info-panel", state="visible")
        time.sleep(1)

        # 1. Main Shaded + Edges view (Default Iso)
        path1 = os.path.join(ASSETS_DIR, "scene_1_iso_shaded.png")
        page.screenshot(path=path1)
        print(f"[+] Captured: {os.path.basename(path1)}")

        # 2. Smoothly rotated 3D perspective
        page.mouse.move(960, 540)
        page.mouse.down()
        page.mouse.move(1220, 410, steps=25)
        page.mouse.up()
        time.sleep(0.6)
        path2 = os.path.join(ASSETS_DIR, "scene_2_rotated_3d.png")
        page.screenshot(path=path2)
        print(f"[+] Captured: {os.path.basename(path2)}")

        # 3. Wireframe Topology Mode
        page.click('button[data-render-mode="wireframe"]')
        time.sleep(0.8)
        path3 = os.path.join(ASSETS_DIR, "scene_3_wireframe.png")
        page.screenshot(path=path3)
        print(f"[+] Captured: {os.path.basename(path3)}")

        # 4. Top Orthographic View
        page.click('button[data-render-mode="shaded"]')
        page.click('button[data-view="top"]')
        time.sleep(0.8)
        path4 = os.path.join(ASSETS_DIR, "scene_4_top_ortho.png")
        page.screenshot(path=path4)
        print(f"[+] Captured: {os.path.basename(path4)}")

        # 5. Front View with Shaded Edges
        page.click('button[data-render-mode="shaded_edges"]')
        page.click('button[data-view="front"]')
        time.sleep(0.8)
        path5 = os.path.join(ASSETS_DIR, "scene_5_front_ortho.png")
        page.screenshot(path=path5)
        print(f"[+] Captured: {os.path.basename(path5)}")

        # 6. Zoomed Detail Inspection
        page.click('button[data-view="iso"]')
        time.sleep(0.5)
        page.mouse.wheel(0, -380)
        time.sleep(0.6)
        path6 = os.path.join(ASSETS_DIR, "scene_6_closeup.png")
        page.screenshot(path=path6)
        print(f"[+] Captured: {os.path.basename(path6)}")

        # 7. Fit Center View
        page.click('#btn-fit')
        time.sleep(0.8)
        path7 = os.path.join(ASSETS_DIR, "scene_7_fit_overview.png")
        page.screenshot(path=path7)
        print(f"[+] Captured: {os.path.basename(path7)}")

        browser.close()
        httpd.shutdown()

    print("[*] All 3D CAD scenes captured cleanly.")

def generate_fluent_explorer_mockup():
    """Generates a pristine, modern Windows 11 style Explorer window with ModelPeek active."""
    print("[*] Generating clean Windows 11 Explorer mockup...")
    w, h = 1920, 1080
    img = Image.new("RGBA", (w, h), (15, 23, 42, 255))
    draw = ImageDraw.Draw(img)

    # Windows 11 Acrylic Window Container
    win_x, win_y, win_w, win_h = 140, 80, 1640, 920
    draw.rounded_rectangle([win_x, win_y, win_x + win_w, win_y + win_h], radius=16, fill=(30, 41, 59, 255), outline=(51, 65, 85, 255), width=2)

    # Title Bar
    draw.rounded_rectangle([win_x, win_y, win_x + win_w, win_y + 48], radius=16, fill=(15, 23, 42, 255))
    draw.rectangle([win_x, win_y + 32, win_x + win_w, win_y + 48], fill=(15, 23, 42, 255))
    draw.line([(win_x, win_y + 48), (win_x + win_w, win_y + 48)], fill=(51, 65, 85, 255), width=1)

    font_bold = ImageFont.truetype("C:/Windows/Fonts/segoeuib.ttf", 16)
    font_reg = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 15)
    font_sm = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 13)

    # Window Controls (Minimize, Maximize, Close)
    draw.text((win_x + 20, win_y + 14), "📁  CAD Projects - Windows Explorer", fill=(226, 232, 240), font=font_bold)
    for i, color in enumerate([(100, 116, 139), (100, 116, 139), (239, 68, 68)]):
        cx = win_x + win_w - 70 + i * 22
        draw.ellipse([cx, win_y + 18, cx + 12, win_y + 30], fill=color)

    # Toolbar Strip
    tb_y = win_y + 49
    draw.rectangle([win_x, tb_y, win_x + win_w, tb_y + 54], fill=(22, 30, 46, 255))
    draw.line([(win_x, tb_y + 54), (win_x + win_w, tb_y + 54)], fill=(51, 65, 85, 255), width=1)
    
    # Breadcrumb bar
    draw.rounded_rectangle([win_x + 20, tb_y + 10, win_x + 680, tb_y + 44], radius=8, fill=(30, 41, 59, 255), outline=(51, 65, 85, 255))
    draw.text((win_x + 36, tb_y + 18), "This PC  >  Engineering  >  Mechanical_CAD", fill=(148, 163, 184), font=font_reg)

    # Search Bar
    draw.rounded_rectangle([win_x + 700, tb_y + 10, win_x + 980, tb_y + 44], radius=8, fill=(30, 41, 59, 255), outline=(51, 65, 85, 255))
    draw.text((win_x + 716, tb_y + 18), "🔍 Search Mechanical_CAD", fill=(100, 116, 139), font=font_reg)

    # Preview Pane Button Active Pill
    draw.rounded_rectangle([win_x + win_w - 240, tb_y + 10, win_x + win_w - 20, tb_y + 44], radius=8, fill=(2, 132, 199, 220))
    draw.text((win_x + win_w - 224, tb_y + 18), "⚡ Preview Pane [ACTIVE]", fill=(255, 255, 255), font=font_bold)

    # Main Area Layout: Left Navigation (w=260), File List (w=700), Right 3D Viewport (w=680)
    nav_w = 260
    list_w = 660
    pane_w = win_w - nav_w - list_w
    main_top = tb_y + 55
    main_h = win_y + win_h - main_top

    # 1. Left Sidebar
    draw.rectangle([win_x, main_top, win_x + nav_w, win_y + win_h - 16], fill=(18, 26, 40, 255))
    draw.line([(win_x + nav_w, main_top), (win_x + nav_w, win_y + win_h)], fill=(51, 65, 85, 255), width=1)
    
    nav_items = ["⭐ Quick Access", "📁 3D CAD Library", "  └─ Assemblies", "  └─ STEP Models", "  └─ STL Meshes", "💾 Local Storage (C:)", "☁ OneDrive CAD"]
    for i, itm in enumerate(nav_items):
        y_pos = main_top + 25 + i * 36
        color = (56, 189, 248) if "STEP" in itm else (148, 163, 184)
        draw.text((win_x + 24, y_pos), itm, fill=color, font=font_reg)

    # 2. File List
    file_x = win_x + nav_w
    files = [
        {"name": "Industrial_Flange.step", "size": "24.5 MB", "type": "STEP 3D Model", "active": True},
        {"name": "Turbine_Blades.step", "size": "48.2 MB", "type": "STEP 3D Model", "active": False},
        {"name": "Robotic_Arm_Joint.stl", "size": "12.8 MB", "type": "STL Mesh Geometry", "active": False},
        {"name": "Engine_Mount_Bracket.obj", "size": "16.1 MB", "type": "Wavefront OBJ Model", "active": False},
        {"name": "Gear_Housing_Enclosure.3mf", "size": "9.4 MB", "type": "3D Manufacturing Format", "active": False},
        {"name": "Hydraulic_Valve_V3.step", "size": "31.0 MB", "type": "STEP 3D Model", "active": False},
        {"name": "Servo_Motor_Coupling.stl", "size": "6.2 MB", "type": "STL Mesh Geometry", "active": False},
    ]

    for i, f in enumerate(files):
        row_y = main_top + 15 + i * 72
        if f["active"]:
            # Highlight active file
            draw.rounded_rectangle([file_x + 10, row_y, file_x + list_w - 10, row_y + 64], radius=10, fill=(30, 58, 138, 180), outline=(56, 189, 248), width=1)
            name_color = (255, 255, 255)
        else:
            name_color = (226, 232, 240)

        # 3D Thumbnail icon box
        draw.rounded_rectangle([file_x + 24, row_y + 10, file_x + 68, row_y + 54], radius=6, fill=(15, 23, 42, 255), outline=(51, 65, 85))
        draw.text((file_x + 36, row_y + 22), "3D", fill=(56, 189, 248), font=font_bold)

        draw.text((file_x + 82, row_y + 14), f["name"], fill=name_color, font=font_bold)
        draw.text((file_x + 82, row_y + 38), f"{f['type']}  •  {f['size']}", fill=(148, 163, 184), font=font_sm)

    # 3. Right ModelPeek 3D Preview Pane
    pane_x = file_x + list_w
    draw.line([(pane_x, main_top), (pane_x, win_y + win_h)], fill=(51, 65, 85, 255), width=1)
    
    # Paste clean 3D render inside right pane
    iso_asset_path = os.path.join(ASSETS_DIR, "scene_2_rotated_3d.png")
    if os.path.exists(iso_asset_path):
        preview_img = Image.open(iso_asset_path).convert("RGBA")
        pw = pane_w - 24
        ph = main_h - 36
        scaled_p = preview_img.resize((pw, ph), Image.Resampling.LANCZOS)
        img.paste(scaled_p, (pane_x + 12, main_top + 18))

    # Frame overlay for 3D Viewport
    draw.rounded_rectangle([pane_x + 12, main_top + 18, pane_x + 12 + pane_w - 24, main_top + 18 + main_h - 36], radius=12, outline=(56, 189, 248), width=2)
    
    # ModelPeek Watermark badge top left of pane
    draw.rounded_rectangle([pane_x + 24, main_top + 30, pane_x + 260, main_top + 68], radius=8, fill=(15, 23, 42, 230), outline=(51, 65, 85))
    draw.text((pane_x + 38, main_top + 42), "ModelPeek 3D Preview", fill=(56, 189, 248), font=font_bold)

    out_mockup = os.path.join(ASSETS_DIR, "scene_0_explorer_mockup.png")
    img.save(out_mockup, "PNG")
    print(f"[+] Generated clean mock Explorer preview: {out_mockup}")

if __name__ == "__main__":
    capture_viewer_scenes()
    generate_fluent_explorer_mockup()
