import os
import sys
import math
import json
import struct
import zipfile

# 1. FreeCAD integration for precision CAD solids
HAS_FREECAD = False
try:
    import FreeCAD
    import Part
    HAS_FREECAD = True
except ImportError:
    pass

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "sample_models2"))
os.makedirs(OUT_DIR, exist_ok=True)

print(f"Generating sample_models2 into: {OUT_DIR}")
print(f"FreeCAD Part Engine available: {HAS_FREECAD}")

# =============================================================================
# Helper: Export FreeCAD Shapes
# =============================================================================
def export_cad(shape, base_name, ext_list):
    for ext in ext_list:
        path = os.path.join(OUT_DIR, f"{base_name}.{ext}")
        if ext in ("step", "stp"):
            shape.exportStep(path)
        elif ext in ("iges", "igs"):
            shape.exportIges(path)
        elif ext in ("brep", "brp"):
            shape.exportBrep(path)
        elif ext == "stl":
            shape.exportStl(path)
        print(f"  [CAD] Created: {os.path.basename(path)}")

# =============================================================================
# 1. Industrial Planetary Gear (STEP)
# =============================================================================
def make_planetary_gear():
    if not HAS_FREECAD: return
    # Cylinder blank
    gear = Part.makeCylinder(28.0, 15.0)
    # Bore
    bore = Part.makeCylinder(8.0, 16.0)
    bore.translate(FreeCAD.Vector(0, 0, -0.5))
    gear = gear.cut(bore)
    # Keyway
    kw = Part.makeBox(3.0, 4.0, 16.0)
    kw.translate(FreeCAD.Vector(-1.5, 6.0, -0.5))
    gear = gear.cut(kw)
    # 18 Teeth cutouts
    teeth_num = 18
    for i in range(teeth_num):
        angle = (2 * math.pi / teeth_num) * i
        # Small cutting wedge
        cutter = Part.makeCylinder(3.2, 16.0)
        cx = 28.0 * math.cos(angle)
        cy = 28.0 * math.sin(angle)
        cutter.translate(FreeCAD.Vector(cx, cy, -0.5))
        gear = gear.cut(cutter)
    # 4 Weight-reduction holes
    for i in range(4):
        ang = (math.pi / 2) * i + math.pi / 4
        hx = 16.0 * math.cos(ang)
        hy = 16.0 * math.sin(ang)
        hole = Part.makeCylinder(4.5, 16.0)
        hole.translate(FreeCAD.Vector(hx, hy, -0.5))
        gear = gear.cut(hole)
    export_cad(gear, "industrial_planetary_gear", ["step"])

# =============================================================================
# 2. Pneumatic Valve Body (STP)
# =============================================================================
def make_valve_body():
    if not HAS_FREECAD: return
    body = Part.makeBox(60.0, 50.0, 35.0)
    # Main central valve bore
    bore_x = Part.makeCylinder(12.0, 70.0)
    bore_x.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,1,0), 90)
    bore_x.translate(FreeCAD.Vector(-5.0, 25.0, 17.5))
    body = body.cut(bore_x)
    # Vertical inlet / outlet ports
    port1 = Part.makeCylinder(9.0, 25.0)
    port1.translate(FreeCAD.Vector(18.0, 25.0, 17.5))
    body = body.cut(port1)
    port2 = Part.makeCylinder(9.0, 25.0)
    port2.translate(FreeCAD.Vector(42.0, 25.0, 17.5))
    body = body.cut(port2)
    # 4 Mounting bolt holes
    for px, py in [(6, 6), (54, 6), (6, 44), (54, 44)]:
        hole = Part.makeCylinder(3.3, 40.0)
        hole.translate(FreeCAD.Vector(px, py, -2.0))
        body = body.cut(hole)
    export_cad(body, "pneumatic_valve_body", ["stp"])

