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

struct Spline {
	
	float tension = -0.5f;
	float bias = -0.5f;
	float continuity = 0.1f;
	int segmentsPerControlPoint = 50;
	unsigned int vao;
	unsigned int vbo;
	std::vector <vec2> controlPoints;
	std::vector <vec2> vertices;
	
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
		
			for (int i = 0; i < controlPoints.size () - 1; i++) {
				
				auto &p1 = controlPoints [i];
				auto &p2 = controlPoints [i + 1];
				auto &p3 = controlPoints [min (i + 2, controlPoints.size () - 1)];
				
				vec2 m1;
				vec2 m2;
				
				m1.x = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f ) * (p1.x - p2.x)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f ) * (p2.x - p1.x);
				m1.y = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f + continuity) ) / 2.0f ) * (p1.y - p2.y)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f - continuity) ) / 2.0f ) * (p2.y - p1.y);
					
				m2.x = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f ) * (p2.x - p1.x)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f ) * (p3.x - p2.x);
				m2.y = ( ( (1.0f - tension) * (1.0f + bias) * (1.0f - continuity) ) / 2.0f ) * (p2.y - p1.y)
					+ ( ( (1.0f - tension) * (1.0f - bias) * (1.0f + continuity) ) / 2.0f ) * (p3.y - p2.y);
				
				vertices.push_back (p1);
				
				// TODO: Optimize for pixels
				for (int j = 1; j < segmentsPerControlPoint - 1; j++) {
					
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
					vertices.push_back (v);
					
				}
				
				vertices.push_back (p2);
				
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
		
		glBindVertexArray(vao);
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size ());
		glBindVertexArray (0);
		
	}
	
};

struct MonocycleGame {
	
	Spline spline;
	
	void Draw () {
		
		spline.Draw ();
		
	}
	
};

MonocycleGame game;

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform mat4 u_modelViewProjection;
	layout(location = 0) in vec2 in_vertexPosition;

	void main() {
		gl_Position = u_modelViewProjection * vec4(in_vertexPosition.x, in_vertexPosition.y, 0, 1);
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	out vec4 out_finalColor;

	void main() {
		out_finalColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU

void onInitialization() {
	
	game.spline.Setup ();
	
	glViewport(0, 0, windowWidth, windowHeight);

	gpuProgram.Create(vertexSource, fragmentSource, "out_finalColor");
	
	glLineWidth (1.0f);
	
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	float MVPtransf[4][4] = { 1, 0, 0, 0,
		                      0, 1, 0, 0,
		                      0, 0, 1, 0,
		                      0, 0, 0, 1 };

	int location = glGetUniformLocation(gpuProgram.getId(), "u_modelViewProjection");
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);

	game.Draw ();

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
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
			game.spline.AddControlPoint (vec2 (cX, cY));
		}
		break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
	glutPostRedisplay();
}
