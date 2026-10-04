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

def convert_step_to_stl(step_path, out_stl_path, deflection=0.1):
    shape = Part.Shape()
    shape.read(step_path)
    # Perform B-Rep meshing and export
    shape.exportStl(out_stl_path)
    return True

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
        print("Usage: cad_processor.py <convert|info> <input_file> [output_file]")
        sys.exit(1)

    cmd = sys.argv[1].lower()
    input_file = sys.argv[2]

    if cmd == "convert":
        if len(sys.argv) < 4:
            print("Missing output file argument", file=sys.stderr)
            sys.exit(1)
        out_file = sys.argv[3]
        if convert_step_to_stl(input_file, out_file):
            print(f"SUCCESS: Converted {input_file} -> {out_file}")
            sys.exit(0)
        else:
            sys.exit(1)

    elif cmd == "info":
        info = get_cad_info(input_file)
        print(json.dumps(info))
        sys.exit(0)