# =============================================================================
# 3. Aerodynamic Wing / Turbine Profile (IGES)
# =============================================================================
def make_wing_profile():
    if not HAS_FREECAD: return
    # Aerodynamic blade sections using circular/elliptical wires
    w1 = Part.Wire([Part.makeCircle(16.0, FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1))])
    w2 = Part.Wire([Part.makeCircle(11.0, FreeCAD.Vector(4, 0, 35), FreeCAD.Vector(0, 0, 1))])
    w3 = Part.Wire([Part.makeCircle(6.0, FreeCAD.Vector(8, 0, 75), FreeCAD.Vector(0, 0, 1))])
    loft = Part.makeLoft([w1, w2, w3], True, True)
    export_cad(loft, "aerodynamic_wing_profile", ["iges"])

# =============================================================================
# 4. Supersonic Rocket Nozzle (IGS)
# =============================================================================
def make_rocket_nozzle():
    if not HAS_FREECAD: return
    # Convergent-divergent cone revolution
    c1 = Part.makeCone(25.0, 10.0, 30.0) # convergent
    c2 = Part.makeCone(10.0, 32.0, 50.0) # divergent
    c2.translate(FreeCAD.Vector(0, 0, 30.0))
    outer = c1.fuse(c2)
    # Inner bore
    b1 = Part.makeCone(22.0, 8.0, 31.0)
    b1.translate(FreeCAD.Vector(0, 0, -0.5))
    b2 = Part.makeCone(8.0, 29.5, 51.0)
    b2.translate(FreeCAD.Vector(0, 0, 30.0))
    inner = b1.fuse(b2)
    nozzle = outer.cut(inner)
    export_cad(nozzle, "supersonic_nozzle", ["igs"])

# =============================================================================
# 5. Robotic Gripper Finger (BREP & BRP)
# =============================================================================
def make_gripper_and_bracket():
    if not HAS_FREECAD: return
    # Gripper
    finger = Part.makeBox(70.0, 18.0, 12.0)
    # Tapered tip cut
    cut_box = Part.makeBox(25.0, 25.0, 20.0)
    cut_box.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), 30)
    cut_box.translate(FreeCAD.Vector(55.0, 12.0, -2.0))
    finger = finger.cut(cut_box)
    # Hinge hole
    hinge = Part.makeCylinder(3.5, 20.0)
    hinge.translate(FreeCAD.Vector(10.0, 9.0, -2.0))
    finger = finger.cut(hinge)
    # Grip ridges
    for i in range(5):
        ridge = Part.makeCylinder(1.2, 20.0)
        ridge.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(1,0,0), 90)
        ridge.translate(FreeCAD.Vector(42.0 + i*5.0, 18.0, 6.0))
        finger = finger.cut(ridge)
    export_cad(finger, "robotic_gripper_finger", ["brep"])

    # Motor Mounting Bracket
    base = Part.makeBox(60.0, 60.0, 8.0)
    # NEMA 23 center pilot hole (dia 38.1mm)
    pilot = Part.makeCylinder(19.05, 12.0)
    pilot.translate(FreeCAD.Vector(30.0, 30.0, -2.0))
    base = base.cut(pilot)
    # 4 Bolt Slots (47.14mm bolt circle)
    offset = 47.14 / 2.0
    for dx, dy in [(-offset, -offset), (offset, -offset), (-offset, offset), (offset, offset)]:
        bhole = Part.makeCylinder(2.6, 12.0)
        bhole.translate(FreeCAD.Vector(30.0 + dx, 30.0 + dy, -2.0))
        base = base.cut(bhole)
    # Upright flange
    flange = Part.makeBox(8.0, 60.0, 45.0)
    bracket = base.fuse(flange)
    export_cad(bracket, "motor_mounting_bracket", ["brp"])

