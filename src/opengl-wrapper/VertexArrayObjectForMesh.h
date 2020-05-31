#pragma once

#ifndef VAOMESH_H
#define VAOMESH_H

#include "mesh/Trimesh.h"
#include "core/common.h"

enum ShadingMethod {
    FLAT_SHADING,
    SMOOTH_SHADING,
    SINOGRAM,
};

class VertexArrayObjectForMesh {
    GLuint vaoId = 0;
    GLuint flatVertexBufferId = 0;
    GLuint flatNormalBufferId = 0;
    GLuint smoothVertexBufferId = 0;
    GLuint smoothNormalBufferId = 0;
    GLuint indexBufferId = 0;
    shared_ptr<TriMesh> mesh;

public:
    VertexArrayObjectForMesh(shared_ptr<TriMesh> mesh) : mesh(mesh) {
        initialize();
    }

    void initialize() {
        mesh->computeVerNormals();
        mesh->computeFaceNormals();
        // VAO�̍쐬
        glGenVertexArrays(1, &vaoId);
        glBindVertexArray(vaoId);

        // ���_�o�b�t�@�̍쐬
        glGenBuffers(1, &smoothVertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, smoothVertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * mesh->vertices.size(), mesh->vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &smoothNormalBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, smoothNormalBufferId);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * mesh->verNormals.size(), mesh->verNormals.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->verIndices.size(), mesh->verIndices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &flatVertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, flatVertexBufferId);
        vector<glm::vec3> buffer(mesh->faceN * 3);
        for (int i = 0; i < mesh->faceN; i++) {
            buffer[3 * i + 0] = mesh->vertices[mesh->verIndices[3 * i + 0]];
            buffer[3 * i + 1] = mesh->vertices[mesh->verIndices[3 * i + 1]];
            buffer[3 * i + 2] = mesh->vertices[mesh->verIndices[3 * i + 2]];
        }
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * buffer.size(), buffer.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &flatNormalBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, flatNormalBufferId);
        for (int i = 0; i < mesh->faceN; i++) {
            buffer[3 * i + 0] = mesh->faceNormals[i];
            buffer[3 * i + 1] = mesh->faceNormals[i];
            buffer[3 * i + 2] = mesh->faceNormals[i];
        }
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * buffer.size(), buffer.data(), GL_STATIC_DRAW);

        // VAO��OFF�ɂ��Ă���
        glBindVertexArray(0);
    }

    void draw(ShadingMethod method) {
        if (method == SMOOTH_SHADING) {
            bind();
            glBindBuffer(GL_ARRAY_BUFFER, smoothVertexBufferId);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

            glBindBuffer(GL_ARRAY_BUFFER, smoothNormalBufferId);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);

            glDrawElements(GL_TRIANGLES, mesh->verIndices.size(), GL_UNSIGNED_INT, 0);
            release();
        } else if (method == FLAT_SHADING) {
            bind();
            glBindBuffer(GL_ARRAY_BUFFER, flatVertexBufferId);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

            glBindBuffer(GL_ARRAY_BUFFER, flatNormalBufferId);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

            glDrawArrays(GL_TRIANGLES, 0, mesh->faceN * 3);
            release();
        }
    }

    void bind() {
        glBindVertexArray(vaoId);
    }

    void release() {
        glBindVertexArray(0);
    }
};

#endif //VAO_H