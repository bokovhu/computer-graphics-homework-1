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

GPUProgram gpuProgram;

inline void SetUniformMatrix (GPUProgram& program, char* name, mat4 &matrix) {
	int location = glGetUniformLocation(program.getId (), name);
	if (location >= 0) glUniformMatrix4fv(location, 1, GL_FALSE, &matrix.m[0][0]);	
}

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
			OrthographicProjection (-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, this->projection);
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
			SetPosition (0.0f, 0.0f);
			RecalculateProjection ();
		}
	
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
	
	void RecalculateVertices () {
		
		vertices.clear ();
		
		if (controlPoints.size () >= 2) {
		
			std::vector <vec2> splinePoints;
			
			for (int i = 0; i < controlPoints.size () - 1; i++) {
				
				// Source of algorithm: https://en.wikipedia.org/wiki/Cubic_Hermite_spline
				
				auto &p0 = controlPoints [max (i - 1, 0)];
				auto &p1 = controlPoints [i];
				auto &p2 = controlPoints [i + 1];
				auto &p3 = controlPoints [min (i + 2, controlPoints.size () - 1)];
				
				vec2 m1;
				vec2 m2;
				
				// Tangents are calculated based on formulas found at https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline
				
				m1.x = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f ) * (p1.x - p0.x)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f ) * (p2.x - p1.x);
				m1.y = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f ) * (p1.y - p0.y)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f ) * (p2.y - p1.y);
					
				m2.x = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f ) * (p2.x - p1.x)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f ) * (p3.x - p2.x);
				m2.y = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f ) * (p2.y - p1.y)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f ) * (p3.y - p2.y);
				
				vec2 prev;
				
				if (splinePoints.empty ()) {
					prev.x = p1.x;
					prev.y = p1.y;
				} else {
					prev.x = splinePoints [splinePoints.size () - 1].x;
					prev.y = splinePoints [splinePoints.size () - 1].y;
				}
				
				// TODO: Optimize for pixels
				for (int j = 0; j < segmentsPerControlPoint; j++) {
					
					// Linear interpolation
					float t = (float) j / (float) segmentsPerControlPoint;
				
					vec2 v;
					v.x = ( p1.x * ( 2.0f * t * t * t - 3.0f * t * t + 1.0f ) ) 
						+ ( ( t * t * t - 2.0f * t * t + t ) * m1.x ) 
						+ ( ( -2.0f * t * t * t + 3.0f * t * t ) * p2.x )
						+ ( (t * t * t - t * t) * m1.x );
					v.y = ( p1.y * ( 2.0f * t * t * t - 3.0f * t * t + 1.0f ) ) 
						+ ( ( t * t * t - 2.0f * t * t + t ) * m2.y ) 
						+ ( ( -2.0f * t * t * t + 3.0f * t * t ) * p2.y )
						+ ( (t * t * t - t * t) * m2.y );
						
					splinePoints.push_back (v);
						
					vertices.push_back (cloneVec2 (prev));
					vertices.push_back (cloneVec2 (v));
					vertices.push_back (vec2 (prev.x, -1.0f));
					
					vertices.push_back (cloneVec2 (v));
					vertices.push_back (vec2 (v.x, -1.0f));
					vertices.push_back (vec2 (prev.x, -1.0f));
					
					prev.x = v.x;
					prev.y = v.y;
					
				}
				
			}
		
		}
		
	}
	
	void UploadVertices () {
		
		glBindBuffer (GL_ARRAY_BUFFER, this->vbo);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * vertices.size () * 2, &vertices [0], GL_DYNAMIC_DRAW);
		
	}
	
	void AddControlPoint (vec2 cp) {
		
		controlPoints.push_back (cp);
		this->RecalculateVertices ();
		this->UploadVertices ();
		
	}
	
	void Draw () {
		
		int colorUniformLocation = glGetUniformLocation(gpuProgram.getId(), "u_color");
		glUniform3f (colorUniformLocation, color.x, color.y, color.z);
		
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size ());
		glBindVertexArray (0);
		
	}
	
};

struct MonocycleGame {
	
	Spline backgroundSpline;
	Spline levelSpline;
	Camera camera;
	
	void Draw () {
		
		backgroundSpline.Draw ();
		levelSpline.Draw ();
		
	}
	
};

MonocycleGame game;
mat4 model;

const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform mat4 u_view;
	uniform mat4 u_projection;
	uniform mat4 u_model;
	layout(location = 0) in vec2 in_vertexPosition;

	void main() {
		gl_Position = u_projection * u_view * u_model * vec4 (in_vertexPosition.x, in_vertexPosition.y, 0.0, 1.0);
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
	
	game.backgroundSpline.Setup ();
	game.backgroundSpline.color = vec3 (0.4f, 0.4f, 0.4f);
	game.backgroundSpline.AddControlPoint (vec2 (-1.0f, 0.6f));
	game.backgroundSpline.AddControlPoint (vec2 (-0.7f, 0.2f));
	game.backgroundSpline.AddControlPoint (vec2 (-0.1f, -0.4f));
	game.backgroundSpline.AddControlPoint (vec2 (0.2f, 0.1f));
	game.backgroundSpline.AddControlPoint (vec2 (0.5f, -0.2f));
	game.backgroundSpline.AddControlPoint (vec2 (1.0f, 0.6f));
	game.backgroundSpline.tension = 0.0f;
	game.backgroundSpline.continuity = 0.0f;
	game.backgroundSpline.bias = 0.0f;
	game.backgroundSpline.RecalculateVertices ();
	game.backgroundSpline.UploadVertices ();
	game.levelSpline.Setup ();
	
	
	glViewport(0, 0, windowWidth, windowHeight);

	gpuProgram.Create(vertexSource, fragmentSource, "out_finalColor");
	
	game.camera.Setup ();
	IdentityMatrix (model);
	
}

void onDisplay() {
	
	glClearColor(0, 0.08f, 0.82f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	game.camera.SetShaderUniforms (gpuProgram, "u_view", "u_projection");
	model.SetUniform (gpuProgram.getId (), "u_model");

	game.Draw ();

	glutSwapBuffers();
	
}

void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();
	
	if (key == 'z') {
		game.backgroundSpline.tension += 0.05f;
	}
	
	if (key == 'h') {
		game.backgroundSpline.tension -= 0.05f;
	}
	
	if (key == 'u') {
		game.backgroundSpline.bias += 0.05f;
	}
	
	if (key == 'j') {
		game.backgroundSpline.bias -= 0.05f;
	}
	
	if (key == 'i') {
		game.backgroundSpline.continuity += 0.05f;
	}
	
	if (key == 'k') {
		game.backgroundSpline.continuity -= 0.05f;
	}
	
	if (key == 'w') {
		game.camera.SetPosition (game.camera.GetX(), game.camera.GetY () + 0.05f);
	}
	
	if (key == 's') {
		game.camera.SetPosition (game.camera.GetX(), game.camera.GetY () - 0.05f);
	}
	
	if (key == 'd') {
		game.camera.SetPosition (game.camera.GetX() + 0.05f, game.camera.GetY ());
	}
	
	if (key == 'a') {
		game.camera.SetPosition (game.camera.GetX() - 0.05f, game.camera.GetY ());
	}
	
	game.backgroundSpline.RecalculateVertices ();
	game.backgroundSpline.UploadVertices ();
	glutPostRedisplay();
	printf ("tension: %3.2f, bias: %3.2f, continuity: %3.2f\n", game.backgroundSpline.tension, game.backgroundSpline.bias, game.backgroundSpline.continuity);
	
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
	
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;

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
	
	glutPostRedisplay();
	
}