# =============================================================================
# 6. High-Density Extruded Heatsink & Gyroscope Ring (STL)
# =============================================================================
def make_heatsink_and_gyro():
    if not HAS_FREECAD: return
    # Heatsink base plate
    hsb = Part.makeBox(65.0, 65.0, 6.0)
    # 10 vertical fins
    for i in range(10):
        fin = Part.makeBox(65.0, 2.0, 28.0)
        fin.translate(FreeCAD.Vector(0, 3.0 + i * 6.2, 6.0))
        hsb = hsb.fuse(fin)
    # Corner mounting holes
    for cx, cy in [(6,6), (59,6), (6,59), (59,59)]:
        h = Part.makeCylinder(2.2, 10.0)
        h.translate(FreeCAD.Vector(cx, cy, -2.0))
        hsb = hsb.cut(h)
    export_cad(hsb, "heat_sink_extrusion", ["stl"])

    # Spherical Gyroscope Ring
    ring_out = Part.makeCylinder(35.0, 10.0)
    ring_in = Part.makeCylinder(28.0, 12.0)
    ring_in.translate(FreeCAD.Vector(0, 0, -1.0))
    gyro = ring_out.cut(ring_in)
    # Pivot Pins
    p1 = Part.makeCylinder(3.0, 12.0)
    p1.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,1,0), 90)
    p1.translate(FreeCAD.Vector(25.0, 0, 5.0))
    p2 = Part.makeCylinder(3.0, 12.0)
    p2.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,1,0), -90)
    p2.translate(FreeCAD.Vector(-25.0, 0, 5.0))
    gyro = gyro.fuse(p1).fuse(p2)
    export_cad(gyro, "spherical_gyroscope_ring", ["stl"])

# =============================================================================
# 7. Quadcopter Frame (OBJ)
# =============================================================================
def make_quadcopter_obj():
    path = os.path.join(OUT_DIR, "quadcopter_frame.obj")
    verts = []
    faces = []

    def add_box(x0, y0, z0, dx, dy, dz):
        base_idx = len(verts) + 1
        vs = [
            (x0, y0, z0), (x0+dx, y0, z0), (x0+dx, y0+dy, z0), (x0, y0+dy, z0),
            (x0, y0, z0+dz), (x0+dx, y0, z0+dz), (x0+dx, y0+dy, z0+dz), (x0, y0+dy, z0+dz)
        ]
        verts.extend(vs)
        fs = [
            (1, 2, 3, 4), (5, 8, 7, 6),
            (1, 5, 6, 2), (2, 6, 7, 3),
            (3, 7, 8, 4), (4, 8, 5, 1)
        ]
        for f in fs:
            faces.append((f[0]+base_idx-1, f[1]+base_idx-1, f[2]+base_idx-1, f[3]+base_idx-1))

    # Center fuselage hub
    add_box(-30, -30, -5, 60, 60, 10)
    # 4 Carbon fiber arms
    arm_len = 110
    add_box(20, -6, -3, arm_len, 12, 6)
    add_box(-20-arm_len, -6, -3, arm_len, 12, 6)
    add_box(-6, 20, -3, 12, arm_len, 6)
    add_box(-6, -20-arm_len, -3, 12, arm_len, 6)
    # 4 Motor Mount Discs
    for mx, my in [(130, 0), (-130, 0), (0, 130), (0, -130)]:
        add_box(mx-15, my-15, -4, 30, 30, 8)

    with open(path, "w", encoding="utf-8") as f:
        f.write("# ModelPeek Quadcopter Drone Frame OBJ\n")
        f.write("o QuadcopterFrame\n")
        for v in verts:
            f.write(f"v {v[0]:.2f} {v[1]:.2f} {v[2]:.2f}\n")
        for q in faces:
            f.write(f"f {q[0]} {q[1]} {q[2]} {q[3]}\n")
    print("  [OBJ] Created: quadcopter_frame.obj")

