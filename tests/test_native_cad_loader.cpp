#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cstdint>
#include <regex>

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

struct LineSeg {
    Vec3 p1, p2;
};

static void RibbonizeSegments(const std::vector<LineSeg>& segs, std::vector<Triangle>& tris, const Vec3& minB, const Vec3& maxB) {
    float maxDim = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z});
    if (maxDim < 1e-4f) maxDim = 1.0f;
    float r = std::max(maxDim * 0.0035f, 0.02f);

    for (const auto& s : segs) {
        Vec3 delta = s.p2 - s.p1;
        float len = std::sqrt(delta.dot(delta));
        if (len < 1e-5f) continue;
        Vec3 dir = delta * (1.0f / len);
        Vec3 ref = (std::abs(dir.x) < 0.9f && std::abs(dir.y) < 0.9f) ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
        Vec3 u1 = dir.cross(ref).normalize() * r;
        Vec3 u2 = dir.cross(u1).normalize() * r;

        Vec3 q1a = s.p1 - u1, q1b = s.p1 + u1, q1c = s.p2 + u1, q1d = s.p2 - u1;
        Vec3 n1 = u2.normalize();
        tris.push_back({ {q1a, q1b, q1c}, n1 });
        tris.push_back({ {q1a, q1c, q1d}, n1 });

        Vec3 q2a = s.p1 - u2, q2b = s.p1 + u2, q2c = s.p2 + u2, q2d = s.p2 - u2;
        Vec3 n2 = u1.normalize();
        tris.push_back({ {q2a, q2b, q2c}, n2 });
        tris.push_back({ {q2a, q2c, q2d}, n2 });
    }
}

