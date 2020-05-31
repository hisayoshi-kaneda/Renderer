#pragma once

#ifndef MESH_VIEWER
#define MESH_VIEWER

#include "opengl-wrapper/FrameBufferObject.h"
#include "opengl-wrapper/Shader.h"
#include "TriMesh.h"
#include "TriMeshLoader.h"
#include "opengl-wrapper/VertexArrayObjectForMesh.h"
#include "opengl-wrapper/Window.h"
#include "core/common.h"
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "opengl-wrapper/XYZ_Axis.h"
#include "stb/stb_image_write.h"

class MeshViewer {
private:
	shared_ptr<Window> window;
	VertexArrayObjectForMesh vao;
	FrameBufferObject fbo;
	Shader normal_shader;
	Shader gooch_shader;
	Shader sinogram_shader;
	Shader crossSection2D_shader;
	Shader crossSection3D_shader;
	Shader texture_shader;
	shared_ptr<TriMesh> mesh;
	string normal_vert_file = "mesh_render.vert";
	string normal_frag_file = "mesh_render.frag";
	string gooch_frag_file = "mesh_render_gooch.frag";
	string sinogram_vert_file = "mesh_sinogram_render.vert";
	string sinogram_frag_file = "mesh_sinogram_render.frag";
	string texture_vert_file = "texture_render.vert";
	string texture_frag_file = "texture_render.frag";
	string crossSection2D_vert_file = "crossSection2D_render.vert";
	string crossSection2D_frag_file = "crossSection2D_render.frag";
	string crossSection3D_vert_file = "crossSection3D_render.vert";
	string crossSection3D_frag_file = "crossSection3D_render.frag";
	glm::vec3 cameraPos;
	glm::vec3 upVector;
	glm::vec3 cameraTarget;

	Texture2D crossSection;

	XYZ_Axis xyzAxis;
	float lightPower = 0.0f;
	inline static char dir[128] = {};

	ShadingMethod shadingMethod = FLAT_SHADING;
	enum RenderingMode {
		RENDER_NORMAL, RENDER_GOOCH, RENDER_SINOGRAM,
	};
	RenderingMode renderingMode = RENDER_NORMAL;

public:
	MeshViewer(shared_ptr<TriMesh>& mesh, shared_ptr<Window>& window)
		: window(window),
		crossSection(window->width, window->height, GL_SRGB, GL_RGBA),
		mesh(mesh),
		vao(mesh), xyzAxis(window) {
		initialize();
	}

	virtual ~MeshViewer() {
	}

	void initialize() {
		const char* glsl_version = "#version 330";
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(window->window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		fbo.setViewport(window->width, window->height);
		normal_shader.create(normal_vert_file, normal_frag_file);
		gooch_shader.create(normal_vert_file, gooch_frag_file);
		sinogram_shader.create(sinogram_vert_file, sinogram_frag_file);
		texture_shader.create(texture_vert_file, texture_frag_file);
		crossSection2D_shader.create(crossSection2D_vert_file, crossSection2D_frag_file);
		crossSection3D_shader.create(crossSection3D_vert_file, crossSection3D_frag_file);
		mesh->computeAABB();
		window->gravity = mesh->centerAABB;
		cout << window->gravity[0] << " " << window->gravity[1] << " " << window->gravity[2] << endl;
		cameraPos = glm::vec3((mesh->maxPointAABB.x - mesh->minPointAABB.x) * 2.0f, 0.0f, 0.0f);
		cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		upVector = glm::vec3(0.0f, 0.0f, 1.0f);
		window->viewMat = glm::lookAt(cameraPos,    // 視点の位置
			cameraTarget, // 見ている先
			upVector);    // 視界の上方向
		window->modelMat = glm::translate(-mesh->centerAABB);
		glm::vec3 temp = mesh->maxPointAABB - mesh->minPointAABB;
		float aabbMaxSize = max(temp.x, max(temp.y, temp.z));
		float K = aabbMaxSize / 10.0f;
		lightPower = 100.0f * K * K / 2.0f;
	}

	void main_loop() {
		while (*window) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			draw();
		}
	}