# =============================================================================
# 8. Parametric Architecture Pavilion (GLTF)
# =============================================================================
def make_pavilion_gltf():
    path = os.path.join(OUT_DIR, "hexagonal_architecture_pavilion.gltf")
    # Generates a hexagonal geodesic dome / canopy
    verts = []
    indices = []
    # Base ring (6 pillars)
    r = 50.0
    h_pillar = 35.0
    top_y = 65.0
    verts.append((0.0, top_y, 0.0)) # Center apex (idx 0)
    for i in range(6):
        ang = (math.pi / 3) * i
        bx = r * math.cos(ang)
        bz = r * math.sin(ang)
        # Ground pillar base
        verts.append((bx, 0.0, bz))
        # Canopy corner
        verts.append((bx * 1.15, h_pillar, bz * 1.15))

    # Canopy triangles
    for i in range(6):
        p1 = 2 + i * 2      # corner
        p2 = 2 + ((i + 1) % 6) * 2
        indices.extend([0, p1, p2])
        # Pillars
        pb = 1 + i * 2      # base
        indices.extend([pb, p1, (pb % 12) + 1])

    # Convert to binary byte buffers
    v_bytes = bytearray()
    for v in verts:
        v_bytes += struct.pack("<fff", v[0], v[1], v[2])
    idx_bytes = bytearray()
    for idx in indices:
        idx_bytes += struct.pack("<H", idx)

    import base64
    v_b64 = base64.b64encode(v_bytes).decode("ascii")
    idx_b64 = base64.b64encode(idx_bytes).decode("ascii")

    gltf = {
        "asset": {"version": "2.0", "generator": "ModelPeek TestSuite"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "HexagonalPavilion"}],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 0},
                "indices": 1,
                "mode": 4
            }]
        }],
        "accessors": [
            {
                "bufferView": 0,
                "byteOffset": 0,
                "componentType": 5126,
                "count": len(verts),
                "type": "VEC3",
                "max": [r*1.2, top_y, r*1.2],
                "min": [-r*1.2, 0.0, -r*1.2]
            },
            {
                "bufferView": 1,
                "byteOffset": 0,
                "componentType": 5123,
                "count": len(indices),
                "type": "SCALAR",
                "max": [len(verts) - 1],
                "min": [0]
            }
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(v_bytes), "target": 34962},
            {"buffer": 1, "byteOffset": 0, "byteLength": len(idx_bytes), "target": 34963}
        ],
        "buffers": [
            {"uri": f"data:application/octet-stream;base64,{v_b64}", "byteLength": len(v_bytes)},
            {"uri": f"data:application/octet-stream;base64,{idx_b64}", "byteLength": len(idx_bytes)}
        ]
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(gltf, f, indent=2)
    print("  [GLTF] Created: hexagonal_architecture_pavilion.gltf")

