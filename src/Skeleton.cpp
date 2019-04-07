//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Kovacs Botond Janos
// Neptun : SSEGZO
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

inline int min (int a, int b) {
	return a < b ? a : b;
}

inline int max (int a, int b) {
	return a > b ? a : b;
}

inline void IdentityMatrix (mat4 &matrix) {
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			matrix.m [i][j] = 0.0f;
		}
		matrix.m [i][i] = 1.0f;
	}
	
}

// Source: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/orthographic-projection-matrix
inline void OrthographicProjection (
	float left, 
	float right, 
	float top, 
	float bottom,
	float n,
	float f,
	mat4 &matrix
) {
	
	IdentityMatrix (matrix);
	
	matrix.m [0][0] = 2.0f / (right - left);
	matrix.m [1][1] = 2.0f / (top - bottom);
	matrix.m [2][2] = -2.0f / (f - n);
	matrix.m [3][3] = 1.0f;
	
	matrix.m [0][3] = -1.0f * ( (right + left) / (right - left) );
	matrix.m [1][3] = -1.0f * ( (top + bottom) / (top - bottom) );
	matrix.m [2][3] = -1.0f * ( (f + n) / (f - n) );
	
}

inline float unzero (float a) {
	if (a == 0.0f) return 0.001f;
	return a;
}

inline void SetUniformMatrix (GPUProgram& program, char* name, mat4 &matrix) {
	int location = glGetUniformLocation(program.getId (), name);
	if (location >= 0) glUniformMatrix4fv(location, 1, GL_FALSE, &matrix.m[0][0]);	
}

const float EPSILON = 0.01f;

class Camera {
	private:
		vec2 position;
		mat4 projection;
		mat4 view;
	public:
	
		void SetPosition (float x, float y) {
			position.x = x;
			position.y = y;
			RecalculateView ();
		}
		
		float GetX () { return position.x; }
		float GetY () { return position.y; }
		
		void RecalculateView () {
			IdentityMatrix (this->view);
			this->view = this->view * TranslateMatrix (vec3 (-position.x, -position.y, 0.0f));
		}
		
		void RecalculateProjection () {
			OrthographicProjection (0, windowWidth * 1.0f, windowHeight * 1.0f, 0, 0.0f, 1000.0f, this->projection);
		}
		
		void SetShaderUniforms (
			GPUProgram& program, 
			char* viewUniformName, 
			char* projectionUniformName
		) {
			
			SetUniformMatrix (program, viewUniformName, this->view);
			SetUniformMatrix (program, projectionUniformName, this->projection);
			
		}
	
		void Setup () {
			SetPosition (windowWidth / 2.0f, windowHeight / 2.0f);
			RecalculateProjection ();
		}
	
};

const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform mat4 u_view;
	uniform mat4 u_projection;
	uniform mat4 u_model;
	layout(location = 0) in vec2 in_vertexPosition;

	void main() {
		gl_Position = vec4 ((u_projection * u_view * u_model * vec4 (in_vertexPosition.x, in_vertexPosition.y, 0.0, 1.0)).xyz, 1.0);
	}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	uniform vec3 u_color;
	out vec4 out_finalColor;

	void main() {
		out_finalColor = vec4(u_color, 1.0);
	}
)";

const char* const backgroundVertexSource = R"(
	#version 330

	precision highp float;

	layout(location = 0) in vec2 in_vertexPosition;
	layout(location = 1) in vec2 in_vertexTexCoord;

	out vec2 v_vertexTexCoord;

	void main () {
		v_vertexTexCoord = in_vertexTexCoord;
		gl_Position = vec4 (in_vertexPosition.x, in_vertexPosition.y, 0.0, 1.0);
	}
)";

const char* const backgroundFragmentSource = R"(
	#version 330

	in vec2 v_vertexTexCoord;

	uniform sampler2D u_texture;

	out vec4 out_finalColor;

	void main () {
		out_finalColor = texture2D(u_texture, v_vertexTexCoord);
	}
)";

GPUProgram gpuProgram;
GPUProgram backgroundGpuProgram;
mat4 model;

struct FindControlPointsResult {
	vec2 p0;
	vec2 p1;
	vec2 p2;
	vec2 p3;
	bool found;
};

