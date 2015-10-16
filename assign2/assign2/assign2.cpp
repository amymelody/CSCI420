// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 480 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	C++ starter code
*/

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include "vector3.h"

using namespace std;

const GLfloat MAX_LINE_LENGTH = 0.02;
const GLfloat BOX_SIZE = 60.0;
const GLfloat RAIL_SIZE = 0.2;
const GLfloat DEBUG_Z_OFFSET = -5.0;
const int SPEED = 2;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* Parallel vectors of vectors of vertices, center vectors, and up vectors for each spline. */
vector<vector<vector3>> g_vVertices;
vector<vector<vector3>> g_vCenters;
vector<vector<vector3>> g_vUps;

/* Rail vertices */
vector<vector<vector3>> g_vRailVertices;

/* Color of the rail */
vector3 g_railColor = vector3(1.0, 0.0, 0.0);

/* Keep track of which spline we are on and how far along it we are */
int g_currentSpline = 0;
int g_currentPointOnSpline = 0;

/* Texture */
GLuint* g_texture;

/* tension parameter (s) */
GLfloat g_tension = 0.5;

GLfloat** g_basisMatrix;

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);

	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;
		if (g_Splines[j].numControlPoints < 4)
		{  
			printf ("Each spline must have at least 4 control points\n");
			exit(0);
		}

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

void matMult(GLfloat** A, GLfloat** B, GLfloat** product, int mA, int nA, int nB)
{
	for (int i=0; i<mA; i++)
	{
		for (int j=0; j<nB; j++)
			product[i][j] = 0;
	}

	for (int i=0; i<mA; i++)
	{
		for (int j=0; j<nB; j++)
		{
			for (int k=0; k<nA; k++)
			{
				product[i][j] += A[i][k]*B[k][j];
			}
		}
	}
}

vector3 controlPointToSpline(vector3 splineVector, int splineIndex, int controlPointIndex, GLfloat u, bool isDerivative)
{
	// Initialize matrices
	GLfloat** uVector = new GLfloat*[1];
	uVector[0] = new GLfloat[4];
	GLfloat** vVector = new GLfloat*[1];
	vVector[0] = new GLfloat[3];
	GLfloat** controlMatrix = new GLfloat*[4];
	for (int i=0; i<4; i++)
		controlMatrix[i] = new GLfloat[3];
	GLfloat** basisControlMatrix = new GLfloat*[4];
	for (int i=0; i<4; i++)
		basisControlMatrix[i] = new GLfloat[3];

	// Fill in matrices
	if (isDerivative)
	{
		for (int i=0; i<3; i++)
			uVector[0][i] = (3-i)*pow(u, 2-i);
		uVector[0][3] = 0;
	}
	else
	{
		for (int i=0; i<4; i++)
			uVector[0][i] = pow(u, 3-i);
	}
	for (int i=0; i<4; i++)
	{
		controlMatrix[i][0] = g_Splines[splineIndex].points[controlPointIndex + i].x;
		controlMatrix[i][1] = g_Splines[splineIndex].points[controlPointIndex + i].y;
		controlMatrix[i][2] = g_Splines[splineIndex].points[controlPointIndex + i].z;
	}
	matMult(g_basisMatrix, controlMatrix, basisControlMatrix, 4, 4, 3);
	matMult(uVector, basisControlMatrix, vVector, 1, 4, 3);

	// Fill in spline vector
	splineVector.x = vVector[0][0];
	splineVector.y = vVector[0][1];
	splineVector.z = vVector[0][2];

	// Deallocate matrices
	for (int i=0; i<4; i++)
		delete[] basisControlMatrix[i];
	delete[] basisControlMatrix;
	for (int i=0; i<4; i++)
		delete[] controlMatrix[i];
	delete[] controlMatrix;
	delete[] vVector[0];
	delete[] vVector;
	delete[] uVector[0];
	delete[] uVector;

	return splineVector;
}

void computeLookAtAndRailVerts(int splineIndex, int controlPointIndex, GLfloat u)
{
	vector3 centerVert, tangent, n, up;
	centerVert = g_vVertices[splineIndex].back();

	tangent = controlPointToSpline(vector3(), splineIndex, controlPointIndex, u, true);
	tangent.normalize();
	g_vCenters[splineIndex].push_back(centerVert + tangent);

	if (g_vVertices[splineIndex].size() == 1)
	{
		// If this is the first up vector we are computing, we need to pick an arbitrary vector
		vector3 v(1.0, 0, 0);
		n = crossProduct(tangent, v);
	}
	else
	{
		// Otherwise compute the new N using the previous up vector
		n = crossProduct(g_vUps[splineIndex].back(), tangent);
	}
	n.normalize();

	up = crossProduct(tangent, n);
	up.normalize();
	g_vUps[splineIndex].push_back(up);

	// Precompute vertices for rail based on local coordinate system
	g_vRailVertices[splineIndex].push_back(centerVert + ((n - up) * RAIL_SIZE));
	g_vRailVertices[splineIndex].push_back(centerVert + ((n + up) * RAIL_SIZE));
	g_vRailVertices[splineIndex].push_back(centerVert + ((-n + up) * RAIL_SIZE));
	g_vRailVertices[splineIndex].push_back(centerVert + ((-n - up) * RAIL_SIZE));
}