# =============================================================================
# 9. Low-Poly Cyber Sports Car (GLB)
# =============================================================================
def make_cyber_car_glb():
    path = os.path.join(OUT_DIR, "lowpoly_cyber_car.glb")
    # Low-poly car body wedge + 4 wheels
    verts = [
        # Chassis bottom
        (-15, 4, -40), (15, 4, -40), (15, 4, 40), (-15, 4, 40),
        # Hood & Trunk belt
        (-14, 12, -38), (14, 12, -38), (14, 12, 38), (-14, 12, 38),
        # Roof
        (-10, 22, -10), (10, 22, -10), (10, 22, 15), (-10, 22, 15),
    ]
    indices = [
        # Bottom
        0, 1, 2,  0, 2, 3,
        # Front bumper
        0, 1, 5,  0, 5, 4,
        # Hood
        4, 5, 9,  4, 9, 8,
        # Windshield
        8, 9, 10, 8, 10, 11,
        # Rear window
        11, 10, 6, 11, 6, 7,
        # Rear bumper
        3, 7, 6,  3, 6, 2,
        # Sides
        0, 4, 7,  0, 7, 3,
        1, 2, 6,  1, 6, 5,
        4, 8, 11, 4, 11, 7,
        5, 6, 10, 5, 10, 9
    ]
    v_bytes = bytearray()
    for v in verts:
        v_bytes += struct.pack("<fff", v[0], v[1], v[2])
    idx_bytes = bytearray()
    for idx in indices:
        idx_bytes += struct.pack("<H", idx)

    # Pad buffers to 4-byte boundaries
    while len(v_bytes) % 4 != 0: v_bytes.append(0)
    idx_offset = len(v_bytes)
    while len(idx_bytes) % 4 != 0: idx_bytes.append(0)
    bin_chunk = v_bytes + idx_bytes

    gltf_json = {
        "asset": {"version": "2.0", "generator": "ModelPeek TestSuite"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "CyberCarBody"}],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 0},
                "indices": 1,
                "mode": 4
            }]
        }],
        "accessors": [
            {"bufferView": 0, "byteOffset": 0, "componentType": 5126, "count": len(verts), "type": "VEC3", "max": [15, 22, 40], "min": [-15, 4, -40]},
            {"bufferView": 1, "byteOffset": 0, "componentType": 5123, "count": len(indices), "type": "SCALAR", "max": [len(verts)-1], "min": [0]}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(v_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": idx_offset, "byteLength": len(idx_bytes), "target": 34963}
        ],
        "buffers": [{"byteLength": len(bin_chunk)}]
    }
    json_bytes = json.dumps(gltf_json).encode("utf-8")
    while len(json_bytes) % 4 != 0: json_bytes += b" "

    # GLB Header
    total_len = 12 + 8 + len(json_bytes) + 8 + len(bin_chunk)
    glb = bytearray()
    glb += struct.pack("<4sII", b"glTF", 2, total_len)
    glb += struct.pack("<II", len(json_bytes), 0x4E4F534A) # JSON
    glb += json_bytes
    glb += struct.pack("<II", len(bin_chunk), 0x004E4942)  # BIN
    glb += bin_chunk

    with open(path, "wb") as f:
        f.write(glb)
    print("  [GLB] Created: lowpoly_cyber_car.glb")

# =============================================================================
# 10. Medical Orthotic Insole (3MF)
# =============================================================================
def make_insole_3mf():
    path = os.path.join(OUT_DIR, "medical_orthotic_insole.3mf")
    # Generates a valid 3MF archive with an ergonomic footbed shape
    model_xml = """<?xml version="1.0" encoding="UTF-8"?>
<model unit="millimeter" xml:lang="en-US" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">
  <metadata name="Title">Orthotic Footbed</metadata>
  <metadata name="Designer">ModelPeek BioMed</metadata>
  <resources>
    <object id="1" type="model">
      <mesh>
        <vertices>
          <vertex x="-35.0" y="-90.0" z="2.0" />
          <vertex x="35.0" y="-90.0" z="2.0" />
          <vertex x="40.0" y="20.0" z="5.0" />
          <vertex x="-30.0" y="20.0" z="18.0" />
          <vertex x="32.0" y="110.0" z="2.0" />
          <vertex x="-25.0" y="110.0" z="2.0" />
          <vertex x="0.0" y="-95.0" z="8.0" />
        </vertices>
        <triangles>
          <triangle v1="0" v2="1" v3="6" />
          <triangle v1="0" v2="6" v3="3" />
          <triangle v1="1" v2="2" v3="6" />
          <triangle v1="2" v2="3" v3="6" />
          <triangle v1="2" v2="4" v3="3" />
          <triangle v1="4" v2="5" v3="3" />
        </triangles>
      </mesh>
    </object>
  </resources>
  <build>
    <item objectid="1" />
  </build>
</model>"""

    content_types = """<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml" />
  <Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml" />
</Types>"""

    rels = """<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel" />
</Relationships>"""

    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", content_types)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", model_xml)
    print("  [3MF] Created: medical_orthotic_insole.3mf")