struct Spline {
	
	// Source: https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline
	float tension = -0.5f;
	float bias = 0.0f;
	float continuity = 0.5f;
	
	int segmentsPerControlPoint = 50;
	unsigned int vao;
	unsigned int vbo;
	std::vector <vec2> controlPoints;
	std::vector <vec2> vertices;
	vec3 color;
	
	void Setup () {
		
		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);

		glGenBuffers(1, &this->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			0
		);
		
		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		
	}

	FindControlPointsResult FindControlPoints (float x) {

		vec2 p0 = controlPoints [0];
		vec2 p1 = controlPoints [0];
		vec2 p2 = controlPoints [0];
		vec2 p3 = controlPoints [0];
		bool foundControlPoints = false;
		
		for (int i = 0; i < controlPoints.size (); i++) {
			
			vec2 cp (controlPoints [i].x, controlPoints [i].y);
			if (cp.x < x) continue;
			
			if (i == 0) {
				
				p0 = controlPoints [0];
				p1 = controlPoints [0];
				p2 = controlPoints [0];
				
				p3 = controlPoints [min (1, controlPoints.size () - 1)];
				
				foundControlPoints = true;
				break;
				
			} else if (i == 1) {
				
				p0 = controlPoints [0];
				p1 = controlPoints [0];
				p2 = controlPoints [min (1, controlPoints.size ())];
				p3 = controlPoints [min (2, controlPoints.size ())];

				foundControlPoints = true;
				break;
				
			} else {
				
				p0 = controlPoints [i - 2];
				p1 = controlPoints [i - 1];
				p2 = controlPoints [i];
				
				if (i == controlPoints.size () - 1) {
					
					p3 = controlPoints [i];
					
				} else {
					
					p3 = controlPoints [i + 1];
					
				}
				
				foundControlPoints = true;
				break;
				
			}
			
		}

		return { p0, p1, p2, p3, foundControlPoints };

	}

	float GetTangent (float x) {

		if (controlPoints.size () == 0) return 0.0f;

		FindControlPointsResult findResult = FindControlPoints (x);
		
		if (!findResult.found) {
			return 0.0f;
		}

		vec2 p0 = findResult.p0;
		vec2 p1 = findResult.p1;
		vec2 p2 = findResult.p2;
		vec2 p3 = findResult.p3;
		
		// Source: https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline
		vec2 m0 = ( p1 - p0 ) * ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f )
			+ (p2 - p1) * ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f );

		vec2 m1 = ( p2 - p1 ) * ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f )
			+ ( p3 - p2 ) * ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f );

		float t = (x - p1.x) / unzero (abs (p2.x - p1.x));

		// Source: https://en.wikipedia.org/wiki/Cubic_Hermite_spline
		vec2 tangentVector;

		tangentVector.x = (6.0f * t * t - 6.0f * t) * p1.x
			+ (3.0f * t * t - 4.0 * t + 1.0f) * m0.x
			+ (-6.0f * t * t + 6.0f * t) * p2.x
			+ (3.0f * t * t - 2.0f * t) * m1.x;

		tangentVector.y = (6.0f * t * t - 6.0f * t) * p1.y
			+ (3.0f * t * t - 4.0 * t + 1.0f) * m0.y
			+ (-6.0f * t * t + 6.0f * t) * p2.y
			+ (3.0f * t * t - 2.0f * t) * m1.y;

		return tangentVector.y / unzero (tangentVector.x);

	}

	float GetHeight (float x) {
		
		if (controlPoints.size () == 0) return 0.0f;
		
		
		FindControlPointsResult findResult = FindControlPoints (x);
		
		if (!findResult.found) {
			return 0.0f;
		}

		vec2 p0 = findResult.p0;
		vec2 p1 = findResult.p1;
		vec2 p2 = findResult.p2;
		vec2 p3 = findResult.p3;
		
		// Source: https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline
		vec2 m0 = ( p1 - p0 ) * ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f )
			+ (p2 - p1) * ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f );

		vec2 m1 = ( p2 - p1 ) * ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f )
			+ ( p3 - p2 ) * ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f );

		float d0 = m0.y / unzero (m0.x);
		float d1 = m1.y / unzero (m1.x);
			
		float t = (x - p1.x) / unzero (abs (p2.x - p1.x));
		
		// Source: https://en.wikipedia.org/wiki/Cubic_Hermite_spline
		return (2.0f * t * t * t - 3.0f * t * t + 1.0f) * p1.y
			+ (t * t * t - 2.0 * t * t + t) * d0 
			+ (-2.0f * t * t * t + 3.0f * t * t) * p2.y
			+ (t * t * t - t * t) * d1;
		
	}
	
	void RecalculateVertices () {
		
		vertices.clear ();
		
		if (controlPoints.size () >= 2) {
			
			float fromX = controlPoints [0].x;
			float toX = controlPoints [controlPoints.size () - 1].x;
			float step = 1.0f;
			
			float x = fromX;
			while (x < toX) {

				float y = GetHeight (x);
				vertices.push_back (vec2 (x, 0.0f));
				vertices.push_back (vec2 (x, y));

				x += step;
			}
			
		}
		
	}
	
	void UploadVertices () {
		
		glBindBuffer (GL_ARRAY_BUFFER, this->vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * vertices.size () * 2, &vertices [0], GL_DYNAMIC_DRAW);
		
	}
	
	void AddControlPoint (vec2 cp, bool doRecalculate = true) {
		
		bool inserted = false;
		for (auto it = controlPoints.begin (); it != controlPoints.end (); it++) {
			
			auto p = *it;
			if (p.x > cp.x) {
				controlPoints.insert (it, cp);
				inserted = true;
				break;
			}
			
		}
		
		if (!inserted) {
			controlPoints.push_back (cp);
		}
		
		if (doRecalculate) {

			this->RecalculateVertices ();
			this->UploadVertices ();

		}
		
	}
	
	void Draw () {
		
		int colorUniformLocation = glGetUniformLocation(gpuProgram.getId(), "u_color");
		glUniform3f (colorUniformLocation, color.x, color.y, color.z);
		
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size ());
		glBindVertexArray (0);
		
	}
	
};

