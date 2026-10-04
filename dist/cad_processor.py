import sys
import os
import json
import math
import struct

# Ensure FreeCAD / OpenCASCADE modules can be loaded
for p in [r"D:\SoftWare2\FreeCAD 1.1\bin", r"D:\SoftWare2\FreeCAD 1.1\lib",
          os.path.join(os.path.dirname(__file__), "..", "occt", "bin"),
          os.path.join(os.path.dirname(__file__), "..", "occt", "lib")]:
    if os.path.exists(p) and p not in sys.path:
        sys.path.insert(0, p)

try:
    import FreeCAD
    import Part
except ImportError as e:
    print(f"ERROR: Cannot import OpenCASCADE / Part: {e}", file=sys.stderr)
    sys.exit(1)

def _normalize(v):
    l = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    if l < 1e-9:
        return (0.0, 0.0, 1.0)
    return (v[0]/l, v[1]/l, v[2]/l)

def _cross(a, b):
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )

def _sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])

def _add(a, b):
    return (a[0]+b[0], a[1]+b[1], a[2]+b[2])

def _mul(a, s):
    return (a[0]*s, a[1]*s, a[2]*s)

def _get_perp_vectors(dir_vec):
    d = _normalize(dir_vec)
    if abs(d[0]) < 0.9 and abs(d[1]) < 0.9:
        ref = (0.0, 0.0, 1.0)
    else:
        ref = (0.0, 1.0, 0.0)
    u1 = _normalize(_cross(d, ref))
    u2 = _cross(d, u1)
    return u1, u2

def edges_to_stl(edges, bbox, out_stl_path, sides=6):
    max_dim = max(bbox.XLength, bbox.YLength, bbox.ZLength)
    if max_dim <= 1e-4:
        max_dim = 10.0
    radius = max(max_dim * 0.005, 0.04)

    triangles = []

    for edge in edges:
        if edge.Length <= 1e-6:
            continue
        n_pts = max(8, min(60, int(edge.Length / (max_dim * 0.02 + 0.05))))
        fc_pts = edge.discretize(Number=n_pts)
        pts = [(p.x, p.y, p.z) for p in fc_pts]
        if len(pts) < 2:
            continue

        rings = []
        for i in range(len(pts)):
            p = pts[i]
            if i == 0:
                tangent = _sub(pts[1], pts[0])
            elif i == len(pts) - 1:
                tangent = _sub(pts[-1], pts[-2])
            else:
                tangent = _sub(pts[i+1], pts[i-1])
            u1, u2 = _get_perp_vectors(tangent)
            ring = []
            for s in range(sides):
                angle = 2.0 * math.pi * s / sides
                offset = _add(_mul(u1, radius * math.cos(angle)), _mul(u2, radius * math.sin(angle)))
                ring.append(_add(p, offset))
            rings.append(ring)

        for i in range(len(rings) - 1):
            r1 = rings[i]
            r2 = rings[i+1]
            for s in range(sides):
                next_s = (s + 1) % sides
                p1 = r1[s]
                p2 = r1[next_s]
                p3 = r2[next_s]
                p4 = r2[s]
                norm1 = _normalize(_cross(_sub(p2, p1), _sub(p3, p1)))
                triangles.append((norm1, p1, p2, p3))
                norm2 = _normalize(_cross(_sub(p3, p1), _sub(p4, p1)))
                triangles.append((norm2, p1, p3, p4))

        # End caps
        start_center = pts[0]
        start_norm = _normalize(_sub(pts[0], pts[1]))
        r_start = rings[0]
        for s in range(sides):
            next_s = (s + 1) % sides
            triangles.append((start_norm, start_center, r_start[next_s], r_start[s]))

        end_center = pts[-1]
        end_norm = _normalize(_sub(pts[-1], pts[-2]))
        r_end = rings[-1]
        for s in range(sides):
            next_s = (s + 1) % sides
            triangles.append((end_norm, end_center, r_end[s], r_end[next_s]))

    with open(out_stl_path, 'wb') as f:
        header = b"ModelPeek Wireframe STL representation"
        f.write(header.ljust(80, b'\0'))
        f.write(struct.pack('<I', len(triangles)))
        for norm, p1, p2, p3 in triangles:
            f.write(struct.pack('<3f', norm[0], norm[1], norm[2]))
            f.write(struct.pack('<3f', p1[0], p1[1], p1[2]))
            f.write(struct.pack('<3f', p2[0], p2[1], p2[2]))
            f.write(struct.pack('<3f', p3[0], p3[1], p3[2]))
            f.write(struct.pack('<H', 0))

def convert_cad_to_stl(cad_path, out_stl_path, deflection=0.1):
    shape = Part.Shape()
    shape.read(cad_path)
    if len(shape.Faces) > 0:
        shape.exportStl(out_stl_path)
        if os.path.exists(out_stl_path) and os.path.getsize(out_stl_path) > 0:
            return True

    # If the shape has no polygonal faces (pure wireframe/curve geometry)
    # or exportStl produced empty file, generate tubular triangular mesh from edges
    if len(shape.Edges) > 0:
        try:
            edges_to_stl(shape.Edges, shape.BoundBox, out_stl_path)
            if os.path.exists(out_stl_path) and os.path.getsize(out_stl_path) > 0:
                return True
        except Exception as e:
            print(f"WARN: edges_to_stl failed: {e}", file=sys.stderr)

    return False

def get_cad_info(file_path):
    shape = Part.Shape()
    shape.read(file_path)
    bbox = shape.BoundBox
    return {
        "x": bbox.XLength,
        "y": bbox.YLength,
        "z": bbox.ZLength,
        "volume": shape.Volume,
        "area": shape.Area,
        "faces": len(shape.Faces),
        "edges": len(shape.Edges),
        "vertices": len(shape.Vertexes)
    }

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: cad_processor.py <convert|info|export> <input_file> [output_file]")
        sys.exit(1)

    cmd = sys.argv[1].lower()
    input_file = os.path.abspath(sys.argv[2])

    if cmd == "convert":
        if len(sys.argv) < 4:
            print("Missing output file argument", file=sys.stderr)
            sys.exit(1)
        out_file = os.path.abspath(sys.argv[3])
        if convert_cad_to_stl(input_file, out_file):
            print(f"SUCCESS: Converted {input_file} -> {out_file}")
            sys.exit(0)
        else:
            print(f"FAILED: Conversion failed for {input_file}", file=sys.stderr)
            sys.exit(1)

    elif cmd == "export":
        if len(sys.argv) < 4:
            sys.exit(1)
        out_file = os.path.abspath(sys.argv[3])
        shape = Part.Shape()
        shape.read(input_file)
        ext = os.path.splitext(out_file)[1].lower()
        if ext in [".iges", ".igs"]:
            shape.exportIges(out_file)
        elif ext in [".brep", ".brp"]:
            shape.exportBrep(out_file)
        elif ext == ".step" or ext == ".stp":
            shape.exportStep(out_file)
        print(f"SUCCESS: Exported {out_file}")
        sys.exit(0)

    elif cmd == "info":
        info = get_cad_info(input_file)
        print(json.dumps(info))
        sys.exit(0)
