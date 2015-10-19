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

const GLfloat BOX_SIZE = 60.0;
const GLfloat RAIL_SIZE = 0.1;
const GLfloat DISTANCE_BETWEEN_RAILS = 0.6;
const GLfloat CAMERA_OFFSET = 0.4;

int g_speed = 4;

/* keep track of the current polygon mode (this is because when we render the environment we always want it to be GL_FILL) */
GLenum g_currentPolygonMode = GL_FILL;

/* Parallel vectors of vectors of vertices, eye vectors, center vectors, and up vectors for each spline. */
vector<vector<vector3>> g_vVertices;
vector<vector<vector3>> g_vEyes;
vector<vector<vector3>> g_vCenters;
vector<vector<vector3>> g_vUps;

/* Rail vertices */
vector<vector<vector3>> g_vLeftRailVertices;
vector<vector<vector3>> g_vRightRailVertices;

/* Color of the rail */
vector3 g_railColor = vector3(0.0, 1.0, 1.0);

/* Keep track of which spline we are on and how far along it we are */
int g_currentSpline = 0;
int g_currentPointOnSpline = 0;

/* Texture */
GLuint* g_textures;

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
	vector3 tangent = controlPointToSpline(vector3(), splineIndex, controlPointIndex, u, true);
	tangent.normalize();

	vector3 n;
	if (g_vVertices[splineIndex].size() == 1)
	{
		// If this is the first up vector we are computing, we need to pick an arbitrary vector
		vector3 v(0, 1.0, 0);
		n = crossProduct(tangent, v);
	}
	else
	{
		// Otherwise compute the new N using the previous up vector
		n = crossProduct(g_vUps[splineIndex].back(), tangent);
	}
	n.normalize();

	vector3 up = crossProduct(tangent, n);
	up.normalize();

	vector3 eye = g_vVertices[splineIndex].back() + up * CAMERA_OFFSET;

	g_vEyes[splineIndex].push_back(eye);
	g_vCenters[splineIndex].push_back(eye + tangent);
	g_vUps[splineIndex].push_back(up);

	// Precompute vertices for rails based on local coordinate system
	vector3 leftCenterVert = g_vVertices[splineIndex].back() + n * (DISTANCE_BETWEEN_RAILS/2.0);
	vector3 rightCenterVert = g_vVertices[splineIndex].back() - n * (DISTANCE_BETWEEN_RAILS/2.0);
	g_vLeftRailVertices[splineIndex].push_back(leftCenterVert + ((n + up) * RAIL_SIZE)); //Top left
	g_vLeftRailVertices[splineIndex].push_back(leftCenterVert + ((-n + up) * RAIL_SIZE)); //Top right
	g_vLeftRailVertices[splineIndex].push_back(leftCenterVert + ((-n - up) * RAIL_SIZE)); //Bottom right
	g_vLeftRailVertices[splineIndex].push_back(leftCenterVert + ((n - up) * RAIL_SIZE)); //Bottom left
	g_vRightRailVertices[splineIndex].push_back(rightCenterVert + ((n + up) * RAIL_SIZE)); //Top left
	g_vRightRailVertices[splineIndex].push_back(rightCenterVert + ((-n + up) * RAIL_SIZE)); //Top right
	g_vRightRailVertices[splineIndex].push_back(rightCenterVert + ((-n - up) * RAIL_SIZE)); //Bottom right
	g_vRightRailVertices[splineIndex].push_back(rightCenterVert + ((n - up) * RAIL_SIZE)); //Bottom left
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
	glBindTexture(GL_TEXTURE_2D, g_textures[i]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->nx, img->ny, 
		0, GL_RGB, GL_UNSIGNED_BYTE, &img->pix[0]);
	pic_free(img);
}