bool LoadSTEP(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0 || fsize > 150 * 1024 * 1024) { fclose(f); return false; }

    std::string content(fsize, '\0');
    fread(&content[0], 1, fsize, f);
    fclose(f);

    std::unordered_map<int, Vec3> points;
    std::unordered_map<int, int> vertex_to_point;
    std::unordered_map<int, Vec3> vertices;
    std::unordered_map<int, std::pair<int, int>> edge_curves;
    std::unordered_map<int, std::pair<int, bool>> oriented_edges;
    std::unordered_map<int, std::vector<int>> edge_loops_map;
    std::vector<LineSeg> segs;
    std::vector<Triangle> rawFaces;

    auto updateBBox = [&](const Vec3& pt) {
        minB.x = std::min(minB.x, pt.x); minB.y = std::min(minB.y, pt.y); minB.z = std::min(minB.z, pt.z);
        maxB.x = std::max(maxB.x, pt.x); maxB.y = std::max(maxB.y, pt.y); maxB.z = std::max(maxB.z, pt.z);
    };

    size_t pos = 0;
    while (pos < content.size()) {
        size_t semiPos = content.find(';', pos);
        if (semiPos == std::string::npos) break;

        std::string stmt = content.substr(pos, semiPos - pos);
        pos = semiPos + 1;

        size_t hashPos = stmt.find('#');
        if (hashPos == std::string::npos) continue;

        size_t eqPos = stmt.find('=', hashPos);
        if (eqPos == std::string::npos) continue;

        std::string idStr = stmt.substr(hashPos + 1, eqPos - (hashPos + 1));
        while (!idStr.empty() && (idStr.front() == ' ' || idStr.front() == '\t' || idStr.front() == '\r' || idStr.front() == '\n')) idStr.erase(0, 1);
        while (!idStr.empty() && (idStr.back() == ' ' || idStr.back() == '\t' || idStr.back() == '\r' || idStr.back() == '\n')) idStr.pop_back();

        int id = 0;
        try { id = std::stoi(idStr); } catch (...) { continue; }

        size_t parenOpen = stmt.find('(', eqPos);
        if (parenOpen == std::string::npos) continue;

        std::string type = stmt.substr(eqPos + 1, parenOpen - (eqPos + 1));
        while (!type.empty() && (type.front() == ' ' || type.front() == '\t' || type.front() == '\r' || type.front() == '\n')) type.erase(0, 1);
        while (!type.empty() && (type.back() == ' ' || type.back() == '\t' || type.back() == '\r' || type.back() == '\n')) type.pop_back();

        if (type == "CARTESIAN_POINT") {
            // e.g. ('', (x, y, z))
            size_t innerOpen = stmt.find('(', parenOpen + 1);
            size_t innerClose = stmt.find(')', innerOpen != std::string::npos ? innerOpen : parenOpen);
            if (innerOpen != std::string::npos && innerClose != std::string::npos) {
                std::string coordsStr = stmt.substr(innerOpen + 1, innerClose - (innerOpen + 1));
                std::stringstream ss(coordsStr);
                std::string part;
                std::vector<float> vals;
                while (std::getline(ss, part, ',')) {
                    try { vals.push_back((float)std::stod(part)); } catch (...) {}
                }
                if (vals.size() == 3) {
                    points[id] = { vals[0], vals[1], vals[2] };
                }
            }
        } else if (type == "VERTEX_POINT") {
            // e.g. ('', #pt_id)
            size_t pHash = stmt.find('#', parenOpen);
            if (pHash != std::string::npos) {
                try {
                    int ptId = std::stoi(stmt.substr(pHash + 1));
                    vertex_to_point[id] = ptId;
                } catch (...) {}
            }
        } else if (type == "EDGE_CURVE") {
            // e.g. ('', #v1, #v2, #crv, .T.)
            std::vector<int> refs;
            size_t cur = parenOpen;
            while ((cur = stmt.find('#', cur)) != std::string::npos) {
                try {
                    refs.push_back(std::stoi(stmt.substr(cur + 1)));
                } catch (...) {}
                cur++;
            }
            if (refs.size() >= 2) {
                edge_curves[id] = { refs[0], refs[1] };
                if (vertices.count(refs[0]) && vertices.count(refs[1])) {
                    segs.push_back({ vertices[refs[0]], vertices[refs[1]] });
                }
            }
        } else if (type == "ORIENTED_EDGE") {
            // e.g. ('', *, *, #eid, .T.)
            size_t eidHash = stmt.rfind('#');
            if (eidHash != std::string::npos) {
                try {
                    int eid = std::stoi(stmt.substr(eidHash + 1));
                    bool sense = (stmt.find(".T.") != std::string::npos);
                    oriented_edges[id] = { eid, sense };
                } catch (...) {}
            }
        } else if (type == "POLY_LOOP") {
            // e.g. ('', (#p1, #p2, #p3...))
            std::vector<Vec3> loopPts;
            size_t cur = parenOpen;
            while ((cur = stmt.find('#', cur)) != std::string::npos) {
                try {
                    int pid = std::stoi(stmt.substr(cur + 1));
                    if (points.count(pid)) loopPts.push_back(points[pid]);
                } catch (...) {}
                cur++;
            }
            if (loopPts.size() >= 3) {
                Vec3 p0 = loopPts[0];
                for (size_t i = 1; i + 1 < loopPts.size(); ++i) {
                    Vec3 p1 = loopPts[i], p2 = loopPts[i + 1];
                    Vec3 norm = (p1 - p0).cross(p2 - p0).normalize();
                    rawFaces.push_back({ {p0, p1, p2}, norm });
                }
            }
        } else if (type == "EDGE_LOOP") {
            // e.g. ('', (#oe1, #oe2, ...))
            std::vector<int> oeList;
            size_t cur = parenOpen;
            while ((cur = stmt.find('#', cur)) != std::string::npos) {
                try {
                    oeList.push_back(std::stoi(stmt.substr(cur + 1)));
                } catch (...) {}
                cur++;
            }
            if (!oeList.empty()) {
                edge_loops_map[id] = oeList;
            }
        }
    }

    // Resolve vertices
    for (const auto& [vid, ptId] : vertex_to_point) {
        if (points.count(ptId)) {
            vertices[vid] = points[ptId];
            updateBBox(points[ptId]);
        }
    }

    // Resolve edge curves into line segments
    for (const auto& [eid, ec] : edge_curves) {
        if (vertices.count(ec.first) && vertices.count(ec.second)) {
            segs.push_back({ vertices[ec.first], vertices[ec.second] });
        }
    }

    // Resolve edge loops into faces
    for (const auto& [lid, oeList] : edge_loops_map) {
        std::vector<Vec3> loopPts;
        for (int oeId : oeList) {
            if (oriented_edges.count(oeId)) {
                auto [eid, sense] = oriented_edges[oeId];
                if (edge_curves.count(eid)) {
                    auto [v1, v2] = edge_curves[eid];
                    int vId = sense ? v1 : v2;
                    if (vertices.count(vId)) {
                        loopPts.push_back(vertices[vId]);
                    }
                }
            }
        }
        if (loopPts.size() >= 3) {
            Vec3 p0 = loopPts[0];
            for (size_t i = 1; i + 1 < loopPts.size(); ++i) {
                Vec3 p1 = loopPts[i], p2 = loopPts[i + 1];
                Vec3 norm = (p1 - p0).cross(p2 - p0).normalize();
                rawFaces.push_back({ {p0, p1, p2}, norm });
            }
        }
    }

    std::cout << "DEBUG STEP: points=" << points.size() 
              << " vertices=" << vertices.size() 
              << " edge_curves=" << edge_curves.size() 
              << " oriented_edges=" << oriented_edges.size() 
              << " loops=" << edge_loops_map.size() 
              << " segs=" << segs.size() 
              << " rawFaces=" << rawFaces.size() << std::endl;

    if (rawFaces.empty() && segs.empty()) return false;

    tris.insert(tris.end(), rawFaces.begin(), rawFaces.end());
    if (tris.size() < 12 && !segs.empty()) {
        RibbonizeSegments(segs, tris, minB, maxB);
    }

    return !tris.empty();
}

