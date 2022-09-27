#include "ExampleLayer.h"
#include <cmath>

using namespace GLCore;
using namespace GLCore::Utils;

ExampleLayer::ExampleLayer()
	: m_CameraController(16.0f / 9.0f)
{
	surfaceControlPoints = {
		{{-1.0f,  1.0f}, {0.0f,  1.0f}, {1.0f,  1.0f}},
		{{-1.0f,  0.0f}, {0.0f,  0.0f}, {1.0f,  0.0f}},
		{{-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}}
	};

	curveControlPoints = {{-1.5f, 0.5f}, {-0.5f, 1.0f}, {0.5f, -1.0f}, {1.0f, 0.0f}};
}

ExampleLayer::~ExampleLayer()
{

}

void ExampleLayer::OnAttach()
{
	EnableGLDebugging();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_Shader = Shader::FromGLSLTextFiles(
		"assets/shaders/test.vert.glsl",
		"assets/shaders/test.frag.glsl"
	);

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* data = stbi_load("assets/imgs/image.png", &width, &height, &nrChannels, 4);
	if(data){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else{
		std::cout << "Failed to load texture" << std::endl;
	}

	glCreateVertexArrays(1, &m_QuadVA);
	glBindVertexArray(m_QuadVA);

	glCreateBuffers(1, &m_QuadVB);
	glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 64, nullptr, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texture));

	//uint32_t indices[] = { 0, 1, 2, 2, 3, 0};
	glCreateBuffers(1, &m_QuadIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 10, nullptr, GL_DYNAMIC_DRAW);
}

void ExampleLayer::OnDetach()
{
	glDeleteVertexArrays(1, &m_QuadVA);
	glDeleteBuffers(1, &m_QuadVB);
	glDeleteBuffers(1, &m_QuadIB);
}

void ExampleLayer::OnEvent(Event& event)
{
	m_CameraController.OnEvent(event);

	EventDispatcher dispatcher(event);
}

void ExampleLayer::OnUpdate(Timestep ts)
{
	fps = 1000.0f / ts.GetMilliseconds();

	pointsCount = 0;
	linesCount = 0;
	trianglesCount = 0;
	quadsCount = 0;

	m_CameraController.OnUpdate(ts);

	if(wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(m_Shader->GetRendererID());

	glBindTexture(GL_TEXTURE_2D, 0);

	int location = glGetUniformLocation(m_Shader->GetRendererID(), "u_ViewProjection");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m_CameraController.GetCamera().GetViewProjectionMatrix()));

	glBindVertexArray(m_QuadVA);

	if(currentShape == Shape::Curve){
		if(tessellate)
			DrawTessellateCurve();
		if(calculate)
			DrawCurve(m_Step);
		if(controlPoints)
			DrawCurveControlLines();
	}

	if(currentShape == Shape::Surface){
		if(tessellate)
			DrawTessellateSurface();
		if(calculate)
			DrawSurface(m_Step);
		if(controlPoints)
			DrawSurfaceControlLines();
	}
	
	//glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, nullptr);
}

void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin("Controls");
	if(tessellate){
		ImGui::DragInt("Max Tessellation Depth", &m_MaxDepth, 0.01f, 1, 10);
		ImGui::DragFloat("Threshold", &m_Threshold, 0.005f, 0.0f, 10.0f);
	}
	if(calculate)
		ImGui::DragFloat("Resolution", &m_Step, 0.001f, 0.0005f, 1.0f, "%.4f");
	if(ImGui::RadioButton("Surface", currentShape == Shape::Surface))
		SwitchShape();
	if(currentShape == Shape::Curve){
		ImGui::DragInt("Current Point", &currentPoint, 0.01f, 0, (int)curveControlPoints.size()-1);
		ImGui::DragFloat2("Position", glm::value_ptr(curveControlPoints[currentPoint]), 0.01f);
	}
	if(currentShape == Shape::Surface){
		ImGui::DragInt2("Current Point", glm::value_ptr(currentCoord), 0.01f, 0, (int)surfaceControlPoints.size()-1);
		ImGui::DragFloat2("Position", glm::value_ptr(surfaceControlPoints[currentCoord.y][currentCoord.x]), 0.01f);
	}
	ImGui::Text(("Points count: " + std::to_string(pointsCount)).c_str());
	ImGui::Text(("Lines count: " + std::to_string(linesCount)).c_str());
	ImGui::Text(("Triangles count: " + std::to_string(trianglesCount)).c_str());
	ImGui::Text(("Quads count: " + std::to_string(quadsCount)).c_str());
	ImGui::Text(("FPS: " + std::to_string(fps)).c_str());
	ImGui::Checkbox("Wireframe", &wireframe);
	ImGui::Checkbox("Calculate shapes", &calculate);
	ImGui::Checkbox("Tessellate shapes", &tessellate);
	ImGui::Checkbox("Show control points/lines", &controlPoints);
	ImGui::Checkbox("Texture", &textured);
	ImGui::End();
}