# =============================================================================
# 11. Scanned Ancient Amphora (PLY)
# =============================================================================
def make_amphora_ply():
    path = os.path.join(OUT_DIR, "scan_ancient_amphora.ply")
    # Torus-fluted vase of revolution with vertex colors
    verts = []
    faces = []
    rings = 36
    segs = 32
    for r in range(rings):
        t = r / (rings - 1)
        z = t * 120.0
        # Radius profile: narrow base, wide belly, narrow neck, flared lip
        radius = 12.0 + 32.0 * math.sin(t * math.pi) - 10.0 * math.sin(t * 3 * math.pi)
        red = int(180 + 50 * math.sin(t * math.pi))
        green = int(110 + 30 * math.cos(t * math.pi))
        blue = 70
        for s in range(segs):
            theta = (2 * math.pi / segs) * s
            x = radius * math.cos(theta)
            y = radius * math.sin(theta)
            verts.append((x, y, z, red, green, blue))

    for r in range(rings - 1):
        for s in range(segs):
            s_next = (s + 1) % segs
            i0 = r * segs + s
            i1 = r * segs + s_next
            i2 = (r + 1) * segs + s_next
            i3 = (r + 1) * segs + s
            faces.append((i0, i1, i2))
            faces.append((i0, i2, i3))

    with open(path, "w", encoding="utf-8") as f:
        f.write("ply\nformat ascii 1.0\n")
        f.write(f"element vertex {len(verts)}\n")
        f.write("property float x\nproperty float y\nproperty float z\n")
        f.write("property uchar red\nproperty uchar green\nproperty uchar blue\n")
        f.write(f"element face {len(faces)}\n")
        f.write("property list uchar int vertex_indices\n")
        f.write("end_header\n")
        for v in verts:
            f.write(f"{v[0]:.2f} {v[1]:.2f} {v[2]:.2f} {v[3]} {v[4]} {v[5]}\n")
        for fc in faces:
            f.write(f"3 {fc[0]} {fc[1]} {fc[2]}\n")
    print("  [PLY] Created: scan_ancient_amphora.ply")

# =============================================================================
# 12. Spiral Helix Tower (GCODE)
# =============================================================================
def make_spiral_gcode():
    path = os.path.join(OUT_DIR, "spiral_helix_tower.gcode")
    with open(path, "w", encoding="utf-8") as f:
        f.write("; ModelPeek Continuous Spiral Vase G-Code\n")
        f.write("M104 S210 ; Set nozzle temp\n")
        f.write("M140 S60  ; Set bed temp\n")
        f.write("G28       ; Home all axes\n")
        f.write("G92 E0    ; Reset extruder\n")
        f.write("G1 Z0.2 F3000\n")
        
        # Spiral upward for 300 steps
        steps = 400
        z = 0.2
        e = 0.0
        center_x, center_y = 110.0, 110.0
        radius = 45.0
        for i in range(steps):
            theta = (2 * math.pi / 24) * i
            # Twisting star radius
            r = radius + 8.0 * math.sin(theta * 3.0)
            x = center_x + r * math.cos(theta)
            y = center_y + r * math.sin(theta)
            z += 0.15
            e += 0.28
            f.write(f"G1 X{x:.3f} Y{y:.3f} Z{z:.3f} E{e:.3f} F1800\n")
        f.write("M104 S0 ; Extruder off\n")
        f.write("M140 S0 ; Bed off\n")
        f.write("M84     ; Disable motors\n")
    print("  [GCODE] Created: spiral_helix_tower.gcode")