bool LoadIGES(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"r");
    if (!f) return false;

    std::string paramText;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len >= 73 && line[72] == 'P') {
            line[64] = '\0';
            paramText += line;
        }
    }
    fclose(f);

    if (paramText.empty()) return false;

    std::vector<LineSeg> segs;
    auto updateBBox = [&](const Vec3& pt) {
        minB.x = std::min(minB.x, pt.x); minB.y = std::min(minB.y, pt.y); minB.z = std::min(minB.z, pt.z);
        maxB.x = std::max(maxB.x, pt.x); maxB.y = std::max(maxB.y, pt.y); maxB.z = std::max(maxB.z, pt.z);
    };

    std::stringstream ss(paramText);
    std::string stmt;
    while (std::getline(ss, stmt, ';')) {
        std::stringstream ssStmt(stmt);
        std::string part;
        std::vector<std::string> parts;
        while (std::getline(ssStmt, part, ',')) {
            while (!part.empty() && (part.front() == ' ' || part.front() == '\t')) part.erase(0, 1);
            while (!part.empty() && (part.back() == ' ' || part.back() == '\t')) part.pop_back();
            if (!part.empty()) parts.push_back(part);
        }
        if (parts.empty()) continue;

        int entityType = 0;
        try { entityType = std::stoi(parts[0]); } catch (...) { continue; }

        if (entityType == 110 && parts.size() >= 7) { // Line
            try {
                Vec3 p1 = { (float)std::stod(parts[1]), (float)std::stod(parts[2]), (float)std::stod(parts[3]) };
                Vec3 p2 = { (float)std::stod(parts[4]), (float)std::stod(parts[5]), (float)std::stod(parts[6]) };
                segs.push_back({ p1, p2 });
                updateBBox(p1); updateBBox(p2);
            } catch (...) {}
        } else if (entityType == 100 && parts.size() >= 8) { // Circular Arc
            try {
                float zt = (float)std::stod(parts[1]);
                float xc = (float)std::stod(parts[2]), yc = (float)std::stod(parts[3]);
                float xs = (float)std::stod(parts[4]), ys = (float)std::stod(parts[5]);
                float xe = (float)std::stod(parts[6]), ye = (float)std::stod(parts[7]);

                float r = std::sqrt((xs - xc)*(xs - xc) + (ys - yc)*(ys - yc));
                float a1 = std::atan2(ys - yc, xs - xc);
                float a2 = std::atan2(ye - yc, xe - xc);
                if (a2 <= a1) a2 += 2.0f * 3.14159265f;

                const int S = 16;
                Vec3 prev = { xs, ys, zt };
                updateBBox(prev);
                for (int i = 1; i <= S; ++i) {
                    float a = a1 + (a2 - a1) * ((float)i / S);
                    Vec3 cur = { xc + r * std::cos(a), yc + r * std::sin(a), zt };
                    segs.push_back({ prev, cur });
                    updateBBox(cur);
                    prev = cur;
                }
            } catch (...) {}
        }
    }

    if (segs.empty()) return false;
    RibbonizeSegments(segs, tris, minB, maxB);
    return !tris.empty();
}