void ExampleLayer::DrawLine(glm::vec2& p1, glm::vec2& p2){
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), glm::value_ptr(p1));
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vertex), sizeof(glm::vec2), glm::value_ptr(p2));

	uint32_t indecies[] = {0, 1};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, nullptr);

	linesCount++;
}

void ExampleLayer::DrawPoint(glm::vec2 p, float size){
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), glm::value_ptr(p));
	glPointSize(size / m_CameraController.GetZoomLevel());

	uint32_t indecies[] = {0};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, nullptr);

	pointsCount++;
}

void ExampleLayer::DrawQuad(std::vector<glm::vec2>& points){
	std::vector<Vertex> vertecies;
	for(auto& point : points){
		vertecies.emplace_back(Vertex(point));
	}

	uint32_t indecies[] = {0, 1, 2, 2, 3, 0};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertecies.size(), vertecies.data());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void ExampleLayer::DrawQuad(std::vector<glm::vec2>& points, std::vector<glm::vec2>& textures){
	std::vector<Vertex> vertecies;
	for(size_t i = 0; i < points.size(); i++)
		vertecies.emplace_back(Vertex(points[i], textures[i]));

	uint32_t indecies[] = {0, 1, 2, 2, 3, 0};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertecies.size(), vertecies.data());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void ExampleLayer::DrawQuad(std::vector<std::vector<glm::vec2>>& quad){
	std::vector<Vertex> vertecies;

	for(auto& line : quad)
		for(auto& point : line)
			vertecies.emplace_back(Vertex(point));

	uint32_t indecies[] = {0, 1, 2, 2, 3, 1};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	if(wireframe)
		color = {0.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertecies.size(), vertecies.data());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	trianglesCount += 2;
	quadsCount++;
}

void ExampleLayer::DrawQuad(std::vector<std::vector<glm::vec2>>& quad, std::vector<std::vector<glm::vec2>>&& textures){
	std::vector<Vertex> vertecies;

	for(size_t i = 0; i < quad.size(); i++)
		for(size_t j = 0; j < quad[i].size(); j++)
			vertecies.emplace_back(Vertex(quad[i][j], textures[i][j]));

	uint32_t indecies[] = {0, 1, 2, 2, 3, 0};
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indecies), indecies);

	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	if(wireframe)
		color = {0.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertecies.size(), vertecies.data());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	trianglesCount += 2;
	quadsCount++;
}

float lerp(float x, float y, float t){
	return x + (y - x) * t;
}

glm::vec2 lerp(glm::vec2 p1, glm::vec2 p2, float t){
	return {lerp(p1.x, p2.x, t), lerp(p1.y, p2.y, t)};
};

glm::vec2 ExampleLayer::EvaluateCurve(float t, std::vector<glm::vec2>& points){
	size_t degree = points.size();
	std::vector<glm::vec2> localPoints = points;
	while(degree > 1){
		for(size_t i = 0; i < degree - 1; i++)
			localPoints[i] = lerp(localPoints[i], localPoints[i + 1], t);
		degree--;
	}

	return localPoints[0];
}

glm::vec2 ExampleLayer::EvaluateSurface(float s, float t, std::vector<std::vector<glm::vec2>>& points){
	std::vector<glm::vec2> localPoints;
	for(std::vector<glm::vec2> curve : points)
		localPoints.push_back(EvaluateCurve(t, curve));

	std::reverse(localPoints.begin(), localPoints.end());

	return EvaluateCurve(s, localPoints);
}

void ExampleLayer::SwitchShape(){
	if(currentShape == Shape::Curve)
		currentShape = Shape::Surface;
	else
		currentShape = Shape::Curve;
}