struct IntersectCirclesResult {

	bool flag;
	vec2 r1;
	vec2 r2;

};

// Source: http://paulbourke.net/geometry/circlesphere/
IntersectCirclesResult intersectCircles (
	vec2 center1, float radius1,
	vec2 center2, float radius2
) {

	IntersectCirclesResult result;
	result.flag = false;

	vec2 diff = center2 - center1;
	float d = length (diff);

	if (d > radius1 + radius2 || d < radius2 - radius1 || (d == 0.0f && radius1 == radius2)) {
	} else {

		float cd = ( radius1 * radius1 - radius2 * radius2 + d * d ) / (2 * d);
		float hcl = sqrtf ( radius1 * radius1 - cd * cd );
		vec2 cm (center1.x + (cd * diff.x) / d, center1.y + (cd * diff.y) / d);

		result.r1 = vec2 (cm.x + (hcl * diff.y) / d, cm.y - (hcl * diff.x) / d);
		result.r2 = vec2 (cm.x - (hcl * diff.y) / d, cm.y + (hcl * diff.x) / d);
		result.flag = true;

	}

	return result;

}

struct Monocycle {

	vec2 position;
	vec2 offset;
	float wheelRotation;
	mat4 model;

	unsigned int wheelVao;
	unsigned int wheelVbo;

	int numWheelSegments = 30;
	int numWheelCrossLines = 4;

	int numWheelVertices = 0;

	unsigned int headAndBodyVao;
	unsigned int headAndBodyVbo;

	int numHeadSegments = 15;
	int numHeadAndBodyVertices = 0;

	unsigned int legsVao;
	unsigned int legsVbo;

	int numLegVertices = 0;

	float mass = 1.0f;
	float wheelRadius = 24.0f;
	float headRadius = 12.0f;
	float headDistanceFromWheel = wheelRadius * 3.0f;
	float bodyDistanceFromWheel = wheelRadius * 1.5f;
	float pedalCircleRadius = wheelRadius * 0.6f;

	bool goingRight = true;

