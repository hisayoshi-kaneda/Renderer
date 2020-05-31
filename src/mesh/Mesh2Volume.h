#pragma once

#ifndef MESH2VOLUME
#define MESH2VOLUME

#include "opengl-wrapper/FrameBufferObject.h"
#include "opengl-wrapper/Shader.h"
#include "opengl-wrapper/Texture2D.h"
#include "opengl-wrapper/Texture2DArray.h"
#include "core/Timer.h"
#include "TriMesh.h"
#include "TriMeshLoader.h"
#include "opengl-wrapper/VertexArrayObjectForMesh.h"
#include "volume/Volume.h"
#include "opengl-wrapper/Window.h"
#include "core/common.h"

class Mesh2Volume {
private:
	shared_ptr<Window> window;
	shared_ptr<TriMesh> mesh;
	FrameBufferObject fbo;
	Shader crossSection_shader;
	string crossSection_vert_file = "crossSection2D_render.vert";
	string crossSection_frag_file = "crossSection2D_render.frag";

	glm::mat4 projMat, viewMat, modelMat;
	glm::vec3 center;
	glm::vec3 size;
	float resolution;
	float zFar;
	float zNear;

public:
	Mesh2Volume(const int sizeX, const int sizeY, const int sizeZ, float resolution, shared_ptr<TriMesh> mesh, shared_ptr<Window> window)
		: window(window),
		mesh(mesh),
		size{ sizeX, sizeY, sizeZ },
		resolution(resolution) {
		initialize();
	}

	virtual ~Mesh2Volume() {
	}

	void initialize() {
		crossSection_shader.create(crossSection_vert_file, crossSection_frag_file);
		mesh->computeAABB();
		center = mesh->centerAABB;
		zFar = max(size[0], max(size[1], size[2])) * resolution + 10.0f;
		zNear = 0.1f;
	}

	Volume generateVolume() {
		Timer timer;
		timer.start();

		Volume volume((int)size[0], (int)size[1], (int)size[2], resolution, 0.5f);
		VertexArrayObjectForMesh vao(mesh);

		projMat = glm::ortho(-size[0] * resolution / 2.0f, size[0] * resolution / 2.0f, -size[1] * resolution / 2.0f, size[1] * resolution / 2.0f, zNear, zFar);
		modelMat = glm::translate(-center);
		fbo.setViewport(int(size[0]), int(size[1]));
		fbo.bind();

		Texture2D colorBuffer(size[0], size[1], GL_RGBA, GL_RGBA);
		Texture2D depthBuffer(size[0], size[1], GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT);
		fbo.attachColorTexture(colorBuffer);
		fbo.attachDepthTexture(depthBuffer);

		float orgZ = -(size[2] - 1.0f) / 2.0f * resolution;
		crossSection_shader.bind();
		unsigned char* buf = new unsigned char[volume.size[0] * volume.size[1]];
		for (int z = 0; z < int(size[2]); z++) {
			float coordZ = orgZ + z * resolution;
			viewMat = glm::lookAt(glm::vec3(0.0f, 0.0f, coordZ), glm::vec3(0.0, 0.0, -size[2] * resolution), glm::vec3(0.0f, 1.0f, 0.0f));
			glLogicOp(GL_INVERT);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_COLOR_LOGIC_OP);
			crossSection_shader.bind();
			crossSection_shader.set_uniform_value(projMat * viewMat * modelMat, "u_mvpMat");
			vao.draw(FLAT_SHADING);
			crossSection_shader.release();
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_COLOR_LOGIC_OP);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(0, 0, size[0], size[1], GL_RED, GL_UNSIGNED_BYTE, buf);
#pragma omp parallel for
			for (int i = 0; i < int(size[0]) * int(size[1]); i++) {
				if (buf[i] > 0)buf[i] = 1;
				volume.data[int(size[0]) * int(size[1]) * z + i] = buf[i];
			}
		}
		crossSection_shader.release();

		fbo.release();

		cout << "Generating volume took " << timer.stop() << " sec" << endl;

		return volume;
	}
};

#endif //MESH_VIEWER