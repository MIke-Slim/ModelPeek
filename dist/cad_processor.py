import sys
import os
import json

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

def convert_cad_to_stl(cad_path, out_stl_path, deflection=0.1):
    shape = Part.Shape()
    shape.read(cad_path)
    if len(shape.Faces) == 0 and len(shape.Edges) > 0:
        # If shape is only wireframe edges (e.g. 2D/3D curve geometry), create a visible pipe/mesh
        wires = shape.Wires
        if wires:
            try:
                shape = shape.makeOffsetShape(0.2, 0.01)
            except Exception:
                pass
    shape.exportStl(out_stl_path)
    return os.path.exists(out_stl_path) and os.path.getsize(out_stl_path) > 0

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