	void draw() {
		xyzAxis.draw();
		if (renderingMode == RENDER_SINOGRAM) {
			renderSinogram();
		}
		else {
			renderSolid();
		}

		GUI_Component();
	}

	void renderSolid() {
		glm::vec3 temp = mesh->maxPointAABB - mesh->minPointAABB;
		float aabbMaxSize = max(temp.x, max(temp.y, temp.z));
		Shader* shader = &normal_shader;
		if (renderingMode == RENDER_GOOCH) {
			shader = &gooch_shader;
		}
		shader->bind();
		shader->set_uniform_value(window->mvpMat(), "u_mvpMat");
		shader->set_uniform_value(window->modelMat, "u_modelMat");
		shader->set_uniform_value(window->viewMat, "u_viewMat");
		shader->set_uniform_value(aabbMaxSize, "u_aabbMaxSize");
		shader->set_uniform_value(lightPower, "u_lightPower");
		shader->set_uniform_value(glm::vec3(1.0, 1.0, 0.0), "u_materialColor");
		vao.draw(shadingMethod);
		shader->release();
	}

	float magnitude = 1.0f;
	void renderSinogram() {
		Texture2D colorBuffer(window->width, window->height, GL_RGB32F, GL_RGBA);
		Texture2D depthBuffer(window->width, window->height, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT);
		// FBOへの描画
		sinogram_shader.bind();
		{
			fbo.setViewport(window->width, window->height);
			fbo.bind();
			fbo.attachColorTexture(colorBuffer);
			fbo.attachDepthTexture(depthBuffer);
			vao.bind();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Uniform変数の転送
			glm::mat4 normMat = window->mvMat();
			sinogram_shader.set_uniform_value(window->mvpMat(), "u_mvpMat");
			sinogram_shader.set_uniform_value(window->mvMat(), "u_mvMat");
			sinogram_shader.set_uniform_value(normMat, "u_normMat");

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			vao.draw(FLAT_SHADING);

			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			fbo.release();
		}
		sinogram_shader.release();

		// ウィンドウへの描画
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		texture_shader.bind();

		colorBuffer.bind();
		glUniform1i(glGetUniformLocation(texture_shader.program_id, "sinogram"), 0);
		glUniform1f(glGetUniformLocation(texture_shader.program_id, "u_magnitude"), magnitude);

		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		vao.release();

		texture_shader.release();
		colorBuffer.release();

	}

	enum AXIS {
		X, Y, Z
	};

