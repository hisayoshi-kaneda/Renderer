#pragma once

#ifndef VAO_H
#define VAO_H

#include "mesh/TriMesh.h"
#include "core/common.h"

struct Vertex {
	Vertex(const glm::vec3& position_, const glm::vec3& color_)
		: position(position_), color(color_) {
	}

	glm::vec3 position;
	glm::vec3 color;
};


class VertexArrayObject {
	int size = 0;
	vector<GLuint> vbo_ids;
	GLuint ibo_id = 0;
	GLuint vao_id = 0;
public:
	VertexArrayObject() {
		glGenVertexArrays(1, &vao_id);
		glBindVertexArray(vao_id);
		glBindVertexArray(0);
	}

	template<typename vboType, typename iboType>
	void createBuffers(vector<vboType*> vboDataPointerArray, int vboDataSize, iboType* iboData = nullptr, int iboDataSize = 0) {
		bind();
		GLuint vbo_id = 0;
		size = vboDataSize;

		for (int i = 0; i < (int)vboDataPointerArray.size(); i++) {
			glGenBuffers(1, &vbo_id);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vboType) * vboDataSize, vboDataPointerArray[i], GL_STATIC_DRAW);

			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, 0);
			vbo_ids.push_back(vbo_id);
		}

		if (iboData != nullptr) {
			glGenBuffers(1, &ibo_id);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(iboType) * iboDataSize, iboData, GL_STATIC_DRAW);
			size = iboDataSize;
		}
		release();
	}

	void draw(GLenum mode) {
		bind();
		//glDrawArrays(mode, 0, size);
		glDrawElements(mode, size, GL_UNSIGNED_INT, 0);
		release();
	}
	void draw(GLenum mode, int _size) {
		bind();
		glDrawArrays(mode, 0, _size);
		release();
	}

	template<typename vboType>
	void setBuffer(int vboIndex, vboType* data, int dataSize) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[vboIndex]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vboType) * dataSize, data);
	}

	void bind() {
		glBindVertexArray(vao_id);
	}

	void release() {
		glBindVertexArray(0);
	}
};

#endif //VAO_H