void subdivide(int splineIndex, int controlPointIndex, GLfloat u0, GLfloat u1, GLfloat maxLineLength)
{
	vector3 v0 = controlPointToSpline(vector3(), splineIndex, controlPointIndex, u0, false);
	vector3 v1 = controlPointToSpline(vector3(), splineIndex, controlPointIndex, u1, false);

	if (lineLengthSquared(v0, v1) > maxLineLength*maxLineLength)
	{
		GLfloat uMid = (u0 + u1) / 2.0;
		subdivide(splineIndex, controlPointIndex, u0, uMid, maxLineLength);
		subdivide(splineIndex, controlPointIndex, uMid, u1, maxLineLength);
	}
	else
	{
		if (g_vVertices[splineIndex].empty() || g_vVertices[splineIndex].back().x != v0.x ||
			g_vVertices[splineIndex].back().y != v0.y || g_vVertices[splineIndex].back().z != v0.z)
		{
			// Don't repeat vertices
			g_vVertices[splineIndex].push_back(v0);
			computeLookAtAndRailVerts(splineIndex, controlPointIndex, u0);
		}
		g_vVertices[splineIndex].push_back(v1);
		computeLookAtAndRailVerts(splineIndex, controlPointIndex, u1);
	}
}

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
}

void texload(int i, char* filename)
{
	Pic* img;
	img = jpeg_read(filename, NULL);
	glBindTexture(GL_TEXTURE_2D, g_texture[i]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 
		0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
	pic_free(img);
}

void myinit(char* argv)
{
	/* Load texture */
	g_texture = new GLuint[1];
	glGenTextures(1, g_texture);
	texload(0, argv);

	/* Fill in the basis matrix */
	g_basisMatrix = new GLfloat*[4];
	for (int i=0; i<4; i++)
		g_basisMatrix[i] = new GLfloat[4];
	g_basisMatrix[0][0] = -g_tension;
	g_basisMatrix[0][1] = 2.0 - g_tension;
	g_basisMatrix[0][2] = g_tension - 2.0;
	g_basisMatrix[0][3] = g_tension;
	g_basisMatrix[1][0] = 2.0 * g_tension;
	g_basisMatrix[1][1] = g_tension - 3.0;
	g_basisMatrix[1][2] = 3.0 - 2.0 * g_tension;
	g_basisMatrix[1][3] = -g_tension;
	g_basisMatrix[2][0] = -g_tension;
	g_basisMatrix[2][1] = 0;
	g_basisMatrix[2][2] = g_tension;
	g_basisMatrix[2][3] = 0;
	g_basisMatrix[3][0] = 0;
	g_basisMatrix[3][1] = 1.0;
	g_basisMatrix[3][2] = 0;
	g_basisMatrix[3][3] = 0;

	/* Fill in the parallel vectors of vertices from spline info */
	for (int i=0; i<g_iNumOfSplines; i++)
	{
		g_vVertices.push_back(vector<vector3>());
		g_vCenters.push_back(vector<vector3>());
		g_vUps.push_back(vector<vector3>());
		g_vRailVertices.push_back(vector<vector3>());
		for (int j=0; j<g_Splines[i].numControlPoints-3; j++)
		{
			subdivide(i, j, 0.0, 1.0, MAX_LINE_LENGTH);
		}
	}

  glClearColor(0.0, 0.0, 0.0, 0.0);
}

void mycleanup()
{
	for (int i=0; i<4; i++)
		delete[] g_basisMatrix[i];
	delete[] g_basisMatrix;
}

void drawSplines()
{
	// Draw the rail
	for (int i=0; i<g_iNumOfSplines; i++)
	{
		if (g_vVertices[i].size() >= 1)
		{
			glBegin(GL_POLYGON);
				glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
				glVertex3f(g_vRailVertices[i][0].x, g_vRailVertices[i][0].y, g_vRailVertices[i][0].z);
				glVertex3f(g_vRailVertices[i][1].x, g_vRailVertices[i][1].y, g_vRailVertices[i][1].z);
				glVertex3f(g_vRailVertices[i][2].x, g_vRailVertices[i][2].y, g_vRailVertices[i][2].z);
				glVertex3f(g_vRailVertices[i][3].x, g_vRailVertices[i][3].y, g_vRailVertices[i][3].z);
			glEnd();
			for (int j=4; j<g_vRailVertices[i].size(); j+=4)
			{
				glBegin(GL_POLYGON);
					glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
					glVertex3f(g_vRailVertices[i][j].x, g_vRailVertices[i][j].y, g_vRailVertices[i][j].z);
					glVertex3f(g_vRailVertices[i][j+1].x, g_vRailVertices[i][j+1].y, g_vRailVertices[i][j+1].z);
					glVertex3f(g_vRailVertices[i][j+2].x, g_vRailVertices[i][j+2].y, g_vRailVertices[i][j+2].z);
					glVertex3f(g_vRailVertices[i][j+3].x, g_vRailVertices[i][j+3].y, g_vRailVertices[i][j+3].z);
				glEnd();
			}
			
			glBegin(GL_LINE_STRIP);
			for (int j=0; j<g_vVertices[i].size(); j++)
			{
				glColor3f(0.0, 1.0, 0.0);
				glVertex3f(g_vVertices[i][j].x, g_vVertices[i][j].y, g_vVertices[i][j].z);
			}
			glEnd();
			/*
			glBegin(GL_LINE_STRIP);
			for (int j=0; j<g_vCenters[i].size(); j++)
			{
				glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
				glVertex3f(g_vCenters[i][j].x, g_vCenters[i][j].y, g_vCenters[i][j].z);
			}
			glEnd();

			glBegin(GL_LINE_STRIP);
			for (int j=0; j<g_vCenters[i].size(); j++)
			{
				glColor3f(0.0, 0.0, 1.0);
				glVertex3f(g_vVertices[i][j].x + g_vUps[i][j].x, g_vVertices[i][j].y + g_vUps[i][j].y, g_vVertices[i][j].z + g_vUps[i][j].z);
			}
			glEnd();*/
		}
	}
}

void drawSkybox()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_texture[0]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Right
	glBegin(GL_POLYGON);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(1.0, 0.25);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.25);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.5);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0, 0.5);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
	glEnd();
	//Left
	glBegin(GL_POLYGON);
		glTexCoord2f(0.0, 0.25);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.25);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.5);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(0.0, 0.5);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
	glEnd();
	//Back
	glBegin(GL_POLYGON);
		glTexCoord2f(2.0/3.0, 1.0);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1.0/3.0, 1.0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.75);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.75);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
	glEnd();
	//Forward
	glBegin(GL_POLYGON);
		glTexCoord2f(2.0/3.0, 0.25);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.25);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.5);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.5);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
	glEnd();
	//Top
	glBegin(GL_POLYGON);
		glTexCoord2f(2.0/3.0, 0.0);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.25);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.25);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
	glEnd();
	//Bottom
	glBegin(GL_POLYGON);
		glTexCoord2f(2.0/3.0, 0.75);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(2.0/3.0, 0.5);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.5);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1.0/3.0, 0.75);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void display()
{
	// Clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Scale, Rotate, and Translate
	glLoadIdentity();
	
	//g_vLandTranslate[2] = DEBUG_Z_OFFSET;
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
	glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
	glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
	
	gluLookAt(g_vVertices[g_currentSpline][g_currentPointOnSpline].x, 
		g_vVertices[g_currentSpline][g_currentPointOnSpline].y, 
		g_vVertices[g_currentSpline][g_currentPointOnSpline].z,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].x,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].y,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].z,
		g_vUps[g_currentSpline][g_currentPointOnSpline].x,
		g_vUps[g_currentSpline][g_currentPointOnSpline].y,
		g_vUps[g_currentSpline][g_currentPointOnSpline].z);

	g_currentPointOnSpline += SPEED;
	if (g_currentPointOnSpline >= g_vVertices[g_currentSpline].size())
	{
		g_currentPointOnSpline = 0;
		g_currentSpline++;
		if (g_currentSpline >= g_vVertices.size())
			g_currentSpline = 0;
	}

	drawSkybox();
	drawSplines();


  glutSwapBuffers();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'v':
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 'w':
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 's':
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		default:
			break;
	}
}