	void CreateWheel () {

		glGenVertexArrays (1, &wheelVao);
		glBindVertexArray (wheelVao);

		glGenBuffers (1, &wheelVbo);
		glBindBuffer (GL_ARRAY_BUFFER, wheelVbo);

		float angleStep = (float) M_PI * 2.0f / (numWheelSegments * 1.0f);
		std::vector <vec2> wheelVertices;

		for (int i = 0; i < numWheelSegments; i++) {

			float angleRadians = i * 1.0f * angleStep;

			wheelVertices.push_back (
				vec2 (
					cosf (angleRadians) * wheelRadius,
					sinf (angleRadians) * wheelRadius
				)
			);
			wheelVertices.push_back (
				vec2 (
					cosf (angleRadians + angleStep) * wheelRadius,
					sinf (angleRadians + angleStep) * wheelRadius
				)
			);

		}

		angleStep = (float) M_PI / (numWheelCrossLines * 1.0f);
		for (int i = 0; i < numWheelCrossLines; i++) {

			float angleRadians = i * 1.0f * angleStep;
			wheelVertices.push_back (
				vec2 (
					cosf (angleRadians) * wheelRadius,
					sinf (angleRadians) * wheelRadius
				)
			);
			wheelVertices.push_back (
				vec2 (
					cosf (angleRadians + (float) M_PI) * wheelRadius,
					sinf (angleRadians + (float) M_PI) * wheelRadius
				)
			);

		}

		numWheelVertices = wheelVertices.size ();
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * wheelVertices.size () * 2, &wheelVertices [0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			0
		);

		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}

	void CreateHeadAndBody () {

		glGenVertexArrays (1, &headAndBodyVao);
		glBindVertexArray (headAndBodyVao);

		glGenBuffers (1, &headAndBodyVbo);
		glBindBuffer (GL_ARRAY_BUFFER, headAndBodyVbo);

		float angleStep = (float) M_PI * 2.0f / (numHeadSegments * 1.0f);
		std::vector <vec2> headVertices;

		for (int i = 0; i < numHeadSegments; i++) {

			float angleRadians = i * 1.0f * angleStep;

			headVertices.push_back (
				vec2 (
					cosf (angleRadians) * headRadius,
					sinf (angleRadians) * headRadius + headDistanceFromWheel
				)
			);
			headVertices.push_back (
				vec2 (
					cosf (angleRadians + angleStep) * headRadius,
					sinf (angleRadians + angleStep) * headRadius + headDistanceFromWheel
				)
			);

		}

		headVertices.push_back (
			vec2 (
				0.0f,
				headDistanceFromWheel - headRadius
			)
		);
		headVertices.push_back (
			vec2 (
				0.0f,
				bodyDistanceFromWheel
			)
		);

		numHeadAndBodyVertices = headVertices.size ();
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * headVertices.size () * 2, &headVertices [0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			0
		);

		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}

	void SetupLegs () {

		glGenVertexArrays (1, &legsVao);
		glBindVertexArray (legsVao);

		glGenBuffers (1, &legsVbo);
		glBindBuffer (GL_ARRAY_BUFFER, legsVbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			0
		);

		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}

	void Setup () {

		CreateWheel ();
		CreateHeadAndBody ();
		SetupLegs ();

	}

	void DrawWheel (GPUProgram& program) {

		mat4 rotation;
		IdentityMatrix (rotation);
		rotation = rotation * RotationMatrix (wheelRotation, vec3 (0.0f, 0.0f, 1.0f));

		if (!goingRight) {
			rotation = rotation * RotationMatrix ((float) M_PI, vec3 (0.0f, 1.0f, 0.0f));
		}
		
		IdentityMatrix (this->model);
		this->model = this->model * rotation;
		this->model = this->model * TranslateMatrix (vec3 (position.x + offset.x, position.y + offset.y, 0.0f));

		SetUniformMatrix (program, "u_model", this->model);

		glBindVertexArray (wheelVao);
		glDrawArrays (GL_LINES, 0, numWheelVertices);
		glBindVertexArray (0);

	}

