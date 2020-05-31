#pragma once

#ifndef XYZAXIS
#define XYZAXIS

#include"Window.h"
#include"VertexArrayObject.h"
#include"Shader.h"
#include"core/common.h"

class XYZ_Axis {
public:
	shared_ptr<Window> window;
	VertexArrayObject vao;
	Shader shader;
	string vert_file = "xyz_axis.vert";
	string frag_file = "xyz_axis.frag";

	XYZ_Axis(shared_ptr<Window> window) :window(window) {
		initialize();
	}

	void initialize() {
		shader.create(vert_file, frag_file);
	}

	void draw() {
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window->height / 5.0, window->height / 5.0);
		shader.bind();
		shader.set_uniform_value(window->mvpMat(), "u_mvpMat");
		glLineWidth(10.0);
		vao.draw(GL_LINES, 6);
		shader.release();
		glViewport(0, 0, window->width, window->height);
	}
};

#endif