void myinit(_TCHAR* argv[])
{
	/* Load textures */
	g_textures = new GLuint[6];
	glGenTextures(6, g_textures);
	for (int i=0; i<6; i++)
		texload(i, argv[i+2]);

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
		g_vEyes.push_back(vector<vector3>());
		g_vCenters.push_back(vector<vector3>());
		g_vUps.push_back(vector<vector3>());
		g_vLeftRailVertices.push_back(vector<vector3>());
		g_vRightRailVertices.push_back(vector<vector3>());
		for (int j=0; j<g_Splines[i].numControlPoints-3; j++)
		{
			subdivide(i, j, 0.0, 1.0, (GLfloat)g_Splines[i].numControlPoints / 500);
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

void drawRail(vector<vector<vector3>> railVertices)
{
	// Draw the rail
	for (int i=0; i<g_iNumOfSplines; i++)
	{
		if (g_vVertices[i].size() >= 1)
		{
			// First face
			glBegin(GL_POLYGON);
				glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
				glVertex3f(railVertices[i][0].x, railVertices[i][0].y, railVertices[i][0].z);
				glVertex3f(railVertices[i][1].x, railVertices[i][1].y, railVertices[i][1].z);
				glVertex3f(railVertices[i][2].x, railVertices[i][2].y, railVertices[i][2].z);
				glVertex3f(railVertices[i][3].x, railVertices[i][3].y, railVertices[i][3].z);
			glEnd();
			// Each subsequent set of 4 coplanar vertices has triangles connecting them with the previous ones
			for (int j=4; j<railVertices[i].size(); j+=4)
			{
				glBegin(GL_TRIANGLE_STRIP);
					glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
					// Top
					glVertex3f(railVertices[i][j].x, railVertices[i][j].y, railVertices[i][j].z);
					glVertex3f(railVertices[i][j-4].x, railVertices[i][j-4].y, railVertices[i][j-4].z);
					glVertex3f(railVertices[i][j+1].x, railVertices[i][j+1].y, railVertices[i][j+1].z);
					glVertex3f(railVertices[i][j-3].x, railVertices[i][j-3].y, railVertices[i][j-3].z);
					// Right
					glVertex3f(railVertices[i][j+2].x, railVertices[i][j+2].y, railVertices[i][j+2].z);
					glVertex3f(railVertices[i][j-2].x, railVertices[i][j-2].y, railVertices[i][j-2].z);
					// Bottom
					glVertex3f(railVertices[i][j+3].x, railVertices[i][j+3].y, railVertices[i][j+3].z);
					glVertex3f(railVertices[i][j-1].x, railVertices[i][j-1].y, railVertices[i][j-1].z);
					// Left
					glVertex3f(railVertices[i][j].x, railVertices[i][j].y, railVertices[i][j].z);
					glVertex3f(railVertices[i][j-4].x, railVertices[i][j-4].y, railVertices[i][j-4].z);
				glEnd();
			}
			// Last face
			glBegin(GL_POLYGON);
				glColor3f(g_railColor.x, g_railColor.y, g_railColor.z);
				glVertex3f(railVertices[i][railVertices[i].size()-4].x, railVertices[i][railVertices[i].size()-4].y, railVertices[i][railVertices[i].size()-4].z);
				glVertex3f(railVertices[i][railVertices[i].size()-3].x, railVertices[i][railVertices[i].size()-3].y, railVertices[i][railVertices[i].size()-3].z);
				glVertex3f(railVertices[i][railVertices[i].size()-2].x, railVertices[i][railVertices[i].size()-2].y, railVertices[i][railVertices[i].size()-2].z);
				glVertex3f(railVertices[i][railVertices[i].size()-1].x, railVertices[i][railVertices[i].size()-1].y, railVertices[i][railVertices[i].size()-1].z);
			glEnd();
		}
	}
}

void drawSkybox()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
	
	//Top
	glBindTexture(GL_TEXTURE_2D, g_textures[0]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(0, 0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
	glEnd();
	//Bottom
	glBindTexture(GL_TEXTURE_2D, g_textures[1]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
	glEnd();
	//Left
	glBindTexture(GL_TEXTURE_2D, g_textures[2]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
	glEnd();
	//Right
	glBindTexture(GL_TEXTURE_2D, g_textures[3]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
	glEnd();
	//Front
	glBindTexture(GL_TEXTURE_2D, g_textures[4]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
		glVertex3f(-BOX_SIZE, BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(BOX_SIZE, -BOX_SIZE, -BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(BOX_SIZE, BOX_SIZE, -BOX_SIZE);
	glEnd();
	//Back
	glBindTexture(GL_TEXTURE_2D, g_textures[5]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
		glVertex3f(-BOX_SIZE, -BOX_SIZE, BOX_SIZE);
		glTexCoord2f(0, 1);
		glVertex3f(-BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 1);
		glVertex3f(BOX_SIZE, BOX_SIZE, BOX_SIZE);
		glTexCoord2f(1, 0);
		glVertex3f(BOX_SIZE, -BOX_SIZE, BOX_SIZE);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, g_currentPolygonMode);
}

void display()
{
	// Clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Scale, Rotate, and Translate
	glLoadIdentity();
	
	gluLookAt(g_vEyes[g_currentSpline][g_currentPointOnSpline].x, 
		g_vEyes[g_currentSpline][g_currentPointOnSpline].y, 
		g_vEyes[g_currentSpline][g_currentPointOnSpline].z,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].x,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].y,
		g_vCenters[g_currentSpline][g_currentPointOnSpline].z,
		g_vUps[g_currentSpline][g_currentPointOnSpline].x,
		g_vUps[g_currentSpline][g_currentPointOnSpline].y,
		g_vUps[g_currentSpline][g_currentPointOnSpline].z);
		
	// Advance the roller coaster
	g_currentPointOnSpline += g_speed;
	if (g_currentPointOnSpline >= g_vVertices[g_currentSpline].size())
	{
		g_currentPointOnSpline = 0;
		g_currentSpline++;
		if (g_currentSpline >= g_vVertices.size())
			g_currentSpline = 0;
	}

	drawSkybox();
	drawRail(g_vLeftRailVertices);
	drawRail(g_vRightRailVertices);

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

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'v':
			g_currentPolygonMode = GL_POINT;
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 'w':
			g_currentPolygonMode = GL_LINE;
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 's':
			g_currentPolygonMode = GL_FILL;
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case 'u':
			g_speed++;
			break;
		case 'j':
			if (g_speed > 0)
				g_speed--;
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
	if (value < 999)
		glutTimerFunc(1000 / 15, timer, value+1);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to track.txt.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your track file name for the "Command Arguments"
	if (argc<8)
	{  
		printf ("usage: %s <trackfile> <skybox top texture> <skybox bottom texture> <skybox left texture> <skybox right texture> <skybox front texture> <skybox back texture>\n", argv[0]);
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

	/* callback for keyboard input */
	glutKeyboardFunc(keyboard);

	/* reshape callback */
	glutReshapeFunc(reshape);

	/* timer callback */
	/* This is commented so that you can run my code without overwriting
		the animation images. */
	//glutTimerFunc(1000 / 15, timer, 0);

	/* do initialization */
	myinit(argv);

	glEnable(GL_DEPTH_TEST);

	glutMainLoop();

	mycleanup();

	return 0;
}