# =============================================================================
# 13. Solar Space Satellite (DAE)
# =============================================================================
def make_satellite_dae():
    path = os.path.join(OUT_DIR, "solar_satellite_array.dae")
    dae = """<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1">
  <asset>
    <contributor><author>ModelPeek</author></contributor>
    <created>2026-10-06T10:00:00</created>
    <unit name="meter" meter="1.0"/>
    <up_axis>Z_UP</up_axis>
  </asset>
  <library_geometries>
    <geometry id="Cube-mesh" name="SatelliteBody">
      <mesh>
        <source id="Cube-mesh-positions">
          <float_array id="Cube-mesh-positions-array" count="24">
            -10 -10 -15  10 -10 -15  10 10 -15  -10 10 -15
            -10 -10  15  10 -10  15  10 10  15  -10 10  15
          </float_array>
          <technique_common>
            <accessor source="#Cube-mesh-positions-array" count="8" stride="3">
              <param name="X" type="float"/><param name="Y" type="float"/><param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <vertices id="Cube-mesh-vertices">
          <input semantic="POSITION" source="#Cube-mesh-positions"/>
        </vertices>
        <polylist count="6">
          <input semantic="VERTEX" source="#Cube-mesh-vertices" offset="0"/>
          <vcount>4 4 4 4 4 4</vcount>
          <p>
            0 1 2 3  4 7 6 5  0 4 5 1
            1 5 6 2  2 6 7 3  3 7 4 0
          </p>
        </polylist>
      </mesh>
    </geometry>
  </library_geometries>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
      <node id="Satellite" name="Satellite" type="NODE">
        <instance_geometry url="#Cube-mesh"/>
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene>
    <instance_visual_scene url="#Scene"/>
  </scene>
</COLLADA>"""
    with open(path, "w", encoding="utf-8") as f:
        f.write(dae)
    print("  [DAE] Created: solar_satellite_array.dae")

# =============================================================================
# 14. Vintage Camera Body (3DS)
# =============================================================================
def make_camera_3ds():
    # Produces binary 3DS chunk format
    path = os.path.join(OUT_DIR, "vintage_camera_housing.3ds")
    verts = [
        (-25, -15, -10), (25, -15, -10), (25, 15, -10), (-25, 15, -10),
        (-25, -15, 10), (25, -15, 10), (25, 15, 10), (-25, 15, 10),
        (-8, -10, 18), (8, -10, 18), (8, 10, 18), (-8, 10, 18) # Pentaprism
    ]
    faces = [
        (0, 1, 2), (0, 2, 3), (4, 6, 5), (4, 7, 6),
        (0, 4, 1), (1, 4, 5), (1, 5, 2), (2, 5, 6),
        (2, 6, 3), (3, 6, 7), (3, 7, 0), (0, 7, 4),
        (4, 8, 5), (5, 8, 9), (5, 9, 6), (6, 9, 10),
        (6, 10, 7), (7, 10, 11), (7, 11, 4), (4, 11, 8),
        (8, 9, 10), (8, 10, 11)
    ]
    # 3DS binary chunk builder
    def chunk(cid, data):
        return struct.pack("<HI", cid, len(data) + 6) + data

    # Point list chunk (0x4110)
    pt_data = struct.pack("<H", len(verts))
    for v in verts:
        pt_data += struct.pack("<fff", v[0], v[1], v[2])
    c_pt = chunk(0x4110, pt_data)

    # Face list chunk (0x4120)
    fc_data = struct.pack("<H", len(faces))
    for fc in faces:
        fc_data += struct.pack("<HHHH", fc[0], fc[1], fc[2], 0x07) # flags
    c_fc = chunk(0x4120, fc_data)

    # Triangle mesh chunk (0x4100)
    c_tri = chunk(0x4100, c_pt + c_fc)

    # Named object chunk (0x4000)
    obj_name = b"CameraBody\x00"
    c_obj = chunk(0x4000, obj_name + c_tri)

    # 3D Editor chunk (0x3D3D)
    c_edit = chunk(0x3D3D, c_obj)

    # Main chunk (0x4D4D)
    c_main = chunk(0x4D4D, c_edit)

    with open(path, "wb") as f:
        f.write(c_main)
    print("  [3DS] Created: vintage_camera_housing.3ds")

