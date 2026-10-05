#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>

struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
    }
    Vec3 normalize() const {
        float l = std::sqrt(x * x + y * y + z * z);
        return l > 1e-6f ? (*this) * (1.0f / l) : *this;
    }
};

struct Triangle {
    Vec3 v[3];
    Vec3 normal;
};

// Robust STL parser supporting both ASCII and Binary STL with wide filepath
bool LoadSTL(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"rb");
    if (!f) return false;

    char header[6] = {0};
    fread(header, 1, 5, f);
    bool isAscii = (std::string(header).substr(0, 5) == "solid");

    if (isAscii) {
        fseek(f, 0, SEEK_SET);
        char buf[256];
        bool foundFacet = false;
        for (int i = 0; i < 20 && fgets(buf, sizeof(buf), f); ++i) {
            if (strstr(buf, "facet")) {
                foundFacet = true;
                break;
            }
        }
        if (!foundFacet) isAscii = false;
    }

    if (isAscii) {
        fseek(f, 0, SEEK_SET);
        char word[64];
        Triangle curTri;
        int vIdx = 0;

        while (fscanf(f, "%63s", word) == 1) {
            if (strcmp(word, "facet") == 0) {
                char dummy[64];
                fscanf(f, "%63s %f %f %f", dummy, &curTri.normal.x, &curTri.normal.y, &curTri.normal.z);
                vIdx = 0;
            } else if (strcmp(word, "vertex") == 0) {
                if (vIdx < 3) {
                    fscanf(f, "%f %f %f", &curTri.v[vIdx].x, &curTri.v[vIdx].y, &curTri.v[vIdx].z);
                    minB.x = std::min(minB.x, curTri.v[vIdx].x);
                    minB.y = std::min(minB.y, curTri.v[vIdx].y);
                    minB.z = std::min(minB.z, curTri.v[vIdx].z);
                    maxB.x = std::max(maxB.x, curTri.v[vIdx].x);
                    maxB.y = std::max(maxB.y, curTri.v[vIdx].y);
                    maxB.z = std::max(maxB.z, curTri.v[vIdx].z);
                    vIdx++;
                }
            } else if (strcmp(word, "endfacet") == 0) {
                if (curTri.normal.dot(curTri.normal) < 1e-4f) {
                    Vec3 edge1 = curTri.v[1] - curTri.v[0];
                    Vec3 edge2 = curTri.v[2] - curTri.v[0];
                    curTri.normal = edge1.cross(edge2).normalize();
                } else {
                    curTri.normal = curTri.normal.normalize();
                }
                tris.push_back(curTri);
            }
        }
        fclose(f);
        return !tris.empty();
    } else {
        fseek(f, 80, SEEK_SET);
        uint32_t count = 0;
        fread(&count, 4, 1, f);
        if (count == 0 || count > 10000000) {
            fclose(f);
            return false;
        }

        tris.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            float n[3], v[9];
            uint16_t attr;
            fread(n, 4, 3, f);
            fread(v, 4, 9, f);
            fread(&attr, 2, 1, f);

            tris[i].normal = {n[0], n[1], n[2]};
            for (int j = 0; j < 3; ++j) {
                tris[i].v[j] = {v[j * 3], v[j * 3 + 1], v[j * 3 + 2]};
                minB.x = std::min(minB.x, tris[i].v[j].x);
                minB.y = std::min(minB.y, tris[i].v[j].y);
                minB.z = std::min(minB.z, tris[i].v[j].z);
                maxB.x = std::max(maxB.x, tris[i].v[j].x);
                maxB.y = std::max(maxB.y, tris[i].v[j].y);
                maxB.z = std::max(maxB.z, tris[i].v[j].z);
            }

            if (tris[i].normal.dot(tris[i].normal) < 1e-4f) {
                Vec3 edge1 = tris[i].v[1] - tris[i].v[0];
                Vec3 edge2 = tris[i].v[2] - tris[i].v[0];
                tris[i].normal = edge1.cross(edge2).normalize();
            } else {
                tris[i].normal = tris[i].normal.normalize();
            }
        }
        fclose(f);
        return true;
    }
}