	void DrawHeadAndBody (GPUProgram& program) {

		IdentityMatrix (this->model);

		if (!goingRight) {
			mat4 rotation;
			IdentityMatrix (rotation);
			rotation = rotation * RotationMatrix ((float) M_PI, vec3 (0.0f, 1.0f, 0.0f));
			this->model = this->model * rotation;
		}

		this->model = this->model * TranslateMatrix (vec3 (position.x + offset.x, position.y + offset.y, 0.0f));

		SetUniformMatrix (program, "u_model", this->model);

		glBindVertexArray (headAndBodyVao);
		glDrawArrays (GL_LINES, 0, numHeadAndBodyVertices);
		glBindVertexArray (0);

	}

	void DrawLegs (GPUProgram& program) {

		std::vector <vec2> legVertices;

		float legLength = abs (bodyDistanceFromWheel + pedalCircleRadius) / 2.0f;

		float leg1Angle = wheelRotation;
		vec2 leg1CirclePosition (cosf (leg1Angle) * pedalCircleRadius, sinf (leg1Angle) * pedalCircleRadius);
		vec2 leg1ToBody (leg1CirclePosition.x, leg1CirclePosition.y - bodyDistanceFromWheel);
		float halfLeg1ToBodyLength = length (leg1ToBody) / 2.0f;
		float d = sqrtf (legLength * legLength - halfLeg1ToBodyLength * halfLeg1ToBodyLength);

		IntersectCirclesResult icr = intersectCircles (
			vec2 (0.0f, bodyDistanceFromWheel), legLength,
			vec2 (leg1CirclePosition.x / 2.0f, (bodyDistanceFromWheel + leg1CirclePosition.y) / 2.0f), d
		);
		legVertices.push_back (vec2 (0.0f, bodyDistanceFromWheel));
		legVertices.push_back (vec2 (icr.r2.x, icr.r2.y));
		legVertices.push_back (leg1CirclePosition);
		legVertices.push_back (vec2 (icr.r2.x, icr.r2.y));


		float leg2Angle = wheelRotation + (float) M_PI;
		vec2 leg2CirclePosition (cosf (leg2Angle) * pedalCircleRadius, sinf (leg2Angle) * pedalCircleRadius);
		vec2 leg2ToBody (leg2CirclePosition.x, leg2CirclePosition.y - bodyDistanceFromWheel);
		float halfLeg2ToBodyLength = length (leg2ToBody) / 2.0f;
		d = sqrtf (legLength * legLength - halfLeg2ToBodyLength * halfLeg2ToBodyLength);

		icr = intersectCircles (
			vec2 (0.0f, bodyDistanceFromWheel), legLength,
			vec2 (leg2CirclePosition.x / 2.0f, (bodyDistanceFromWheel + leg2CirclePosition.y) / 2.0f), d
		);
		legVertices.push_back (vec2 (0.0f, bodyDistanceFromWheel));
		legVertices.push_back (vec2 (icr.r2.x, icr.r2.y));
		legVertices.push_back (leg2CirclePosition);
		legVertices.push_back (vec2 (icr.r2.x, icr.r2.y));

		numLegVertices = legVertices.size ();

		glBindBuffer (GL_ARRAY_BUFFER, legsVbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * legVertices.size () * 2, &legVertices [0], GL_DYNAMIC_DRAW);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

		glBindVertexArray (legsVao);
		glDrawArrays (GL_LINES, 0, numLegVertices);
		glBindVertexArray (0);

	}

	void Draw (GPUProgram& program) {

		int colorUniformLocation = glGetUniformLocation(program.getId(), "u_color");
		glUniform3f (colorUniformLocation, 1.0f, 1.0f, 0.0f);

		DrawWheel (program);
		DrawHeadAndBody (program);
		DrawLegs (program);

	}

};

struct MonocycleGame {
	
	Spline backgroundSpline;
	Spline levelSpline;
	Camera camera;
	Camera stationaryCamera;
	bool cameraFollowsMonocycle;
	
	Monocycle monocycle;

	float physicsTime = 0.0f;
	
	float gravity = 10.0f;
	float drag = 1.5f;
	float monocycleForce = 350.0f;
	
	float physicsTimeStep = 0.01f;

	unsigned int debugLinesVao;
	unsigned int debugLinesVbo;

	Texture* backgroundSplineTexture;
	unsigned int backgroundVao;
	unsigned int backgroundVbo;

