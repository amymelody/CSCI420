/*
CSCI 480
Assignment 3 Raytracer

Name: <Your name here>
*/

#define _USE_MATH_DEFINES

#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "vector3.h"

#include <stdio.h>
#include <string>
#include <cmath>
#include <float.h>

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename=0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode=MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 320 //Change to 640 later
#define HEIGHT 240 //Change to 480 later

//the field of view of the camera
#define fov 60.0

using namespace std;

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

typedef struct _Triangle
{
  struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  double position[3];
  double color[3];
} Light;

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles=0;
int num_spheres=0;
int num_lights=0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

double checkSphereIntersection(vector3 start, vector3 direction)
{
	double minT = DBL_MAX;

	for (int i=0; i<num_spheres; i++)
	{
		double b = 2.0 * (direction.x*(start.x-spheres[i].position[0]) +
			direction.y*(start.y-spheres[i].position[1]) +
			direction.z*(start.z-spheres[i].position[2]));
		double c = (start.x-spheres[i].position[0]) * (start.x-spheres[i].position[0]) +
			(start.y-spheres[i].position[1]) * (start.y-spheres[i].position[1]) +
			(start.z-spheres[i].position[2]) * (start.z-spheres[i].position[2])
			- spheres[i].radius * spheres[i].radius;
		
		double discriminant = b*b - 4*c;
		if (discriminant >= 0)
		{
			double t0 = (-b - sqrt(discriminant))/2.0;
			double t1 = (-b + sqrt(discriminant))/2.0;
			if (t0 > 0 && t0 < minT)
				minT = t0;
			else if (t1 > 0 && t1 < minT)
				minT = t1;
		}
	}

	if (minT == DBL_MAX)
		minT = -1.0;

	return minT;
}

double checkTriangleIntersection(vector3 start, vector3 direction)
{
	double minT = DBL_MAX;

	for (int i=0; i<num_triangles; i++)
	{
		//Ray-plane intersection:
		//Ray: r(t) = r0 + dt
		//Plane: p dot n = p0 dot n  (where p0 is any arbitrary point on the plane)
		//Intersection: (r0 + dt) dot n = p0 dot n
		// => t = ((p0 dot n) - (r0 dot n)) / (d dot n)
		vector3 a(triangles[i].v[0].position[0],
			triangles[i].v[0].position[1],
			triangles[i].v[0].position[2]);
		vector3 b(triangles[i].v[1].position[0],
			triangles[i].v[1].position[1],
			triangles[i].v[1].position[2]);
		vector3 c(triangles[i].v[2].position[0],
			triangles[i].v[2].position[1],
			triangles[i].v[2].position[2]);
		vector3 n = crossProduct(b-a, c-a);

		double dDotN = dotProduct(direction, n);
		if (dDotN != 0)
		{
			double t = (dotProduct(a, n)-dotProduct(start, n)) / dDotN;
			if (t > 0 && t < minT)
			{
				//To check if point P lies in triangle ABC:
				//Take cross products (B-A)x(P-A), (C-B)x(P-B), (A-C)x(P-C)
				//If they all point in the same direction relative to the plane,
				//then P lies inside triangle ABC.
				vector3 p = start + direction * t;
				vector3 crossA = crossProduct(b-a, p-a);
				vector3 crossB = crossProduct(c-b, p-b);
				vector3 crossC = crossProduct(a-c, p-c);
				if (dotProduct(crossA, crossB) > 0 && dotProduct(crossA, crossC) > 0)
				{
					//They all point in the same direction, so P is inside the triangle!
					minT = t;
				}
			}
		}
	}

	if (minT == DBL_MAX)
		minT = -1.0;

	return minT;
}

