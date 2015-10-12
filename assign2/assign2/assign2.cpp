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

using namespace std;

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

/* Parallel vectors of vectors of vertices and colors for each spline. */
vector<vector<struct vertex>> g_vVertices;
vector<vector<struct color>> g_vColors;

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

struct vertex {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

struct color {
	GLfloat r;
	GLfloat g;
	GLfloat b;
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

/* tension parameter (s) */
GLfloat g_tension = 0.5;


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

GLfloat lineLengthSquared(struct vertex v0, struct vertex v1)
{
	return (v1.x-v0.x)*(v1.x-v0.x) + 
		(v1.y-v0.y)*(v1.y-v0.y) + 
		(v1.z-v0.z)*(v1.z-v0.z);
}

struct vertex getPointOnSpline(int splineIndex, int controlPointIndex, GLfloat u)
{
	struct vertex v;

	GLfloat* uVector = new GLfloat[4];
	GLfloat** basis = new GLfloat*[4];
	for (int i=0; i<4; i++)
		basis[i] = new GLfloat[4];
	GLfloat** control = new GLfloat*[4];
	for (int i=0; i<4; i++)
		control[i] = new GLfloat[3];
	GLfloat** basisControl = new GLfloat*[4];
	for (int i=0; i<4; i++)
		basisControl[i] = new GLfloat[3];

	// Fill in u vector
	for (int i=0; i<4; i++)
		uVector[i] = pow(u, 3 - i);

	// Fill in basis batrix
	basis[0][0] = -g_tension;
	basis[0][1] = 2.0 - g_tension;
	basis[0][2] = g_tension - 2.0;
	basis[0][3] = g_tension;
	basis[1][0] = 2.0 * g_tension;
	basis[1][1] = g_tension - 3.0;
	basis[1][2] = 3.0 - 2.0 * g_tension;
	basis[1][3] = -g_tension;
	basis[2][0] = -g_tension;
	basis[2][1] = 0;
	basis[2][2] = g_tension;
	basis[2][3] = 0;
	basis[3][0] = 0;
	basis[3][1] = 1.0;
	basis[3][2] = 0;
	basis[3][3] = 0;

	// Fill in control matrix
	for (int i=0; i<4; i++)
	{
		control[i][0] = g_Splines[splineIndex].points[controlPointIndex + i].x;
		control[i][1] = g_Splines[splineIndex].points[controlPointIndex + i].y;
		control[i][2] = g_Splines[splineIndex].points[controlPointIndex + i].z;
	}

	// Fill in basis * control matrix
	for (int i=0; i<4; i++)
	{
		for (int j=0; j<3; j++)
		{
			basisControl[i][j] = basis[i][0] * control[0][j] +
				basis[i][1] * control[1][j] +
				basis[i][2] * control[2][j] +
				basis[i][3] * control[3][j];
		}
	}

	// uVector * basisControl
	v.x = uVector[0] * basisControl[0][0] +
		uVector[1] * basisControl[1][0] +
		uVector[2] * basisControl[2][0] +
		uVector[3] * basisControl[3][0];
	v.y = uVector[0] * basisControl[0][1] +
		uVector[1] * basisControl[1][1] +
		uVector[2] * basisControl[2][1] +
		uVector[3] * basisControl[3][1];
	v.z = uVector[0] * basisControl[0][2] +
		uVector[1] * basisControl[1][2] +
		uVector[2] * basisControl[2][2] +
		uVector[3] * basisControl[3][2];

	delete[] uVector;
	for (int i=0; i<4; i++)
		delete[] basis[i];
	delete[] basis;
	for (int i=0; i<3; i++)
		delete[] control[i];
	delete[] control;
	for (int i=0; i<3; i++)
		delete[] basisControl[i];
	delete[] basisControl;

	return v;
}

void subdivide(int splineIndex, int controlPointIndex, GLfloat u0, GLfloat u1, GLfloat maxLineLength)
{
	GLfloat uMid = (u0 + u1) / 2.0;
	struct vertex v0 = getPointOnSpline(splineIndex, controlPointIndex, u0);
	struct vertex v1 = getPointOnSpline(splineIndex, controlPointIndex, u1);
	if (lineLengthSquared(v0, v1) > maxLineLength*maxLineLength)
	{
		subdivide(splineIndex, controlPointIndex, u0, uMid, maxLineLength);
		subdivide(splineIndex, controlPointIndex, uMid, u1, maxLineLength);
	}
	else
	{
		struct color vertColor;
		vertColor.r = 1.0;
		vertColor.g = 0;
		vertColor.b = 0;
		if (g_vVertices[splineIndex].empty() || g_vVertices[splineIndex].back().x != v0.x ||
			g_vVertices[splineIndex].back().y != v0.y || g_vVertices[splineIndex].back().z != v0.z)
		{
			// Don't repeat vertices
			g_vVertices[splineIndex].push_back(v0);
			g_vColors[splineIndex].push_back(vertColor);
		}
		g_vVertices[splineIndex].push_back(v1);
		g_vColors[splineIndex].push_back(vertColor);
	}
}

void myinit()
{
	g_vLandTranslate[2] = -50.0;

	/* Fill in the parallel vectors of vertices and colors from spline info */
	for (int i=0; i<g_iNumOfSplines; i++)
	{
		g_vVertices.push_back(vector<struct vertex>());
		g_vColors.push_back(vector<struct color>());
		for (int j=0; j<g_Splines[i].numControlPoints-3; j++)
		{
			subdivide(i, j, 0.0, 1.0, 0.01);
		}
	}

  glClearColor(0.0, 0.0, 0.0, 0.0);
}

void drawSplines()
{
	for (int i=0; i<g_iNumOfSplines; i++)
	{
		glBegin(GL_LINE_STRIP);
		for (int j=0; j<g_vVertices[i].size(); j++)
		{
			glColor3f(g_vColors[i][j].r, g_vColors[i][j].g, g_vColors[i][j].b);
			glVertex3f(g_vVertices[i][j].x, g_vVertices[i][j].y, g_vVertices[i][j].z);
		}
		glEnd();
	}
}

void display()
{
	// Clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Scale, Rotate, and Translate
	glLoadIdentity();
	
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
	glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
	glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

	// Draw splines here
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

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
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
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
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

	/* reshape callback */
	glutReshapeFunc(reshape);

	/* timer callback */
	/* This is commented so that you can run my code without overwriting
		the animation images. */
	//glutTimerFunc(1000 / 15, timer, 0);

	/* do initialization */
	myinit();

	glEnable(GL_DEPTH_TEST);

	glutMainLoop();

	return 0;
}