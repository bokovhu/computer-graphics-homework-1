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

const float splineTension = -0.5f;
const float splineBias = -0.5f;
const float splineContinuity = 0.2f;
const int splineSegmentsBetweenControlPoints = 100;

std::vector <vec2> splineControlPoints;
std::vector <vec2> splineVertices;

unsigned int splineVao;
unsigned int splineVbo;

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform mat4 MVP;
	layout(location = 0) in vec2 vp;

	void main() {
		gl_Position = MVP * vec4(vp.x, vp.y, 0, 1);
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	out vec4 outColor;

	void main() {
		outColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU

void onInitialization() {
	
	splineControlPoints.push_back (vec2 (-1.0f, -0.8f));
	splineControlPoints.push_back (vec2 (-0.5f, -0.3f));
	splineControlPoints.push_back (vec2 (0.0f, -0.7f));
	splineControlPoints.push_back (vec2 (0.5f, 0.7f));
	splineControlPoints.push_back (vec2 (1.0f, 0.1f));
	
	for (int i = 0; i < splineControlPoints.size () - 1; i++) {
		
		auto &v1 = splineControlPoints [i];
		auto &v2 = splineControlPoints [i + 1];
		auto &v3 = splineControlPoints [i + 2 == splineControlPoints.size () - 1 ? splineControlPoints.size () - 1 : i + 2];
		
		vec2 startingTangent;
		vec2 endingTangent;
		
		startingTangent.x = ( ( (1.0f - splineTension) * (1.0f + splineBias) * (1.0f + splineContinuity) ) / 2.0f ) * (v1.x - v2.x)
			+ ( ( (1.0f - splineTension) * (1.0f - splineBias) * (1.0f - splineContinuity) ) / 2.0f ) * (v2.x - v1.x);
		startingTangent.y = ( ( (1.0f - splineTension) * (1.0f + splineBias) * (1.0f + splineContinuity) ) / 2.0f ) * (v1.y - v2.y)
			+ ( ( (1.0f - splineTension) * (1.0f - splineBias) * (1.0f - splineContinuity) ) / 2.0f ) * (v2.y - v1.y);
			
		endingTangent.x = ( ( (1.0f - splineTension) * (1.0f + splineBias) * (1.0f - splineContinuity) ) / 2.0f ) * (v2.x - v1.x)
			+ ( ( (1.0f - splineTension) * (1.0f - splineBias) * (1.0f + splineContinuity) ) / 2.0f ) * (v3.x - v2.x);
		endingTangent.y = ( ( (1.0f - splineTension) * (1.0f + splineBias) * (1.0f - splineContinuity) ) / 2.0f ) * (v2.y - v1.y)
			+ ( ( (1.0f - splineTension) * (1.0f - splineBias) * (1.0f + splineContinuity) ) / 2.0f ) * (v3.y - v2.y);
		
		for (int j = 0; j < splineSegmentsBetweenControlPoints; j++) {
			
			float t = (float) j / (float) splineSegmentsBetweenControlPoints;
			
			vec2 v;
			v.x = ( v1.x * ( 2.0f * t * t * t - 3.0f * t * t + 1.0f ) ) 
				+ ( ( t * t * t - 2.0f * t * t + t ) * startingTangent.x ) 
				+ ( ( -2.0f * t * t * t + 3.0f * t * t ) * v2.x )
				+ ( (t * t * t - t * t) * endingTangent.x );
			v.y = ( v1.y * ( 2.0f * t * t * t - 3.0f * t * t + 1.0f ) ) 
				+ ( ( t * t * t - 2.0f * t * t + t ) * startingTangent.y ) 
				+ ( ( -2.0f * t * t * t + 3.0f * t * t ) * v2.y )
				+ ( (t * t * t - t * t) * endingTangent.y );
			splineVertices.push_back (v);
			
		}
		
	}
	
	float splineVboData [splineVertices.size () * 2];
	size_t splineDataIndex = 0;
	for (int i = 0; i < splineVertices.size (); i++) {
		
		auto &splineVertex = splineVertices [i];
		
		splineVboData [splineDataIndex++] = splineVertex.x;
		splineVboData [splineDataIndex++] = splineVertex.y;
		
	}
	
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &splineVao);
	glBindVertexArray(splineVao);

	glGenBuffers(1, &splineVbo);
	glBindBuffer(GL_ARRAY_BUFFER, splineVbo);
	
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(splineVboData),
		splineVboData,
		GL_DYNAMIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		2, GL_FLOAT, GL_FALSE,
		0, NULL
	);

	gpuProgram.Create(vertexSource, fragmentSource, "outColor");
	
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

	int location = glGetUniformLocation(gpuProgram.getId(), "MVP");
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);

	glBindVertexArray(splineVao);
	glDrawArrays(GL_LINE_STRIP, 0, splineVertices.size ());

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
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}