# =============================================================================
# 15. Architectural Duplex Floorplan (DXF)
# =============================================================================
def make_floorplan_dxf():
    path = os.path.join(OUT_DIR, "architectural_floorplan_duplex.dxf")
    dxf = """0
SECTION
2
ENTITIES
0
LINE
8
WALLS
10
0.0
20
0.0
30
0.0
11
120.0
21
0.0
31
0.0
0
LINE
8
WALLS
10
120.0
20
0.0
30
0.0
11
120.0
21
80.0
31
0.0
0
LINE
8
WALLS
10
120.0
20
80.0
30
0.0
11
0.0
21
80.0
31
0.0
0
LINE
8
WALLS
10
0.0
20
80.0
30
0.0
11
0.0
21
0.0
31
0.0
0
LINE
8
INTERIOR_WALLS
10
60.0
20
0.0
30
0.0
11
60.0
21
80.0
31
0.0
0
CIRCLE
8
COLUMNS
10
30.0
20
40.0
30
0.0
40
3.5
0
CIRCLE
8
COLUMNS
10
90.0
20
40.0
30
0.0
40
3.5
0
ENDSEC
0
EOF
"""
    with open(path, "w", encoding="utf-8") as f:
        f.write(dxf)
    print("  [DXF] Created: architectural_floorplan_duplex.dxf")

# =============================================================================
# 16. Autonomous City Drive LiDAR (PCD)
# =============================================================================
def make_city_lidar_pcd():
    path = os.path.join(OUT_DIR, "autonomous_city_drive.pcd")
    points = []
    # Ground plane (-2.0m height, 40m x 20m grid)
    for x in range(-20, 20):
        for y in range(-10, 10):
            intensity = 200.0 if abs(y) == 3 else 30.0 # Lane markings brighter
            points.append((x * 1.5, y * 1.5, -2.0, intensity))
    # Two parked cars (box point clouds)
    for cx, cy in [(12.0, 5.0), (-10.0, -6.0)]:
        for dx in range(-4, 5):
            for dy in range(-2, 3):
                for dz in range(0, 4):
                    points.append((cx + dx*0.6, cy + dy*0.6, -2.0 + dz*0.5, 120.0))
    # Roadside trees / poles
    for tx, ty in [(0.0, 8.0), (15.0, 8.0), (-15.0, 8.0)]:
        for h in range(12):
            points.append((tx, ty, -2.0 + h*0.5, 80.0))
            # Foliage sphere
            if h >= 8:
                for a in range(8):
                    ang = a * (math.pi / 4)
                    points.append((tx + 1.2*math.cos(ang), ty + 1.2*math.sin(ang), -2.0 + h*0.5, 95.0))

    header = f"""# .PCD v0.7 - Point Cloud Data file format
VERSION 0.7
FIELDS x y z intensity
SIZE 4 4 4 4
TYPE F F F F
COUNT 1 1 1 1
WIDTH {len(points)}
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS {len(points)}
DATA ascii
"""
    with open(path, "w", encoding="utf-8") as f:
        f.write(header)
        for p in points:
            f.write(f"{p[0]:.3f} {p[1]:.3f} {p[2]:.3f} {p[3]:.1f}\n")
    print("  [PCD] Created: autonomous_city_drive.pcd")

# =============================================================================
# Main Driver
# =============================================================================
if __name__ == "__main__":
    print("--- Generating CAD Solids via OpenCASCADE ---")
    make_planetary_gear()
    make_valve_body()
    make_wing_profile()
    make_rocket_nozzle()
    make_gripper_and_bracket()
    make_heatsink_and_gyro()

    print("--- Generating Mesh, Point Clouds, Toolpaths & Drawings ---")
    make_quadcopter_obj()
    make_pavilion_gltf()
    make_cyber_car_glb()
    make_insole_3mf()
    make_amphora_ply()
    make_spiral_gcode()
    make_satellite_dae()
    make_camera_3ds()
    make_floorplan_dxf()
    make_city_lidar_pcd()
    print("All sample_models2 assets generated successfully!")