	void SetupBackgroundSpline () {

		backgroundSpline.Setup ();
		
		backgroundSpline.color = vec3 (0.4f, 0.4f, 0.4f);
		
		backgroundSpline.AddControlPoint (vec2 (EPSILON, windowHeight * 0.9f), false);
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.2f, windowHeight * 0.6f), false);
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.4f, windowHeight * 0.3f), false);
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.6f, windowHeight * 0.5f), false);
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.8f, windowHeight * 0.3f), false);
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 1.0f, windowHeight * 0.7f), false);
		
		backgroundSpline.tension = 0.5f;
		backgroundSpline.continuity = 0.0f;
		backgroundSpline.bias = 0.5f;
		
		backgroundSpline.RecalculateVertices ();
		
		std::vector <vec4> backgroundSplinePixels;

		for (int y = 0; y < windowHeight; y++) {
			for (int x = 0; x < windowWidth; x++) {

				vec2 v (x * 1.0f, y * 1.0f);
				vec4 pixel;

				float spline = backgroundSpline.GetHeight (x);

				if (v.y > spline) {
					if (v.y < 200.0) {
						pixel = vec4 (1.0, 1.0, 1.0, 1.0);
					} else if (v.y < 350.0) {
						pixel = vec4 (0.6, 0.6, 0.6, 1.0);
					} else {
						pixel = vec4 (0.4f, 0.4f, 0.4f, 1.0f);
					}
				} else {
					pixel = vec4 (0.24f, 0.5f, 0.95f, 1.0f);
				}

				backgroundSplinePixels.push_back (pixel);

			}
		}

		backgroundSplineTexture = new Texture (windowWidth, windowHeight, backgroundSplinePixels);

		glGenVertexArrays (1, &backgroundVao);
		glBindVertexArray (backgroundVao);

		glGenBuffers (1, &backgroundVbo);
		glBindBuffer (GL_ARRAY_BUFFER, backgroundVbo);

		float bgData [] = {
			-1.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 1.0f,

			1.0f, 1.0f, 1.0f, 0.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 1.0f
		};
		glBufferData (
			GL_ARRAY_BUFFER,
			sizeof (bgData),
			bgData,
			GL_STATIC_DRAW
		);

		glEnableVertexAttribArray (0);
		glVertexAttribPointer (
			0,
			2,
			GL_FLOAT,
			GL_FALSE,
			4 * sizeof (float),
			0
		);

		glEnableVertexAttribArray (1);
		glVertexAttribPointer (
			1,
			2,
			GL_FLOAT,
			GL_FALSE,
			4 * sizeof (float),
			(void*) (2 * sizeof (float))
		);
		
	}

	void SetupLevelSpline () {

		levelSpline.Setup ();
		levelSpline.AddControlPoint (vec2 (0, windowHeight * 0.5f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 0.35f, windowHeight * 0.7f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 0.6f, windowHeight * 0.55f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 1.0f, windowHeight * 0.3f));
		
		levelSpline.tension = -0.5f;
		levelSpline.bias = 0.5f;
		levelSpline.continuity = 0.0f;
		levelSpline.color = vec3 (0.15, 0.24, 0.17);
		
		levelSpline.RecalculateVertices ();
		levelSpline.UploadVertices ();
		
	}
	
	void Setup () {
		
		SetupBackgroundSpline ();

		SetupLevelSpline ();

		camera.Setup ();
		stationaryCamera.Setup ();
		
		monocycle.Setup ();
		monocycle.position = levelSpline.controlPoints [0];
		monocycle.position.x += EPSILON;

		cameraFollowsMonocycle = false;
		
	}
	
	void Update (float delta) {
		
		physicsTime += delta;
		
		while (physicsTime >= physicsTimeStep) {
			
			if (monocycle.position.x <= levelSpline.controlPoints [0].x + 2.0) {
				monocycle.position.x = levelSpline.controlPoints [0].x + 2.0;
				monocycle.position.y = levelSpline.GetHeight (monocycle.position.x);
				monocycle.goingRight = true;
			}

			if (monocycle.position.x >= levelSpline.controlPoints [levelSpline.controlPoints.size () - 1].x - 1.0) {
				monocycle.position.x = levelSpline.controlPoints [levelSpline.controlPoints.size () - 1].x - 1.0;
				monocycle.position.y = levelSpline.GetHeight (monocycle.position.x);
				monocycle.goingRight = false;
			}

			vec2 beforePosition (monocycle.position.x, monocycle.position.y);

			float pathTangent = levelSpline.GetTangent (monocycle.position.x);
			if (pathTangent < -1000.0f) pathTangent = -1000.0f;
			if (pathTangent > 1000.0f) pathTangent = 1000.0f;

			float dr = abs (sqrtf (1.0f + pathTangent * pathTangent));

			float angle = atanf (pathTangent);
			if (!monocycle.goingRight) {
				angle *= -1.0f;
			}

			float sinAlpha = sinf (angle);
			float cosAlpha = cosf (angle);

			float velocity = ( monocycleForce - (monocycle.mass * gravity * sinAlpha) ) / drag;
			float distanceTraveled = velocity * physicsTimeStep;

			float dx = distanceTraveled / dr;
			if (!monocycle.goingRight) {
				dx *= -1.0f;
			}
			monocycle.position.x += dx;
			monocycle.position.y = levelSpline.GetHeight (monocycle.position.x);

			pathTangent = levelSpline.GetTangent (monocycle.position.x);

			vec2 normal;
			normal.x = -sinf (atanf (pathTangent));
			normal.y = cosf (atanf (pathTangent));

			float actualTotalMovement = length (monocycle.position - beforePosition);
			float angleChange = (2.0f * M_PI) * (actualTotalMovement / (2 * monocycle.wheelRadius * M_PI));
			printf ("actualTotalMovement: %3.2f, angleChange: %3.2f, angle: %3.2f\n", actualTotalMovement, angleChange, monocycle.wheelRotation);

			monocycle.wheelRotation -= angleChange;

			monocycle.offset = vec2 ( normal.x * monocycle.wheelRadius, normal.y * monocycle.wheelRadius );
			

			physicsTime -= physicsTimeStep;
			
		}
		
		if (cameraFollowsMonocycle) {
			
			camera.SetPosition ( monocycle.position.x, camera.GetY () );
			
		}
		
	}

	void Draw () {
		
		backgroundGpuProgram.Use ();
		backgroundSplineTexture->SetUniform (backgroundGpuProgram.getId (), "u_texture");

		glBindVertexArray (backgroundVao);
		glDrawArrays (GL_TRIANGLES, 0, 6);
		glBindVertexArray (0);

		gpuProgram.Use ();

		camera.SetShaderUniforms (gpuProgram, "u_view", "u_projection");
		levelSpline.Draw ();
		monocycle.Draw (gpuProgram);
		
	}
	
};