// Wavefront OBJ parser with wide filepath
bool LoadOBJ(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"r");
    if (!f) return false;

    std::vector<Vec3> verts;
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            Vec3 pt;
            if (sscanf(line + 2, "%f %f %f", &pt.x, &pt.y, &pt.z) == 3) {
                verts.push_back(pt);
                minB.x = std::min(minB.x, pt.x);
                minB.y = std::min(minB.y, pt.y);
                minB.z = std::min(minB.z, pt.z);
                maxB.x = std::max(maxB.x, pt.x);
                maxB.y = std::max(maxB.y, pt.y);
                maxB.z = std::max(maxB.z, pt.z);
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            std::vector<int> faceIndices;
            char* token = strtok(line + 2, " \t\r\n");
            while (token) {
                int idx = 0;
                sscanf(token, "%d", &idx);
                if (idx > 0) idx -= 1;
                else if (idx < 0) idx = (int)verts.size() + idx;
                faceIndices.push_back(idx);
                token = strtok(NULL, " \t\r\n");
            }

            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                int i0 = faceIndices[0];
                int i1 = faceIndices[i];
                int i2 = faceIndices[i + 1];

                if (i0 >= 0 && i0 < (int)verts.size() &&
                    i1 >= 0 && i1 < (int)verts.size() &&
                    i2 >= 0 && i2 < (int)verts.size()) {
                    Triangle tri;
                    tri.v[0] = verts[i0];
                    tri.v[1] = verts[i1];
                    tri.v[2] = verts[i2];
                    Vec3 e1 = tri.v[1] - tri.v[0];
                    Vec3 e2 = tri.v[2] - tri.v[0];
                    tri.normal = e1.cross(e2).normalize();
                    tris.push_back(tri);
                }
            }
        }
    }
    fclose(f);
    return !tris.empty();
}

// Stanford PLY parser (ASCII & basic Little Endian)
bool LoadPLY(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"r");
    if (!f) return false;

    char line[256];
    if (!fgets(line, sizeof(line), f) || strncmp(line, "ply", 3) != 0) {
        fclose(f);
        return false;
    }

    int numVerts = 0;
    int numFaces = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "element vertex", 14) == 0) {
            sscanf(line + 14, "%d", &numVerts);
        } else if (strncmp(line, "element face", 12) == 0) {
            sscanf(line + 12, "%d", &numFaces);
        } else if (strncmp(line, "end_header", 10) == 0) {
            break;
        }
    }

    if (numVerts <= 0) {
        fclose(f);
        return false;
    }

    std::vector<Vec3> verts;
    verts.reserve(numVerts);

    for (int i = 0; i < numVerts; ++i) {
        Vec3 pt;
        if (fscanf(f, "%f %f %f", &pt.x, &pt.y, &pt.z) == 3) {
            char rest[256];
            fgets(rest, sizeof(rest), f);
            verts.push_back(pt);
            minB.x = std::min(minB.x, pt.x);
            minB.y = std::min(minB.y, pt.y);
            minB.z = std::min(minB.z, pt.z);
            maxB.x = std::max(maxB.x, pt.x);
            maxB.y = std::max(maxB.y, pt.y);
            maxB.z = std::max(maxB.z, pt.z);
        }
    }

    for (int i = 0; i < numFaces; ++i) {
        int n = 0;
        if (fscanf(f, "%d", &n) == 1 && n >= 3) {
            std::vector<int> idx(n);
            for (int j = 0; j < n; ++j) {
                fscanf(f, "%d", &idx[j]);
            }
            char rest[256];
            fgets(rest, sizeof(rest), f);

            for (int j = 1; j + 1 < n; ++j) {
                int i0 = idx[0], i1 = idx[j], i2 = idx[j + 1];
                if (i0 < (int)verts.size() && i1 < (int)verts.size() && i2 < (int)verts.size()) {
                    Triangle tri;
                    tri.v[0] = verts[i0];
                    tri.v[1] = verts[i1];
                    tri.v[2] = verts[i2];
                    Vec3 e1 = tri.v[1] - tri.v[0];
                    Vec3 e2 = tri.v[2] - tri.v[0];
                    tri.normal = e1.cross(e2).normalize();
                    tris.push_back(tri);
                }
            }
        }
    }
    fclose(f);
    return !tris.empty();
}