void reshape(int w, int h)
{
	GLfloat aspect = (GLfloat) w / (GLfloat) h;
	GLfloat nearVal = -10.0;
	GLfloat farVal = 10.0;
	GLfloat xMultiplier = 1.0;
	GLfloat yMultiplier = 1.0;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60.0, 1.0 * w/h, 0.01, 1000.0);

	glMatrixMode(GL_MODELVIEW);
}

void timer(int value)
{
	char fileName[2048];
	sprintf(fileName, "%03d.jpg", value);
	saveScreenshot(fileName);
	glutTimerFunc(1000 / 15, timer, value+1);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to track.txt.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your track file name for the "Command Arguments"
	if (argc<3)
	{  
		printf ("usage: %s <trackfile> <skybox texture>\n", argv[0]);
		exit(0);
	}

	loadSplines(argv[1]);

	glutInit(&argc,(char**)argv);
  
	/* Create a window that is double buffered and uses depth testing */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);
  
	/* allow the user to quit using the right mouse button menu */
	int g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit",0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* callback for animation */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for keyboard input */
	glutKeyboardFunc(keyboard);

	/* reshape callback */
	glutReshapeFunc(reshape);

	/* timer callback */
	/* This is commented so that you can run my code without overwriting
		the animation images. */
	//glutTimerFunc(1000 / 15, timer, 0);

	/* do initialization */
	myinit(argv[2]);

	glEnable(GL_DEPTH_TEST);

	glutMainLoop();

	mycleanup();

	return 0;
}