//MODIFY THIS FUNCTION
void draw_scene()
{
	//camera position (0,0,0)
	//look at (0,0,-1)
	//up vector (0,1,0)
	//near plane: z=-1
	//fov: 60 degrees
	//a = aspect ratio = w/h

	//send out one ray for each pixel
	double aspectRatio = (double)WIDTH / (double)HEIGHT;
	double xMin = -aspectRatio * tan((fov/2) * M_PI / 180.0);
	double xMax = aspectRatio * tan((fov/2) * M_PI / 180.0);
	double yMin = -tan((fov/2) * M_PI / 180.0);
	double yMax = tan((fov/2) * M_PI / 180.0);

	unsigned int x,y;
	double currentX = xMin;

	for (x=0; x<WIDTH; x++)
	{
		double currentY = yMin;
		glPointSize(2.0);  
		glBegin(GL_POINTS);
		for (y=0; y<HEIGHT; y++)
		{
			vector3 direction(currentX, currentY, -1);
			direction.normalize();
			double t;
			double sphereT = checkSphereIntersection(vector3(0, 0, 0), direction);
			double triangleT = checkTriangleIntersection(vector3(0, 0, 0), direction);
			vector3 color(0,0,0);

			if ((sphereT > 0 && sphereT < triangleT) || triangleT <= 0) 
			{
				t = sphereT;
				color.x = 255;
			} 
			else 
			{
				t = triangleT;
				color.y = 255;
			}
			
			if (t > 0)
				plot_pixel(x,y,color.x,color.y,color.z);
			else
				plot_pixel(x,y,255,255,255);
			
			currentY += (yMax-yMin)/(double)(HEIGHT-1);
		}
		glEnd();
		glFlush();
		currentX += (xMax-xMin)/(double)(WIDTH-1);
	}

  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}

void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  Pic *in = NULL;

  in = pic_alloc(640, 480, 3, NULL);
  printf("Saving JPEG file: %s\n", filename);

  memcpy(in->pix,buffer,3*WIDTH*HEIGHT);
  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);      

}

void parse_check(char *expected,char *found)
{
  if(stricmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}

void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE *file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  int i;
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i",&number_of_objects);

  printf("number of objects: %i\n",number_of_objects);
  char str[200];

  parse_doubles(file,"amb:",ambient_light);

  for(i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
      if(stricmp(type,"triangle")==0)
	{

	  printf("found triangle\n");
	  int j;

	  for(j=0;j < 3;j++)
	    {
	      parse_doubles(file,"pos:",t.v[j].position);
	      parse_doubles(file,"nor:",t.v[j].normal);
	      parse_doubles(file,"dif:",t.v[j].color_diffuse);
	      parse_doubles(file,"spe:",t.v[j].color_specular);
	      parse_shi(file,&t.v[j].shininess);
	    }

	  if(num_triangles == MAX_TRIANGLES)
	    {
	      printf("too many triangles, you should increase MAX_TRIANGLES!\n");
	      exit(0);
	    }
	  triangles[num_triangles++] = t;
	}
      else if(stricmp(type,"sphere")==0)
	{
	  printf("found sphere\n");

	  parse_doubles(file,"pos:",s.position);
	  parse_rad(file,&s.radius);
	  parse_doubles(file,"dif:",s.color_diffuse);
	  parse_doubles(file,"spe:",s.color_specular);
	  parse_shi(file,&s.shininess);

	  if(num_spheres == MAX_SPHERES)
	    {
	      printf("too many spheres, you should increase MAX_SPHERES!\n");
	      exit(0);
	    }
	  spheres[num_spheres++] = s;
	}
      else if(stricmp(type,"light")==0)
	{
	  printf("found light\n");
	  parse_doubles(file,"pos:",l.position);
	  parse_doubles(file,"col:",l.color);

	  if(num_lights == MAX_LIGHTS)
	    {
	      printf("too many lights, you should increase MAX_LIGHTS!\n");
	      exit(0);
	    }
	  lights[num_lights++] = l;
	}
      else
	{
	  printf("unknown type in scene description:\n%s\n",type);
	  exit(0);
	}
    }
  return 0;
}

void display()
{

}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
      draw_scene();
      if(mode == MODE_JPEG)
	save_jpg();
    }
  once=1;
}

int main (int argc, char ** argv)
{
  if (argc<2 || argc > 3)
  {  
    printf ("usage: %s <scenefile> [jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
    {
      mode = MODE_JPEG;
      filename = argv[2];
    }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
