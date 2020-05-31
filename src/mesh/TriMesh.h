#pragma once

#ifndef TRI_MESH
#define TRI_MESH

#include "core/common.h"
#include "tinyply.h"

namespace std {
	template <>
	struct hash<glm::vec3> {
		size_t operator()(const glm::vec3& v) const {
			size_t seed = 0;
			auto x_hash = hash<float>()(v.x);
			auto y_hash = hash<float>()(v.y);
			auto z_hash = hash<float>()(v.z);

			seed ^= x_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= y_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= z_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
} // namespace std

class TriMesh {
public:
    unsigned int faceN = 0;
    unsigned int verN = 0;
    unsigned int uvN = 0;
    unsigned int uvfaceN = 0;
    vector<glm::vec3> vertices;
    vector<unsigned int> verIndices;
    vector<glm::vec3> verNormals;
    vector<glm::vec3> faceNormals;
    vector<glm::vec3> faceCenters;
    vector<glm::vec2> uvs;
    vector<unsigned int> uvIndices;
    glm::vec3 gravity = {0.0f, 0.0f, 0.0f};

    glm::vec3 minPointAABB = {FLT_MAX, FLT_MAX, FLT_MAX};
    glm::vec3 maxPointAABB = {FLT_MIN, FLT_MIN, FLT_MIN};
    glm::vec3 centerAABB;

	string filename;

    TriMesh() = default;

	virtual ~TriMesh() {}

    void addVertex(glm::vec3 vertex) {
        vertices.push_back(vertex);
        verN++;
    }

    void addFace(unsigned int index[3]) {
        verIndices.push_back(index[0]);
        verIndices.push_back(index[1]);
        verIndices.push_back(index[2]);
        faceN++;
    }

	void addTriangle(glm::vec3 vertexes[3], unsigned int index[3]) {
		addVertex(vertexes[0]);
		addVertex(vertexes[1]);
		addVertex(vertexes[2]);
		addFace(index);
	}

    void adduv(glm::vec2 uv) {
        uvs.push_back(uv);
        uvN++;
    }

    void adduvFace(unsigned int index[3]) {
        uvIndices.push_back(index[0]);
        uvIndices.push_back(index[1]);
        uvIndices.push_back(index[2]);
        uvfaceN++;
    }

    void computeFaceNormals() {
        faceNormals.clear();
        faceNormals.resize(faceN);
#pragma omp parallel for
        for (int i = 0; i < (int)faceN; i++) {
            unsigned int index[3] = {verIndices[3 * i + 0], verIndices[3 * i + 1], verIndices[3 * i + 2]};
            glm::vec3 v1 = vertices[index[1]] - vertices[index[0]];
            glm::vec3 v2 = vertices[index[2]] - vertices[index[1]];
            faceNormals[i] = normalize(cross(v1, v2));
        }
    }

    void computeVerNormals() {
        verNormals.clear();
        verNormals.resize(verN, glm::vec3(0.0f));
        //#pragma omp parallel for
        for (int i = 0; i < (int)faceN; i++) {
            unsigned int index[3] = {verIndices[3 * i + 0], verIndices[3 * i + 1], verIndices[3 * i + 2]};
            glm::vec3 v1 = vertices[index[1]] - vertices[index[0]];
            glm::vec3 v2 = vertices[index[2]] - vertices[index[1]];
            verNormals[index[0]] += cross(v1, v2);
            verNormals[index[1]] += cross(v1, v2);
            verNormals[index[2]] += cross(v1, v2);
        }
        //#pragma omp parallel for
        for (int i = 0; i < (int)verN; i++) {
            verNormals[i] = normalize(verNormals[i]);
        }
    }

    void computeFaceCenters() {
        faceCenters.clear();
        faceCenters.resize(faceN);
#pragma omp parallel for
        for (int i = 0; i < faceN; i++) {
            unsigned int index[3] = {verIndices[3 * i + 0], verIndices[3 * i + 1], verIndices[3 * i + 2]};
            faceCenters[i] = (vertices[index[0]] + vertices[index[1]] + vertices[index[2]]) / 3.0f;
        }
    }

    void computeGravity() {
        for (int i = 0; i < verN; i++) {
            gravity += vertices[i] / (float)verN;
        }
    }

    void computeAABB() {
        for (int i = 0; i < verN; i++) {
            for (int j = 0; j < 3; j++) {
                minPointAABB[j] = min(vertices[i][j], minPointAABB[j]);
                maxPointAABB[j] = max(vertices[i][j], maxPointAABB[j]);
            }
        }
        centerAABB = (minPointAABB + maxPointAABB) / 2.0f;
    }

	void unifyDuprecatedVertices() {
		unordered_map<glm::vec3, int> mp;
		int tmpVerN = 0;
		for (int i = 0; i < verN; i++) {
			bool exists = mp.emplace(vertices[i], tmpVerN).second;
			if (exists)
				tmpVerN++;
		}
		verN = tmpVerN;
#pragma omp parallel for
		for (int i = 0; i < faceN; i++) {
			verIndices[3 * i + 0] = mp[vertices[verIndices[3 * i + 0]]];
			verIndices[3 * i + 1] = mp[vertices[verIndices[3 * i + 1]]];
			verIndices[3 * i + 2] = mp[vertices[verIndices[3 * i + 2]]];
		}
		vertices.clear();
		vertices.resize(mp.size());

		for (auto& elem : mp) {
			vertices[elem.second] = elem.first;
		}
	}

	void writePly(const std::string& filename)
	{
		using namespace tinyply;
		std::filebuf fb_binary;
		fb_binary.open(filename + ".ply", std::ios::out | std::ios::binary);
		std::ostream outstream_binary(&fb_binary);
		if (outstream_binary.fail()) throw std::runtime_error("failed to open " + filename);

		PlyFile file;

		file.add_properties_to_element("vertex", { "x", "y", "z" },
			Type::FLOAT32, verN, reinterpret_cast<uint8_t*>(vertices.data()), Type::INVALID, 0);

		file.add_properties_to_element("face", { "vertex_indices" },
			Type::UINT32, faceN, reinterpret_cast<uint8_t*>(verIndices.data()), Type::UINT8, 3);

		file.get_comments().push_back("generated by tinyply 2.2");

		// Write a binary file
		file.write(outstream_binary, true);
	}
};

#endif //TRI_MESH