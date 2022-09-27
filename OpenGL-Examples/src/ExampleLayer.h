#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

#include "stb_image.h"

class ExampleLayer : public GLCore::Layer
{
public:
	struct Vertex{
		glm::vec2 position;
		glm::vec2 texture;

		Vertex(glm::vec2 pos, glm::vec2 tex)
			: position(pos), texture(tex){};

		Vertex(glm::vec2 pos)
			: position(pos), texture(1.0f, 1.0f){};
	};

	enum class Shape{
		Curve,
		Surface
	};

	ExampleLayer();
	virtual ~ExampleLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	
private:

	GLCore::Utils::Shader* m_Shader;
	GLCore::Utils::OrthographicCameraController m_CameraController;

	int nCurvePoints = 4;
	std::vector<glm::vec2> curveControlPoints;
	std::vector<std::vector<glm::vec2>> surfaceControlPoints;
	
	GLuint m_QuadVA, m_QuadVB, m_QuadIB;

	unsigned int texture;

	int pointsCount = 0;
	int linesCount = 0;
	int trianglesCount = 0;
	int quadsCount = 0;
	std::string buffer;

	float m_Step = 0.05f;
	int currentPoint = 0;
	glm::ivec2 currentCoord = {0, 0};
	int m_MaxDepth = 2;
	float m_Threshold = 0.1f;

	float fps = 0;

	bool calculate = false;
	bool controlPoints = true;
	bool tessellate = true;
	bool wireframe = false;
	bool textured = true;
	
	Shape currentShape = Shape::Curve;
private:
	glm::vec2 EvaluateCurve(float t, std::vector<glm::vec2>& points);
	glm::vec2 EvaluateSurface(float s, float t, std::vector<std::vector<glm::vec2>>& points);
	
	void SwitchShape();

	void DrawLine(glm::vec2& p1, glm::vec2& p2);
	void DrawPoint(glm::vec2 p, float size = 5);
	void DrawQuad(std::vector<glm::vec2>& points);
	void DrawQuad(std::vector<std::vector<glm::vec2>>& quad);
	void DrawQuad(std::vector<glm::vec2>& points, std::vector<glm::vec2>& textures);
	void DrawQuad(std::vector<std::vector<glm::vec2>>& quad, std::vector<std::vector<glm::vec2>>&& textures);

	void DrawCurveControlLines();
	void DrawSurfaceControlLines();
	void DrawCurve(float step);
	void DrawSurface(float step);
	void TessellateCurve(float t0, float t1, glm::vec2& p0, glm::vec2& p1, int depth);
	void TessellateSurface(float s0, float s1, float t0, float t1, std::vector<std::vector<glm::vec2>>&& quad, int depth);
	void DrawTessellateCurve();
	void DrawTessellateSurface();
};