void ExampleLayer::DrawCurveControlLines(){
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	for(size_t i = 1; i < curveControlPoints.size(); i++)
		DrawLine(curveControlPoints[i - 1], curveControlPoints[i]);

	color = {1.0f, 0.0f, 0.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	float size = 10.0f;

	for(size_t i = 0; i < curveControlPoints.size(); i++){
		if(currentPoint == i){
			color = {1.0f, 1.0f, 0.0f, 1.0f};
			size = 20.0f;
		}
		else{
			color = {1.0f, 0.0f, 0.0f, 1.0f};
			size = 10.0f;
		}

		glUniform4fv(location, 1, glm::value_ptr(color));
		DrawPoint(curveControlPoints[i], size);
	}
}

void ExampleLayer::DrawSurfaceControlLines(){
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {1.0f, 0.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	for(size_t i = 0; i < surfaceControlPoints.size(); i++){
		for(float t = 0.0f; t <= 1.0f; t += m_Step){
			std::vector<glm::vec2> verticalLine;
			for(auto& line : surfaceControlPoints)
				verticalLine.push_back(line[i]);
			DrawPoint(EvaluateCurve(t, surfaceControlPoints[i]));
			DrawPoint(EvaluateCurve(t, verticalLine));
		}
	}

	float size = 10.0f;

	for(size_t i = 0; i < surfaceControlPoints.size(); i++)
	for(size_t j = 0; j < surfaceControlPoints[i].size(); j++){
		if(currentCoord.y == i && currentCoord.x == j){
			color = {1.0f, 1.0f, 0.0f, 1.0f};
			size = 20.0f;
		}
		else{
		color = {1.0f, 0.0f, 0.0f, 1.0f};
			size = 10.0f;
		}

		glUniform4fv(location, 1, glm::value_ptr(color));
		DrawPoint(surfaceControlPoints[i][j], size);
	}
}

void ExampleLayer::DrawCurve(float step){
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {0.0f, 0.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	if(step <= 0.0f)
		return;

	for(float t = 0.0f; t <= 1.0f; t += step)
		DrawPoint(EvaluateCurve(t, curveControlPoints));
}

void ExampleLayer::DrawSurface(float step){
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	if(step <= 0.005f)
		return;

	for(float s = 0.0f; s <= 1.0f; s += step)
		for(float t = 0.0f; t <= 1.0f; t += step)
			DrawPoint(EvaluateSurface(s, t, surfaceControlPoints));
}

void ExampleLayer::TessellateCurve(float t0, float t1, glm::vec2& p0, glm::vec2& p1, int depth){
	if(depth >= m_MaxDepth){
		DrawLine(p0, p1);
		return;
	}
	
	float tOneQuarter = (t1 + t0) * 0.25f;
	float tMidpoint = (t1 + t0) * 0.5f;
	float tThreeQuarter = (t1 + t0) * 0.75f;

	glm::vec2 cOneQuarter = EvaluateCurve(tOneQuarter, curveControlPoints);
	glm::vec2 cMidpoint = EvaluateCurve(tMidpoint, curveControlPoints);
	glm::vec2 cThreeQuarter = EvaluateCurve(tThreeQuarter, curveControlPoints);

	glm::vec2 pOneQuarter = (p0 + p1) * 0.25f;
	glm::vec2 pMidpoint = (p0 + p1) * 0.5f;
	glm::vec2 pThreeQuarter = (p0 + p1) * 0.75f;

	if(glm::distance(cOneQuarter, pOneQuarter) > m_Threshold ||
	   glm::distance(cMidpoint, pMidpoint) > m_Threshold ||
	   glm::distance(cThreeQuarter, pThreeQuarter) > m_Threshold){
		TessellateCurve(t0, tMidpoint, p0, cMidpoint, depth + 1);
		TessellateCurve(tMidpoint, t1, cMidpoint, p1, depth + 1);
	}
	else
		DrawLine(p0, p1);
}

void ExampleLayer::TessellateSurface(float s0, float s1, float t0, float t1, std::vector<std::vector<glm::vec2>>&& quad, int depth){
	if(depth >= m_MaxDepth){
		DrawQuad(quad, {{{t0, s0}, {t1, s0}},
						{{t1, s1}, {t0, s1}}});
		return;
	}

	float sMidpoint = (s1 + s0) * 0.5f;
	float tMidpoint = (t1 + t0) * 0.5f;

	glm::vec2 cTopMid = EvaluateSurface(s1, tMidpoint, surfaceControlPoints);
	glm::vec2 cLeftMid = EvaluateSurface(sMidpoint, t0, surfaceControlPoints);
	glm::vec2 cRightMid = EvaluateSurface(sMidpoint, t1, surfaceControlPoints);
	glm::vec2 cBottomMid = EvaluateSurface(s0, tMidpoint, surfaceControlPoints);
	glm::vec2 cCenter = EvaluateSurface(sMidpoint, tMidpoint, surfaceControlPoints);

	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {0.0f, 1.0f, 0.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	glm::vec2 pTopMid = (quad[1][1] + quad[1][0]) * 0.5f;
	glm::vec2 pLeftMid = (quad[1][1] + quad[0][0]) * 0.5f;
	glm::vec2 pRightMid = (quad[1][0] + quad[0][1]) * 0.5f;
	glm::vec2 pBottomMid = (quad[0][0] + quad[0][1]) * 0.5f;
	glm::vec2 pCenter = (pTopMid + pBottomMid) * 0.5f;

	color = {0.0f, 0.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	if(glm::distance(cCenter, pCenter) > m_Threshold ||
	   glm::distance(cLeftMid, pLeftMid) > m_Threshold ||
	   glm::distance(cRightMid, pRightMid) > m_Threshold ||
	   glm::distance(cTopMid, pTopMid) > m_Threshold ||
	   glm::distance(cBottomMid, pBottomMid) > m_Threshold){
		if(glm::distance(cLeftMid, pLeftMid) > m_Threshold ||
		   glm::distance(cRightMid, pRightMid) > m_Threshold){
			TessellateSurface(sMidpoint, s1, t0, t1, {{{cLeftMid}, {cRightMid}},
													 {{quad[1][0]}, {quad[1][1]}}}, depth + 1);
			TessellateSurface(s0, sMidpoint, t0, t1, {{{quad[0][0]}, {quad[0][1]}},
													 {{cRightMid}, {cLeftMid}}}, depth + 1);
		}
		else if(glm::distance(cTopMid, pTopMid) > m_Threshold ||
				glm::distance(cBottomMid, pBottomMid) > m_Threshold){
			 TessellateSurface(s0, s1, t0, tMidpoint, {{{quad[0][0]}, {cBottomMid}},
													  {{cTopMid}, {quad[1][1]}}}, depth + 1);
			 TessellateSurface(s0, s1, tMidpoint, t1, {{{cBottomMid}, {quad[0][1]}},
													  {{quad[1][0]}, {cTopMid}}}, depth + 1);
		}
		else{
			TessellateSurface(sMidpoint, s1, t0, tMidpoint, {{{cLeftMid}, {cCenter}},
															{{cTopMid}, {quad[1][1]}}}, depth + 1);
			TessellateSurface(sMidpoint, s1, tMidpoint, t1, {{{cCenter}, {cRightMid}},
															{{quad[1][0]}, {cTopMid}}}, depth + 1);
			TessellateSurface(s0, sMidpoint, t0, tMidpoint, {{{quad[0][0]}, {cBottomMid}},
															{{cCenter}, {cRightMid}}}, depth + 1);
			TessellateSurface(s0, sMidpoint, tMidpoint, t1, {{{cBottomMid}, {quad[0][1]}},
															{{cRightMid}, {cCenter}}}, depth + 1);
		}
	}
	else
		DrawQuad(quad, {{{t0, s0}, {t1, s0}},
						{{t1, s1}, {t0, s1}}});

	location = glGetUniformLocation(m_Shader->GetRendererID(), "textured");
	glUniform1i(location, false);

	if(controlPoints){
		DrawPoint(cTopMid);
		DrawPoint(cLeftMid);
		DrawPoint(cRightMid);
		DrawPoint(cBottomMid);
		DrawPoint(cCenter);

		DrawPoint(pTopMid);
		DrawPoint(pLeftMid);
		DrawPoint(pRightMid);
		DrawPoint(pBottomMid);
		DrawPoint(pCenter);
	}

	location = glGetUniformLocation(m_Shader->GetRendererID(), "textured");
	glUniform1i(location, textured && !wireframe);
}

void ExampleLayer::DrawTessellateCurve(){
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {0.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));
	TessellateCurve(0.0f, 1.0f, curveControlPoints.front(), curveControlPoints.back(), 0);
}

void ExampleLayer::DrawTessellateSurface(){
	if(textured)
		glBindTexture(GL_TEXTURE_2D, texture);
	else
		glBindTexture(GL_TEXTURE_2D, 0);
	uint32_t location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
	glm::vec4 color = {0.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(location, 1, glm::value_ptr(color));

	location = glGetUniformLocation(m_Shader->GetRendererID(), "textured");
	glUniform1i(location, textured && !wireframe);
	TessellateSurface(0.0f, 1.0f, 0.0f, 1.0f, {{{surfaceControlPoints.back().front()}, {surfaceControlPoints.back().back()}},
												{{surfaceControlPoints.front().back()}, {surfaceControlPoints.front().front()}}}, 0);
	glUniform1i(location, false);
}