// 256x256 Soft Rasterizer for CAD Thumbnails
void RenderThumbnail(const std::vector<Triangle>& tris, const Vec3& minB, const Vec3& maxB, int W, int H, const std::wstring& outBmp) {
    std::vector<uint32_t> pixels(W * H, 0xFF1E222B); // CAD dark slate background
    std::vector<float> zbuffer(W * H, 1e9f);

    Vec3 center = (minB + maxB) * 0.5f;
    Vec3 size = maxB - minB;
    float maxDim = std::max({size.x, size.y, size.z});
    if (maxDim < 1e-3f) maxDim = 1.0f;

    float yaw = 45.0f * 3.14159265f / 180.0f;
    float pitch = 30.0f * 3.14159265f / 180.0f;

    float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
    float cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);

    Vec3 light1 = Vec3{0.6f, 0.8f, 0.5f}.normalize();
    Vec3 light2 = Vec3{-0.5f, 0.3f, -0.4f}.normalize();

    auto project = [&](const Vec3& pt, float& sx, float& sy, float& sz) {
        Vec3 p = pt - center;
        float x1 = p.x * cosYaw - p.z * sinYaw;
        float z1 = p.x * sinYaw + p.z * cosYaw;
        float y2 = p.y * cosPitch - z1 * sinPitch;
        float z2 = p.y * sinPitch + z1 * cosPitch;

        float scale = (float)(std::min(W, H)) * 0.42f / (maxDim * 0.5f);
        sx = W * 0.5f + x1 * scale;
        sy = H * 0.5f - y2 * scale;
        sz = z2;
    };

    auto edgeFunction = [](float ax, float ay, float bx, float by, float cx, float cy) {
        return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    };

    for (const auto& tri : tris) {
        float sx[3], sy[3], sz[3];
        for (int j = 0; j < 3; ++j) {
            project(tri.v[j], sx[j], sy[j], sz[j]);
        }

        Vec3 n = tri.normal;
        float nx1 = n.x * cosYaw - n.z * sinYaw;
        float nz1 = n.x * sinYaw + n.z * cosYaw;
        float ny2 = n.y * cosPitch - nz1 * sinPitch;
        float nz2 = n.y * sinPitch + nz1 * cosPitch;
        Vec3 rotN = Vec3{nx1, ny2, nz2}.normalize();

        float diff1 = std::max(0.0f, rotN.dot(light1));
        float diff2 = std::max(0.0f, rotN.dot(light2)) * 0.4f;
        float ambient = 0.25f;
        float intensity = std::min(1.0f, ambient + diff1 * 0.65f + diff2);

        uint8_t r = (uint8_t)std::clamp((int)(144.0f * intensity + 20.0f), 0, 255);
        uint8_t g = (uint8_t)std::clamp((int)(164.0f * intensity + 25.0f), 0, 255);
        uint8_t b = (uint8_t)std::clamp((int)(174.0f * intensity + 30.0f), 0, 255);
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

        int minX = std::max(0, (int)std::floor(std::min({sx[0], sx[1], sx[2]})));
        int maxX = std::min(W - 1, (int)std::ceil(std::max({sx[0], sx[1], sx[2]})));
        int minY = std::max(0, (int)std::floor(std::min({sy[0], sy[1], sy[2]})));
        int maxY = std::min(H - 1, (int)std::ceil(std::max({sy[0], sy[1], sy[2]})));

        float area = edgeFunction(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]);
        if (std::abs(area) < 1e-4f) continue;
        float invArea = 1.0f / area;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px = x + 0.5f, py = y + 0.5f;
                float w0 = edgeFunction(sx[1], sy[1], sx[2], sy[2], px, py) * invArea;
                float w1 = edgeFunction(sx[2], sy[2], sx[0], sy[0], px, py) * invArea;
                float w2 = edgeFunction(sx[0], sy[0], sx[1], sy[1], px, py) * invArea;

                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    float z = w0 * sz[0] + w1 * sz[1] + w2 * sz[2];
                    int idx = y * W + x;
                    if (z < zbuffer[idx]) {
                        zbuffer[idx] = z;
                        pixels[idx] = color;
                    }
                }
            }
        }
    }

    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    bfh.bfType = 0x4D42;
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + W * H * 4;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = W;
    bih.biHeight = -H;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    FILE* out = _wfopen(outBmp.c_str(), L"wb");
    if (out) {
        fwrite(&bfh, sizeof(bfh), 1, out);
        fwrite(&bih, sizeof(bih), 1, out);
        fwrite(pixels.data(), 4, pixels.size(), out);
        fclose(out);
    }
}

