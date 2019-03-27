// TODO:
//     * circle drawing for monocycle and cyclist
//     * calculate math for position of legs

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

inline vec2 cloneVec2 (vec2& v) {
	vec2 clone;
	clone.x = v.x;
	clone.y = v.y;
	return clone;
}

inline void IdentityMatrix (mat4 &matrix) {
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			matrix.m [i][j] = 0.0f;
		}
		matrix.m [i][i] = 1.0f;
	}
	
}

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

GPUProgram gpuProgram;

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
			0, // attrib location
			2, // attrib element count, vec2 has 2 elements
			GL_FLOAT, // attrib element type, elements are of type float
			GL_FALSE, // please don't normalize OpenGL, thank you very much
			0, // no stride in the data, only using positions, tightly packed together
			0 // first element of the buffer is data already
		);
		
		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		
	}

	float GetTangent (float x) {

		if (controlPoints.size () == 0) return 0.0f;
		
		vec2 p0 (controlPoints [0].x, controlPoints [0].y);
		vec2 p1 (controlPoints [0].x, controlPoints [0].y);
		vec2 p2 (controlPoints [0].x, controlPoints [0].y);
		vec2 p3 (controlPoints [0].x, controlPoints [0].y);
		bool foundControlPoints = false;
		
		for (int i = 0; i < controlPoints.size (); i++) {
			
			vec2 cp (controlPoints [i].x, controlPoints [i].y);
			if (cp.x < x) continue;
			
			if (i == 0) {
				
				p0.x = controlPoints [0].x;
				p0.y = controlPoints [0].y;
				
				p1.x = controlPoints [0].x;
				p1.y = controlPoints [0].y;
				
				p2.x = controlPoints [0].x;
				p2.y = controlPoints [0].y;
				
				p3.x = controlPoints [min (1, controlPoints.size ())].x;
				p3.y = controlPoints [min (1, controlPoints.size ())].y;
				
				foundControlPoints = true;
				break;
				
			} else if (i == 1) {
				
				p0.x = controlPoints [0].x;
				p0.y = controlPoints [0].y;
				
				p1.x = controlPoints [0].x;
				p1.y = controlPoints [0].y;
				
				p2.x = controlPoints [min (1, controlPoints.size ())].x;
				p2.y = controlPoints [min (1, controlPoints.size ())].y;
				
				p3.x = controlPoints [min (2, controlPoints.size ())].x;
				p3.y = controlPoints [min (2, controlPoints.size ())].y;
				
				foundControlPoints = true;
				break;
				
			} else {
				
				p0.x = controlPoints [i - 2].x;
				p0.y = controlPoints [i - 2].y;
				
				p1.x = controlPoints [i - 1].x;
				p1.y = controlPoints [i - 1].y;
				
				if (i == controlPoints.size () - 1) {
					
					p2.x = controlPoints [i].x;
					p2.y = controlPoints [i].y;
					
					p3.x = controlPoints [i].x;
					p3.y = controlPoints [i].y;
					
				} else {
					
					p2.x = controlPoints [i].x;
					p2.y = controlPoints [i].y;
					
					p3.x = controlPoints [i + 1].x;
					p3.y = controlPoints [i + 1].y;
					
				}
				
				foundControlPoints = true;
				break;
				
			}
			
		}
		
		if (!foundControlPoints) {
			return controlPoints [controlPoints.size () - 1].y;
		}
		
		// Calculate Kochanek-Bartels tangents
		vec2 d0;

		d0.x = ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f * unzero ( p1.x - p0.x )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f * unzero (p2.x - p1.x);

		d0.y = ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f * unzero ( p1.y - p0.y )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f * unzero (p2.y - p1.y);

		vec2 d1;

		d1.x = ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f * unzero ( p2.x - p1.x )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f * unzero ( p3.x - p2.x );
		d1.y = ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f * unzero ( p2.y - p1.y )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f * unzero ( p3.y - p2.y );
			
		// Alpha is the interpolation value between p1 and p2
		float t = (x - p1.x) / unzero (abs (p2.x - p1.x));

		vec2 tangentVector;

		tangentVector.x = (6.0f * t * t - 6.0f * t) * p1.x
			+ (3.0f * t * t - 4.0 * t + 1.0f) * d0.x
			+ (-6.0f * t * t + 6.0f * t) * p2.x
			+ (3.0f * t * t - 2.0f * t) * d1.x;

		tangentVector.y = (6.0f * t * t - 6.0f * t) * p1.y
			+ (3.0f * t * t - 4.0 * t + 1.0f) * d0.y
			+ (-6.0f * t * t + 6.0f * t) * p2.y
			+ (3.0f * t * t - 2.0f * t) * d1.y;

		return tangentVector.y / unzero (tangentVector.x);

	}

	float GetHeight (float x) {
		
		if (controlPoints.size () == 0) return 0.0f;
		
		vec2 p0 (controlPoints [0].x, controlPoints [0].y);
		vec2 p1 (controlPoints [0].x, controlPoints [0].y);
		vec2 p2 (controlPoints [0].x, controlPoints [0].y);
		vec2 p3 (controlPoints [0].x, controlPoints [0].y);
		bool foundControlPoints = false;
		
		for (int i = 0; i < controlPoints.size (); i++) {
			
			vec2 cp (controlPoints [i].x, controlPoints [i].y);
			if (cp.x < x) continue;
			
			if (i == 0) {
				
				p0.x = controlPoints [0].x;
				p0.y = controlPoints [0].y;
				
				p1.x = controlPoints [0].x;
				p1.y = controlPoints [0].y;
				
				p2.x = controlPoints [0].x;
				p2.y = controlPoints [0].y;
				
				p3.x = controlPoints [min (1, controlPoints.size ())].x;
				p3.y = controlPoints [min (1, controlPoints.size ())].y;
				
				foundControlPoints = true;
				break;
				
			} else if (i == 1) {
				
				p0.x = controlPoints [0].x;
				p0.y = controlPoints [0].y;
				
				p1.x = controlPoints [0].x;
				p1.y = controlPoints [0].y;
				
				p2.x = controlPoints [min (1, controlPoints.size ())].x;
				p2.y = controlPoints [min (1, controlPoints.size ())].y;
				
				p3.x = controlPoints [min (2, controlPoints.size ())].x;
				p3.y = controlPoints [min (2, controlPoints.size ())].y;
				
				foundControlPoints = true;
				break;
				
			} else {
				
				p0.x = controlPoints [i - 2].x;
				p0.y = controlPoints [i - 2].y;
				
				p1.x = controlPoints [i - 1].x;
				p1.y = controlPoints [i - 1].y;
				
				if (i == controlPoints.size () - 1) {
					
					p2.x = controlPoints [i].x;
					p2.y = controlPoints [i].y;
					
					p3.x = controlPoints [i].x;
					p3.y = controlPoints [i].y;
					
				} else {
					
					p2.x = controlPoints [i].x;
					p2.y = controlPoints [i].y;
					
					p3.x = controlPoints [i + 1].x;
					p3.y = controlPoints [i + 1].y;
					
				}
				
				foundControlPoints = true;
				break;
				
			}
			
		}
		
		if (!foundControlPoints) {
			return controlPoints [controlPoints.size () - 1].y;
		}
		
		// Calculate Kochanek-Bartels tangents
		float d0 = ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f * unzero ( p1.y - p0.y )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f * unzero (p2.y - p1.y);
		float d1 = ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f * unzero ( p2.y - p1.y )
			+ ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f * unzero ( p3.y - p2.y );
			
		// Alpha is the interpolation value between p1 and p2
		float t = (x - p1.x) / unzero (abs (p2.x - p1.x));
		
		// Calculate the spline interpolation value
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
	
	void AddControlPoint (vec2 cp) {
		
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
		
		this->RecalculateVertices ();
		this->UploadVertices ();
		
	}
	
	void Draw () {
		
		int colorUniformLocation = glGetUniformLocation(gpuProgram.getId(), "u_color");
		glUniform3f (colorUniformLocation, color.x, color.y, color.z);
		
		glBindVertexArray(vao);
		glDrawArrays(GL_LINES, 0, vertices.size ());
		glBindVertexArray (0);
		
	}
	
};