	AXIS crossSectionAXIS = Z;
	float slicePos = 0;
	void renderCrossSections(AXIS axis) {
		float size;
		glm::vec3 positions[4];
		glm::vec3 fragColor;
		if (axis == X) {
			cameraPos = glm::vec3(slicePos - 0.1f, 0.0f, 0.0f);
			cameraTarget = glm::vec3(mesh->maxPointAABB.x - mesh->minPointAABB.x, 0.0f, 0.0f);
			upVector = glm::vec3(0.0f, 0.0f, -1.0f);
			size = max(mesh->maxPointAABB.y - mesh->minPointAABB.y, mesh->maxPointAABB.z - mesh->minPointAABB.z);
			float m_slicePos = slicePos + mesh->centerAABB.x;
			positions[0] = glm::vec3(m_slicePos, mesh->minPointAABB.y, mesh->minPointAABB.z);
			positions[1] = glm::vec3(m_slicePos, mesh->minPointAABB.y, mesh->maxPointAABB.z);
			positions[2] = glm::vec3(m_slicePos, mesh->maxPointAABB.y, mesh->minPointAABB.z);
			positions[3] = glm::vec3(m_slicePos, mesh->maxPointAABB.y, mesh->maxPointAABB.z);
			fragColor = glm::vec3(1.0, 0.0, 0.0);
		}
		else if (axis == Y) {
			cameraPos = glm::vec3(0.0f, slicePos - 0.1f, 0.0f);
			cameraTarget = glm::vec3(0.0f, mesh->maxPointAABB.y - mesh->minPointAABB.y, 0.0f);
			upVector = glm::vec3(-1.0f, 0.0f, 0.0f);
			size = max(mesh->maxPointAABB.x - mesh->minPointAABB.x, mesh->maxPointAABB.z - mesh->minPointAABB.z);
			float m_slicePos = slicePos + mesh->centerAABB.y;
			positions[0] = glm::vec3(mesh->minPointAABB.x, m_slicePos, mesh->minPointAABB.z);
			positions[1] = glm::vec3(mesh->maxPointAABB.x, m_slicePos, mesh->minPointAABB.z);
			positions[2] = glm::vec3(mesh->minPointAABB.x, m_slicePos, mesh->maxPointAABB.z);
			positions[3] = glm::vec3(mesh->maxPointAABB.x, m_slicePos, mesh->maxPointAABB.z);
			fragColor = glm::vec3(0.0, 1.0, 0.0);
		}
		else {
			cameraPos = glm::vec3(0.0f, 0.0f, slicePos - 0.1f);
			cameraTarget = glm::vec3(0.0f, 0.0f, mesh->maxPointAABB.z - mesh->minPointAABB.z);
			upVector = glm::vec3(0.0f, -1.0f, 0.0f);
			size = max(mesh->maxPointAABB.x - mesh->minPointAABB.x, mesh->maxPointAABB.y - mesh->minPointAABB.y);
			float m_slicePos = slicePos + mesh->centerAABB.z;
			positions[0] = glm::vec3(mesh->minPointAABB.x, mesh->minPointAABB.y, m_slicePos);
			positions[1] = glm::vec3(mesh->minPointAABB.x, mesh->maxPointAABB.y, m_slicePos);
			positions[2] = glm::vec3(mesh->maxPointAABB.x, mesh->minPointAABB.y, m_slicePos);
			positions[3] = glm::vec3(mesh->maxPointAABB.x, mesh->maxPointAABB.y, m_slicePos);
			fragColor = glm::vec3(0.0, 0.0, 1.0);
		}

		{
			Texture2D depthBuffer(window->width, window->height, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT);
			fbo.setViewport(window->width, window->height);
			fbo.bind();
			fbo.attachColorTexture(crossSection);
			fbo.attachDepthTexture(depthBuffer);

			glLogicOp(GL_INVERT);
			glDisable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_COLOR_LOGIC_OP);
			crossSection2D_shader.bind();

			glm::mat4 viewMat = glm::lookAt(cameraPos, cameraTarget, upVector);
			glm::mat4 projMat = glm::ortho(-size / 2.0f, size / 2.0f, -size / 2.0f, size / 2.0f, 0.1f, 1000.0f);
			crossSection2D_shader.set_uniform_value(projMat * viewMat * window->modelMat, "u_mvpMat");
			vao.draw(shadingMethod);
			crossSection2D_shader.release();
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_COLOR_LOGIC_OP);
			fbo.release();
		}

		{
			vao.bind();
			crossSection3D_shader.bind();
			crossSection3D_shader.set_uniform_value(window->mvpMat(), "u_mvpMat");
			crossSection3D_shader.set_uniform_value(positions, 4, "u_positions");
			crossSection3D_shader.set_uniform_value(fragColor, "u_fragColor");
			crossSection3D_shader.set_uniform_value(mesh->minPointAABB, "u_minPoint");
			crossSection3D_shader.set_uniform_value(mesh->maxPointAABB, "u_maxPoint");
			glDrawArrays(GL_TRIANGLES, 0, 6);
			crossSection3D_shader.release();
			vao.release();
		}

	}

	void capture(const char* dir, const char* name) {
		int pixelN = window->width * window->height;
		unsigned char* pixels = new unsigned char[pixelN * 3];
		glReadPixels(0, 0, window->width, window->height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
#pragma omp parallel for
		for (int i = 0; i < window->height / 2; i++) {
			for (int j = 0; j < window->width; j++) {
				swap(pixels[(i * window->width + j) * 3 + 0], pixels[((window->height - 1 - i) * window->width + j) * 3 + 0]);
				swap(pixels[(i * window->width + j) * 3 + 1], pixels[((window->height - 1 - i) * window->width + j) * 3 + 1]);
				swap(pixels[(i * window->width + j) * 3 + 2], pixels[((window->height - 1 - i) * window->width + j) * 3 + 2]);
			}
		}
		string str_dir = dir;
		if (str_dir == "") {
			str_dir = ".";
		}
		if (str_dir.back() != '/') {
			str_dir += '/';
		}
		if (name == "") {
			name = "render";
		}
		string path = str_dir + name + ".png";
		int hasSaved = stbi_write_png(path.c_str(), window->width, window->height, 3, pixels, 0);
		if (!hasSaved) {
			fprintf(stderr, "\"%s\" is invalid directory. Please try again.\n", dir);
		}
		delete[] pixels;
	}

	bool show_cross_sections = false;
	void GUI_Component() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("option"); // Create a window called "Hello, world!" and append into it.
		ImGui::Text("Please select projection mode");
		if (ImGui::RadioButton("perspective", window->projectionMode == PERSPECTIVE)) {
			window->projectionMode = PERSPECTIVE;
			window->resize(window->width, window->height);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("ortho", window->projectionMode == ORTHO)) {
			window->projectionMode = ORTHO;
			window->resize(window->width, window->height);
		}

		ImGui::Text("Please select shading method."); // Display some text (you can use a format strings too)
		if (ImGui::RadioButton("flat", shadingMethod == FLAT_SHADING)) {
			shadingMethod = FLAT_SHADING;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("smooth", shadingMethod == SMOOTH_SHADING)) {
			shadingMethod = SMOOTH_SHADING;
		}
		ImGui::Text("Please select rendering mode."); // Display some text (you can use a format strings too)
		glm::vec3 temp = mesh->maxPointAABB - mesh->minPointAABB;
		float aabbMaxSize = max(temp.x, max(temp.y, temp.z));
		float K = aabbMaxSize / 10.0f;
		if (ImGui::RadioButton("normal", renderingMode == RENDER_NORMAL)) {
			renderingMode = RENDER_NORMAL;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("gooch", renderingMode == RENDER_GOOCH)) {
			renderingMode = RENDER_GOOCH;
		}
		ImGui::SliderFloat("light power", &lightPower, 0.0f, 100.0f * K * K);
		if (ImGui::RadioButton("sinogram", renderingMode == RENDER_SINOGRAM)) {
			renderingMode = RENDER_SINOGRAM;
		}
		ImGui::SliderFloat("transmission length magnitude", &magnitude, 0.0f, 1.0f);
		ImGui::Checkbox("Cross Section Window", &show_cross_sections);
		ImGui::InputText("dir", dir, 128);
		static char name[128] = "render"; ImGui::InputText("name", name, 128);
		if (ImGui::Button("capture")) {
			capture(dir, name);
		}
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		if (show_cross_sections) {
			ImGui::Begin("cross sections", &show_cross_sections);
			renderCrossSections(crossSectionAXIS);
			ImGui::Image((void*)(intptr_t)crossSection.textureId, ImVec2(300, 300));
			float sliceWidth = mesh->maxPointAABB[crossSectionAXIS] - mesh->minPointAABB[crossSectionAXIS];
			ImGui::SliderFloat("slice position", &slicePos, -sliceWidth / 2.0f, sliceWidth / 2.0f);
			if (ImGui::RadioButton("X", crossSectionAXIS == X)) {
				crossSectionAXIS = X;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Y", crossSectionAXIS == Y)) {
				crossSectionAXIS = Y;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Z", crossSectionAXIS == Z)) {
				crossSectionAXIS = Z;
			}
			ImGui::End();
		}
		// Rendering
		ImGui::Render();
		window->isAnyImguiWindowHovered = ImGui::IsAnyWindowHovered();
		int display_w, display_h;
		glfwGetFramebufferSize(window->window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
};

#endif //MESH_VIEWER