std::wstring FindOCCPython() {
    WCHAR selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    PathRemoveFileSpecW(selfPath);

    std::wstring p1 = std::wstring(selfPath) + L"\\occt\\python.exe";
    if (PathFileExistsW(p1.c_str())) return p1;

    std::wstring p2 = L"D:\\SoftWare2\\FreeCAD 1.1\\bin\\python.exe";
    if (PathFileExistsW(p2.c_str())) return p2;

    return L"python.exe";
}

std::wstring GetCadProcessorScript() {
    WCHAR selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    PathRemoveFileSpecW(selfPath);

    std::wstring s1 = std::wstring(selfPath) + L"\\cad_processor.py";
    if (PathFileExistsW(s1.c_str())) return s1;

    std::wstring s2 = std::wstring(selfPath) + L"\\..\\src\\worker\\cad_processor.py";
    return s2;
}

bool RunCadConverter(const std::wstring& inputStep, const std::wstring& outputStl) {
    std::wstring pythonExe = FindOCCPython();
    std::wstring script = GetCadProcessorScript();

    WCHAR fullIn[MAX_PATH] = {0};
    WCHAR fullOut[MAX_PATH] = {0};
    GetFullPathNameW(inputStep.c_str(), MAX_PATH, fullIn, NULL);
    GetFullPathNameW(outputStl.c_str(), MAX_PATH, fullOut, NULL);

    std::wstringstream cmd;
    cmd << L"\"" << pythonExe << L"\" \"" << script << L"\" convert \"" 
        << fullIn << L"\" \"" << fullOut << L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::wstring cmdStr = cmd.str();
    std::vector<WCHAR> cmdLine(cmdStr.begin(), cmdStr.end());
    cmdLine.push_back(L'\0');

    if (!CreateProcessW(NULL, cmdLine.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 15000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return PathFileExistsW(fullOut) == TRUE;
}

bool ProcessThumbnail(const std::wstring& input, const std::wstring& output, int size) {
    std::wstring ext;
    size_t dot = input.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        ext = input.substr(dot);
        for (auto& c : ext) c = towlower(c);
    }

    std::vector<Triangle> tris;
    Vec3 minB, maxB;
    std::wstring meshToRender = input;
    std::wstring tempStl = L"";

    bool isCad = (ext == L".step" || ext == L".stp" || ext == L".iges" || ext == L".igs" || ext == L".brep" || ext == L".brp");
    if (isCad) {
        tempStl = output + L".tmp.stl";
        if (!RunCadConverter(input, tempStl)) {
            return false;
        }
        meshToRender = tempStl;
        ext = L".stl";
    }

    bool loaded = false;
    if (ext == L".stl") {
        loaded = LoadSTL(meshToRender, tris, minB, maxB);
    } else if (ext == L".obj") {
        loaded = LoadOBJ(meshToRender, tris, minB, maxB);
    } else if (ext == L".ply") {
        loaded = LoadPLY(meshToRender, tris, minB, maxB);
    }

    if (!tempStl.empty()) {
        DeleteFileW(tempStl.c_str());
    }

    if (!loaded || tris.empty()) {
        return false;
    }

    RenderThumbnail(tris, minB, maxB, size, size, output);
    return PathFileExistsW(output.c_str()) == TRUE;
}

void RunDaemonMode() {
    LPCWSTR pipeName = L"\\\\.\\pipe\\ModelPeekWorkerPipe";
    std::wcout << L"ModelPeek Worker Daemon started on " << pipeName << std::endl;
    while (true) {
        HANDLE hPipe = CreateNamedPipeW(
            pipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096, 4096, 5000, NULL
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(100);
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            WCHAR buffer[2048] = {0};
            DWORD bytesRead = 0;
            if (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(WCHAR), &bytesRead, NULL)) {
                std::wstring request = buffer;
                std::wstringstream ss(request);
                std::wstring op, arg1, arg2, arg3;
                std::getline(ss, op, L'\t');
                std::getline(ss, arg1, L'\t');
                std::getline(ss, arg2, L'\t');
                std::getline(ss, arg3, L'\t');

                std::wstring response = L"FAILED\n";
                if (op == L"thumbnail" && !arg1.empty() && !arg2.empty()) {
                    int size = arg3.empty() ? 256 : _wtoi(arg3.c_str());
                    if (ProcessThumbnail(arg1, arg2, size)) {
                        response = L"SUCCESS\n";
                    }
                } else if (op == L"convert" && !arg1.empty() && !arg2.empty()) {
                    if (RunCadConverter(arg1, arg2)) {
                        response = L"SUCCESS\n";
                    }
                }

                DWORD bytesWritten = 0;
                WriteFile(hPipe, response.c_str(), (DWORD)(response.length() * sizeof(WCHAR)), &bytesWritten, NULL);
            }
        }
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}

int main(int, char*[]) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv || argc < 2) {
        std::wcout << L"ModelPeek Worker v2.0\n"
                   << L"Usage:\n"
                   << L"  ModelPeekWorker.exe thumbnail <input_model> <output_bmp> [size]\n"
                   << L"  ModelPeekWorker.exe convert <input_step> <output_stl>\n"
                   << L"  ModelPeekWorker.exe --daemon\n";
        if (argv) LocalFree(argv);
        return 0;
    }

    std::wstring cmd = argv[1];

    if (cmd == L"--daemon" || cmd == L"daemon") {
        LocalFree(argv);
        RunDaemonMode();
        return 0;
    } else if (cmd == L"convert" && argc >= 4) {
        std::wstring input = argv[2];
        std::wstring output = argv[3];
        if (RunCadConverter(input, output)) {
            std::wcout << L"SUCCESS" << std::endl;
            LocalFree(argv);
            return 0;
        } else {
            std::wcerr << L"FAILED" << std::endl;
            LocalFree(argv);
            return 1;
        }
    } else if (cmd == L"thumbnail" && argc >= 4) {
        std::wstring input = argv[2];
        std::wstring output = argv[3];
        int size = (argc >= 5) ? _wtoi(argv[4]) : 256;

        if (ProcessThumbnail(input, output, size)) {
            std::wcout << L"SUCCESS" << std::endl;
            LocalFree(argv);
            return 0;
        } else {
            std::wcerr << L"Failed to generate thumbnail" << std::endl;
            LocalFree(argv);
            return 1;
        }
    }

    LocalFree(argv);
    return 1;
}