bool LoadBREP(const std::wstring& wpath, std::vector<Triangle>& tris, Vec3& minB, Vec3& maxB) {
    tris.clear();
    minB = {1e9f, 1e9f, 1e9f};
    maxB = {-1e9f, -1e9f, -1e9f};

    FILE* f = _wfopen(wpath.c_str(), L"r");
    if (!f) return false;

    std::vector<Vec3> verts;
    std::vector<LineSeg> segs;
    char line[256];
    bool inTShapes = false;
    int veCountDown = 0;

    auto updateBBox = [&](const Vec3& pt) {
        minB.x = std::min(minB.x, pt.x); minB.y = std::min(minB.y, pt.y); minB.z = std::min(minB.z, pt.z);
        maxB.x = std::max(maxB.x, pt.x); maxB.y = std::max(maxB.y, pt.y); maxB.z = std::max(maxB.z, pt.z);
    };

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "TShapes", 7) == 0) { inTShapes = true; continue; }
        if (inTShapes) {
            if (strncmp(line, "Ve", 2) == 0) {
                veCountDown = 2; // tolerance, then coords
                continue;
            }
            if (veCountDown > 0) {
                veCountDown--;
                if (veCountDown == 0) {
                    float x, y, z;
                    if (sscanf(line, "%f %f %f", &x, &y, &z) == 3) {
                        Vec3 pt = {x, y, z};
                        verts.push_back(pt);
                        updateBBox(pt);
                    }
                }
            }
        }
    }
    fclose(f);

    if (verts.empty()) return false;

    for (size_t i = 0; i + 1 < verts.size(); ++i) {
        segs.push_back({ verts[i], verts[i+1] });
    }

    if (segs.empty()) return false;
    RibbonizeSegments(segs, tris, minB, maxB);
    return !tris.empty();
}

bool SaveSTL(const std::wstring& wpath, const std::vector<Triangle>& tris) {
    if (tris.empty()) return false;
    FILE* f = _wfopen(wpath.c_str(), L"wb");
    if (!f) return false;
    char header[80] = "ModelPeek Native High-Speed STL Exporter";
    fwrite(header, 1, 80, f);
    uint32_t count = (uint32_t)tris.size();
    fwrite(&count, 4, 1, f);
    for (const auto& t : tris) {
        fwrite(&t.normal, 4, 3, f);
        fwrite(&t.v[0], 4, 3, f);
        fwrite(&t.v[1], 4, 3, f);
        fwrite(&t.v[2], 4, 3, f);
        uint16_t attr = 0;
        fwrite(&attr, 2, 1, f);
    }
    fclose(f);
    return true;
}

int main() {
    std::vector<Triangle> tris;
    Vec3 minB, maxB;

    std::cout << "--- Testing Native C++ STEP Loader ---" << std::endl;
    if (LoadSTEP(L"sample_models\\test_flange.step", tris, minB, maxB)) {
        std::cout << "[PASS] test_flange.step parsed successfully! Triangles: " << tris.size() << std::endl;
        SaveSTL(L"test_flange_native.stl", tris);
    } else {
        std::cout << "[FAIL] test_flange.step failed!" << std::endl;
    }

    if (LoadSTEP(L"sample_models\\sample_bracket.step", tris, minB, maxB)) {
        std::cout << "[PASS] sample_bracket.step parsed successfully! Triangles: " << tris.size() << std::endl;
    } else {
        std::cout << "[FAIL] sample_bracket.step failed!" << std::endl;
    }

    if (LoadSTEP(L"sample_models2\\industrial_planetary_gear.step", tris, minB, maxB)) {
        std::cout << "[PASS] industrial_planetary_gear.step parsed successfully! Triangles: " << tris.size() << std::endl;
    } else {
        std::cout << "[FAIL] industrial_planetary_gear.step failed!" << std::endl;
    }

    std::cout << "\n--- Testing Native C++ IGES Loader ---" << std::endl;
    if (LoadIGES(L"sample_models\\sample_bracket.iges", tris, minB, maxB)) {
        std::cout << "[PASS] sample_bracket.iges parsed successfully! Triangles: " << tris.size() << std::endl;
        SaveSTL(L"sample_bracket_native.stl", tris);
    } else {
        std::cout << "[FAIL] sample_bracket.iges failed!" << std::endl;
    }

    if (LoadIGES(L"sample_models2\\aerodynamic_wing_profile.iges", tris, minB, maxB)) {
        std::cout << "[PASS] aerodynamic_wing_profile.iges parsed successfully! Triangles: " << tris.size() << std::endl;
    } else {
        std::cout << "[FAIL] aerodynamic_wing_profile.iges failed!" << std::endl;
    }

    std::cout << "\n--- Testing Native C++ BREP Loader ---" << std::endl;
    if (LoadBREP(L"sample_models\\sample_bracket.brep", tris, minB, maxB)) {
        std::cout << "[PASS] sample_bracket.brep parsed successfully! Triangles: " << tris.size() << std::endl;
        SaveSTL(L"sample_bracket_brep_native.stl", tris);
    } else {
        std::cout << "[FAIL] sample_bracket.brep failed!" << std::endl;
    }

    if (LoadBREP(L"sample_models2\\robotic_gripper_finger.brep", tris, minB, maxB)) {
        std::cout << "[PASS] robotic_gripper_finger.brep parsed successfully! Triangles: " << tris.size() << std::endl;
    } else {
        std::cout << "[FAIL] robotic_gripper_finger.brep failed!" << std::endl;
    }

    return 0;
}