MonocycleGame game;
long lastFrameTimestamp;

void onInitialization() {
	
	game.Setup ();
	
	glViewport(0, 0, windowWidth, windowHeight);

	gpuProgram.Create(vertexSource, fragmentSource, "out_finalColor");
	backgroundGpuProgram.Create (backgroundVertexSource, backgroundFragmentSource, "out_finalColor");
	
	IdentityMatrix (model);
	
}

void onDisplay() {
	
	glClearColor(0, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	model.SetUniform (gpuProgram.getId (), "u_model");

	game.Draw ();

	glutSwapBuffers();
	
}

void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();
	
	if (key == ' ') {
		game.cameraFollowsMonocycle = !game.cameraFollowsMonocycle;
	}

	glutPostRedisplay();
	
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(int pX, int pY) {
	
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	
}

void onMouse(int button, int state, int pX, int pY) {
	
	float cX = pX * 1.0f + (game.camera.GetX () - windowWidth / 2.0f);
	float cY = windowHeight - pY * 1.0f;

	switch (button) {
		case GLUT_LEFT_BUTTON:   
			if (state == GLUT_DOWN) {
				game.levelSpline.AddControlPoint (vec2 (cX, cY));
			}
			break;
	}

}

void onIdle() {
	
	long time = glutGet(GLUT_ELAPSED_TIME);
	long deltaMs = time - lastFrameTimestamp;
	lastFrameTimestamp = time;
	
	float delta = (float) deltaMs / 1000.0f;
	if (delta > 1.0f) delta = 0.0f;
	
	game.Update (delta);
	
	glutPostRedisplay();
	
}