struct IntersectCirclesResult {

	bool flag;
	vec2 r1;
	vec2 r2;

};

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
	float wheelRadius = 32.0f;
	float headRadius = 24.0f;
	float headDistanceFromWheel = wheelRadius * 3.0f;
	float bodyDistanceFromWheel = wheelRadius * 1.5f;
	float pedalCircleRadius = wheelRadius * 0.6f;

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
			0, // attrib location
			2, // attrib element count, vec2 has 2 elements
			GL_FLOAT, // attrib element type, elements are of type float
			GL_FALSE, // please don't normalize OpenGL, thank you very much
			0, // no stride in the data, only using positions, tightly packed together
			0 // first element of the buffer is data already
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
			0, // attrib location
			2, // attrib element count, vec2 has 2 elements
			GL_FLOAT, // attrib element type, elements are of type float
			GL_FALSE, // please don't normalize OpenGL, thank you very much
			0, // no stride in the data, only using positions, tightly packed together
			0 // first element of the buffer is data already
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
			0, // attrib location
			2, // attrib element count, vec2 has 2 elements
			GL_FLOAT, // attrib element type, elements are of type float
			GL_FALSE, // please don't normalize OpenGL, thank you very much
			0, // no stride in the data, only using positions, tightly packed together
			0 // first element of the buffer is data already
		);

		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}

	void Setup () {

		// Create the wheel
		CreateWheel ();

		// Create head and body
		CreateHeadAndBody ();

		// Set-up legs
		SetupLegs ();

	}

	void DrawWheel (GPUProgram& program) {

		mat4 rotation;
		IdentityMatrix (rotation);
		rotation = rotation * RotationMatrix (wheelRotation, vec3 (0.0f, 0.0f, 1.0f));
		
		IdentityMatrix (this->model);
		this->model = this->model * rotation;
		this->model = this->model * TranslateMatrix (vec3 (position.x + offset.x, position.y + offset.y, 0.0f));

		SetUniformMatrix (program, "u_model", this->model);

		glBindVertexArray (wheelVao);
		glDrawArrays (GL_LINES, 0, numWheelVertices);
		glBindVertexArray (0);

	}

	void DrawHeadAndBody (GPUProgram& program) {

		// Other parts don't require rotation
		IdentityMatrix (this->model);
		this->model = this->model * TranslateMatrix (vec3 (position.x + offset.x, position.y + offset.y, 0.0f));

		SetUniformMatrix (program, "u_model", this->model);

		glBindVertexArray (headAndBodyVao);
		glDrawArrays (GL_LINES, 0, numHeadAndBodyVertices);
		glBindVertexArray (0);

	}

	void DrawLegs (GPUProgram& program) {

		std::vector <vec2> legVertices;

		float legLength = abs (bodyDistanceFromWheel + pedalCircleRadius) / 2.0f;

		// Leg #1
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


		// Leg #2
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

mat4 model;

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
	std::vector <vec2> debugLinesVertices;

	void SetupBackgroundSpline () {

		backgroundSpline.Setup ();
		
		backgroundSpline.color = vec3 (0.4f, 0.4f, 0.4f);
		
		backgroundSpline.AddControlPoint (vec2 (EPSILON, windowHeight * 0.9f));
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.2f, windowHeight * 0.6f));
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.4f, windowHeight * 0.3f));
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.6f, windowHeight * 0.5f));
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 0.8f, windowHeight * 0.3f));
		backgroundSpline.AddControlPoint (vec2 (windowWidth * 1.0f, windowHeight * 0.7f));
		
		backgroundSpline.tension = 0.5f;
		backgroundSpline.continuity = 0.0f;
		backgroundSpline.bias = 0.5f;
		
		backgroundSpline.RecalculateVertices ();
		backgroundSpline.UploadVertices ();
		
	}

	void SetupLevelSpline () {

		levelSpline.Setup ();
		levelSpline.AddControlPoint (vec2 (EPSILON, windowHeight * 0.5f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 0.15f, windowHeight * 0.7f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 0.3f, windowHeight * 0.55f));
		levelSpline.AddControlPoint (vec2 (windowWidth * 0.5f, windowHeight * 0.3f));
		
		levelSpline.tension = -0.5f;
		levelSpline.bias = 0.5f;
		levelSpline.continuity = 0.0f;
		
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

		SetupDebugLines ();
		
	}
	
	void Update (float delta) {
		
		if (cameraFollowsMonocycle) {
			
			camera.SetPosition ( monocycle.position.x, camera.GetY () );
			
		}
		
		physicsTime += delta;
		
		while (physicsTime >= physicsTimeStep) {
			
			float pathTangent = levelSpline.GetTangent (monocycle.position.x);
			if (pathTangent < -500.0f) pathTangent = -500.0f;
			if (pathTangent > 500.0f) pathTangent = 500.0f;

			float dr = abs (sqrtf (1.0f + pathTangent * pathTangent));

			float sinAlpha = sinf (atanf (pathTangent));
			float cosAlpha = cosf (atanf (pathTangent));

			debugLinesVertices.push_back (vec2 (monocycle.position.x - 20.0f * cosAlpha, monocycle.position.y - 20.0f * sinAlpha));
			debugLinesVertices.push_back (vec2 (monocycle.position.x + 20.0f * cosAlpha, monocycle.position.y + 20.0f * sinAlpha));

			vec2 normal;
			normal.x = -sinf (atanf (pathTangent));
			normal.y = cosf (atanf (pathTangent));

			float velocity = ( monocycleForce - (monocycle.mass * gravity * sinAlpha) ) / drag;
			float distanceTraveled = velocity * physicsTimeStep;

			float dx = distanceTraveled / dr;

			monocycle.position.x += dx;
			monocycle.position.y = levelSpline.GetHeight (monocycle.position.x);
			monocycle.wheelRotation -= (2.0f * M_PI) * (distanceTraveled / (2 * monocycle.wheelRadius * M_PI));
			monocycle.offset = vec2 ( normal.x * monocycle.wheelRadius, normal.y * monocycle.wheelRadius );
			
			physicsTime -= physicsTimeStep;
			
		}
		
	}

	void SetupDebugLines () {

		glGenVertexArrays (1, &debugLinesVao);
		glBindVertexArray (debugLinesVao);

		glGenBuffers (1, &debugLinesVbo);
		glBindBuffer (GL_ARRAY_BUFFER, debugLinesVbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0, // attrib location
			2, // attrib element count, vec2 has 2 elements
			GL_FLOAT, // attrib element type, elements are of type float
			GL_FALSE, // please don't normalize OpenGL, thank you very much
			0, // no stride in the data, only using positions, tightly packed together
			0 // first element of the buffer is data already
		);

		glBindVertexArray (0);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}

	void UploadDebugLines () {

		glBindBuffer (GL_ARRAY_BUFFER, debugLinesVbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * debugLinesVertices.size () * 2, &debugLinesVertices [0], GL_DYNAMIC_DRAW);
		glBindBuffer (GL_ARRAY_BUFFER, 0);

	}
	
	void Draw () {
		
		stationaryCamera.SetShaderUniforms (gpuProgram, "u_view", "u_projection");
		backgroundSpline.Draw ();
		
		camera.SetShaderUniforms (gpuProgram, "u_view", "u_projection");
		levelSpline.Draw ();
		monocycle.Draw (gpuProgram);

		model.SetUniform (gpuProgram.getId (), "u_model");
		UploadDebugLines ();
		glBindVertexArray (debugLinesVao);
		int colorUniformLocation = glGetUniformLocation(gpuProgram.getId(), "u_color");
		glUniform3f (colorUniformLocation, 1.0f, 0.0f, 0.0f);
		glDrawArrays (GL_LINES, 0, debugLinesVertices.size ());
		glBindVertexArray (0);

		debugLinesVertices.clear ();
		
	}
	
};

MonocycleGame game;
long lastFrameTimestamp;

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

void onInitialization() {
	
	game.Setup ();
	
	glViewport(0, 0, windowWidth, windowHeight);

	gpuProgram.Create(vertexSource, fragmentSource, "out_finalColor");
	
	IdentityMatrix (model);
	
}

void onDisplay() {
	
	glClearColor(0, 0.08f, 0.82f, 1.0f);
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
	
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
	
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	
	float cX = pX * 1.0f + (game.camera.GetX () - windowWidth / 2.0f);
	float cY = windowHeight - pY * 1.0f;

	char * buttonStat;
	switch (state) {
		case GLUT_DOWN: buttonStat = "pressed"; break;
		case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
		case GLUT_LEFT_BUTTON:   
			printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);
			if (state == GLUT_DOWN) {
				game.levelSpline.AddControlPoint (vec2 (cX, cY));
			}
			break;
		case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
		case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
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
