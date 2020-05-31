#pragma once

#ifndef PATH_TRACER
#define PATH_TRACER

#include "opengl-wrapper/FrameBufferObject.h"
#include "opengl-wrapper/Shader.h"
#include "opengl-wrapper/VertexArrayObjectForMesh.h"
#include "opengl-wrapper/Window.h"
#include "core/common.h"
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "opengl-wrapper/XYZ_Axis.h"
#include "stb/stb_image_write.h"

class PathTracer {
private:
	shared_ptr<Window> window;
	VertexArrayObject vao;
	FrameBufferObject fbo;
	Shader normal_shader;
	Shader texture_shader;
	string normal_vert_file = "render.vert";
	string normal_frag_file = "render.frag";
	string texture_vert_file = "texture_render.vert";
	string texture_frag_file = "texture_render.frag";
	Texture2D colorBuffer;

public:
	PathTracer(shared_ptr<Window>& window)
		: window(window),
		colorBuffer(window->width, window->height, GL_RGB32F, GL_RGBA) {
		initialize();
	}

	virtual ~PathTracer() {
	}

	void initialize() {
		//const char* glsl_version = "#version 330";
		//IMGUI_CHECKVERSION();
		//ImGui::CreateContext();
		//ImGuiIO& io = ImGui::GetIO();
		//(void)io;

		//ImGui::StyleColorsDark();

		//ImGui_ImplGlfw_InitForOpenGL(window->window, true);
		//ImGui_ImplOpenGL3_Init(glsl_version);
		fbo.setViewport(window->width, window->height);
		normal_shader.create(normal_vert_file, normal_frag_file);
		texture_shader.create(texture_vert_file, texture_frag_file);
	}

	void render() {
		int frame = 0;
		int numSamples = 1000;
		while (*window) {
			frame++;
			cout << frame << endl;
			if (frame < numSamples) {
				normal_shader.bind();
				{
					fbo.setViewport(window->width, window->height);
					fbo.bind();
					fbo.attachColorTexture(colorBuffer);
					// Uniform•Ï”‚Ì“]‘—
					normal_shader.set_uniform_value(glm::vec2(float(window->width), float(window->height)), "resolution");
					normal_shader.set_uniform_value(frame, "frame");
					normal_shader.set_uniform_value(numSamples, "numSamples");

					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE);

					vao.draw(GL_TRIANGLES, 6);

					glEnable(GL_DEPTH_TEST);
					glDisable(GL_BLEND);

					fbo.release();
				}
				normal_shader.release();
			}

			// ƒEƒBƒ“ƒhƒE‚Ö‚Ì•`‰æ
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			texture_shader.bind();

			colorBuffer.bind();
			glUniform1i(glGetUniformLocation(texture_shader.program_id, "texImage"), 0);

			vao.draw(GL_TRIANGLES, 6);

			texture_shader.release();
			colorBuffer.release();
		}
	}
};

#endif //MESH_VIEWER