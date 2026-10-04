#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstdint>

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

// Robust STL parser supporting both ASCII and Binary STL
bool LoadSTL(const std::string& path, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    // Check if ASCII or binary
    char header[6] = {0};
    f.read(header, 5);
    bool isAscii = (std::string(header).substr(0, 5) == "solid");

    if (isAscii) {
        // Double check by reading a few lines
        f.seekg(0, std::ios::beg);
        std::string line;
        bool foundFacet = false;
        for (int i = 0; i < 20 && std::getline(f, line); ++i) {
            if (line.find("facet") != std::string::npos) {
                foundFacet = true;
                break;
            }
        }
        if (!foundFacet) isAscii = false;
    }

    if (isAscii) {
        f.clear();
        f.seekg(0, std::ios::beg);
        std::string word;
        Triangle curTri;
        int vIdx = 0;

        while (f >> word) {
            if (word == "facet") {
                std::string normalWord;
                f >> normalWord >> curTri.normal.x >> curTri.normal.y >> curTri.normal.z;
                vIdx = 0;
            } else if (word == "vertex") {
                if (vIdx < 3) {
                    f >> curTri.v[vIdx].x >> curTri.v[vIdx].y >> curTri.v[vIdx].z;
                    minB.x = std::min(minB.x, curTri.v[vIdx].x);
                    minB.y = std::min(minB.y, curTri.v[vIdx].y);
                    minB.z = std::min(minB.z, curTri.v[vIdx].z);
                    maxB.x = std::max(maxB.x, curTri.v[vIdx].x);
                    maxB.y = std::max(maxB.y, curTri.v[vIdx].y);
                    maxB.z = std::max(maxB.z, curTri.v[vIdx].z);
                    vIdx++;
                }
            } else if (word == "endfacet") {
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
        return !tris.empty();
    } else {
        // Binary STL
        f.clear();
        f.seekg(80, std::ios::beg);
        uint32_t count = 0;
        f.read((char*)&count, 4);
        if (count == 0 || count > 10000000) return false;

        tris.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            float n[3], v[9];
            uint16_t attr;
            f.read((char*)n, 12);
            f.read((char*)v, 36);
            f.read((char*)&attr, 2);

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
        return true;
    }
}

// 256x256 Soft Rasterizer for CAD Thumbnails
void RenderThumbnail(const std::vector<Triangle>& tris, const Vec3& minB, const Vec3& maxB, int W, int H, const std::string& outBmp) {
    std::vector<uint32_t> pixels(W * H, 0xFF1E222B); // CAD dark slate background
    std::vector<float> zbuffer(W * H, 1e9f);

    Vec3 center = (minB + maxB) * 0.5f;
    Vec3 size = maxB - minB;
    float maxDim = std::max({size.x, size.y, size.z});
    if (maxDim < 1e-3f) maxDim = 1.0f;

    // Isometric camera rotation: 45 deg yaw, 35.264 deg pitch
    float yaw = 45.0f * 3.14159265f / 180.0f;
    float pitch = 30.0f * 3.14159265f / 180.0f;

    float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
    float cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);

    // Directional studio lights
    Vec3 light1 = Vec3{0.6f, 0.8f, 0.5f}.normalize();
    Vec3 light2 = Vec3{-0.5f, 0.3f, -0.4f}.normalize();

    auto project = [&](const Vec3& pt, float& sx, float& sy, float& sz) {
        Vec3 p = pt - center;
        // Yaw
        float x1 = p.x * cosYaw - p.z * sinYaw;
        float z1 = p.x * sinYaw + p.z * cosYaw;
        // Pitch
        float y2 = p.y * cosPitch - z1 * sinPitch;
        float z2 = p.y * sinPitch + z1 * cosPitch;

        float scale = (float)(std::min(W, H)) * 0.42f / (maxDim * 0.5f);
        sx = W * 0.5f + x1 * scale;
        sy = H * 0.5f - y2 * scale; // Y down for image
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

        // Lighting calculation
        // Transform normal
        Vec3 n = tri.normal;
        float nx1 = n.x * cosYaw - n.z * sinYaw;
        float nz1 = n.x * sinYaw + n.z * cosYaw;
        float ny2 = n.y * cosPitch - nz1 * sinPitch;
        float nz2 = n.y * sinPitch + nz1 * cosPitch;
        Vec3 rotN = Vec3{nx1, ny2, nz2}.normalize();

        // Two-sided illumination for CAD
        float diff1 = std::max(0.0f, rotN.dot(light1));
        float diff2 = std::max(0.0f, rotN.dot(light2)) * 0.4f;
        float ambient = 0.25f;
        float intensity = std::min(1.0f, ambient + diff1 * 0.65f + diff2);

        // CAD Metallic Aluminum Color (RGB: 144, 164, 174 -> #90A4AE)
        uint8_t r = (uint8_t)std::clamp((int)(144.0f * intensity + 20.0f), 0, 255);
        uint8_t g = (uint8_t)std::clamp((int)(164.0f * intensity + 25.0f), 0, 255);
        uint8_t b = (uint8_t)std::clamp((int)(174.0f * intensity + 30.0f), 0, 255);
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

        // Bounding box of triangle in 2D
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

    // Write standard 32-bit BMP
    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + W * H * 4;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = W;
    bih.biHeight = -H; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    std::ofstream out(outBmp, std::ios::binary);
    out.write((char*)&bfh, sizeof(bfh));
    out.write((char*)&bih, sizeof(bih));
    out.write((char*)pixels.data(), pixels.size() * 4);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: test_rasterizer <model.stl> <output.bmp> [size]" << std::endl;
        return 1;
    }
    int size = argc >= 4 ? std::atoi(argv[3]) : 256;
    std::vector<Triangle> tris;
    Vec3 minB, maxB;
    if (!LoadSTL(argv[1], tris, minB, maxB)) {
        std::cerr << "Failed to load STL: " << argv[1] << std::endl;
        return 1;
    }
    std::cout << "Loaded " << tris.size() << " triangles. Rendering " << size << "x" << size << " thumbnail..." << std::endl;
    RenderThumbnail(tris, minB, maxB, size, size, argv[2]);
    std::cout << "Rendered successfully to " << argv[2] << std::endl;
    return 0;
}
