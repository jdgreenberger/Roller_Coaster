/*
 CSCI 480
 Assignment 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include "math.h"
#include <iostream>
using namespace std;

int g_iMenuId;
int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;
Pic * g_pHeightData;
static GLuint texName;
GLuint texture[5];
string fileName;

typedef enum { ROTATE, TRANSLATE, SCALE} CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

/* represents one control point along the spline */
struct point {
    double x;
    double y;
    double z;
};

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};
bool playCoaster = false;
float heightmap [20] = {0.0, 3.0, 5.0, -4.0, 4.0, 2.0, 0.0, 3.0, 2.0, -1.0};
struct point eyePoint;
struct point centerPoint;
struct point upVector;

/* spline struct which contains how many control points, and an array of control points */
struct spline {
    int numControlPoints;
    struct point *points;
};

struct point CatmullRoll(float t, struct point p1, struct point p2, struct point p3, struct point p4)
{
    
	float t2 = t*t;
	float t3 = t*t*t;
	struct point v; // Interpolated point
    
	/* Catmull Rom spline Calculation */
    
	v.x = ((-t3 + 2*t2-t)*(p1.x) + (3*t3-5*t2+2)*(p2.x) + (-3*t3+4*t2+t)* (p3.x) + (t3-t2)*(p4.x))/2;
	v.y = ((-t3 + 2*t2-t)*(p1.y) + (3*t3-5*t2+2)*(p2.y) + (-3*t3+4*t2+t)* (p3.y) + (t3-t2)*(p4.y))/2;
    v.z = ((-t3 + 2*t2-t)*(p1.z) + (3*t3-5*t2+2)*(p2.z) + (-3*t3+4*t2+t)* (p3.z) + (t3-t2)*(p4.z))/2;
//	printf("Values of v.x = %f and v.y = %f\n", v.x,v.y);
    
	return v;
}

struct point CatmullRollDeriv(float t, struct point p1, struct point p2, struct point p3, struct point p4)
{
    
	float t2 = t*t;
	float t3 = t*t*t;
	struct point v; // Interpolated point
    
	/* Catmull Rom spline Calculation */
    
	v.x = ((-3*t2 + 4*t-1)*(p1.x) + (9*t2-10*t)*(p2.x) + (-9*t2+8*t+1)* (p3.x) + (3*t2-2*t)*(p4.x))/2;
    v.y = ((-3*t2 + 4*t-1)*(p1.y) + (9*t2-10*t)*(p2.y) + (-9*t2+8*t+1)* (p3.y) + (3*t2-2*t)*(p4.y))/2;
    v.z = ((-3*t2 + 4*t-1)*(p1.z) + (9*t2-10*t)*(p2.z) + (-9*t2+8*t+1)* (p3.z) + (3*t2-2*t)*(p4.z))/2;

    //Tangent normal computed by diving by length
    v.x /= p3.x-p2.x;
    v.y /= p3.y-p2.y;
    v.z /= p3.z-p2.z;
	return v;
}

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

void texload(int i,char *filename)
{
    /* Texture Mapping Setup */

    Pic* img;
    img = jpeg_read(filename, NULL);
    glBindTexture(GL_TEXTURE_2D, texture[i]);
    
    // specify texture parameters (they affect whatever texture is active)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // repeat pattern in s
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // repeat pattern in t
    // use linear filter both for magnification and minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 img->nx,
                 img->ny,
                 0, 
                 GL_RGB, 
                 GL_UNSIGNED_BYTE, 
                 &img->pix[0]); 
    pic_free(img); 
}

void myinit()
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);            // enable depth buffering
    
    /* setup gl view here */
    glShadeModel(GL_SMOOTH); //Set shader mode to smooth
    
    //Initial LookAt View
    
    //Up Vector
    upVector.x = 0.0; upVector.y = 1.0; upVector.z = 0.0;
    
    //Eye Point
    eyePoint.x = -50.0; eyePoint.y = 0.0; eyePoint.z = 20.0;
    
    //Center Point
    centerPoint.x = 0.0; centerPoint.y = 0.0; centerPoint.z = 20.0;
}

void setImagePixel (int x, int y, float image_height, float image_width){
    //xImage & yImage are coordinates we want to project.
    //Adjusted in negative direction to center on screen
    float xImage = ((float)x - image_width/2)/image_width * 50;
    float yImage = (((float)y - image_height/2)/image_height * 50);
  
    glTexCoord2f((float) x/image_height, (float) y/image_height);
    glVertex3f(xImage, 0, yImage);
}

void drawScene (){
    /* Draw Ground */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    
    for (int x = -100; x <= 80; x = x+50){
        for (int y = -100; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(x, 0.0, y+50);
            glTexCoord2f(0.0, 0.0); glVertex3f(x, 0.0, y);
            glTexCoord2f(1.0, 0.0); glVertex3f(x+50, 0.0, y);
            glTexCoord2f(1.0, 1.0); glVertex3f(x+50, 0.0, y+50);
            glEnd();
        }
    }
//
//    glBegin(GL_QUADS);
//    glTexCoord2f(0.0, 1.0); glVertex3f(-100, 0.0, 100);
//    glTexCoord2f(0.0, 0.0); glVertex3f(-100, 0.0, -100);
//    glTexCoord2f(1.0, 0.0); glVertex3f(100, 0.0, -100);
//    glTexCoord2f(1.0, 1.0); glVertex3f(100, 0.0, 100);
//    glEnd();
    
    /* Draw Top Sky */
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    
    for (int x = -100; x <= 80; x = x+50){
        for (int y = -100; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(x, 100.0, y+50);
            glTexCoord2f(0.0, 0.0); glVertex3f(x, 100.0, y);
            glTexCoord2f(1.0, 0.0); glVertex3f(x+50, 100.0, y);
            glTexCoord2f(1.0, 1.0); glVertex3f(x+50, 100.0, y+50);
            glEnd();
        }
    }
    
    /* Draw Back Sky */
    for (int x = -100; x <= 80; x = x+50){
        for (int y = 0; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(x+50, y, -100);
            glTexCoord2f(0.0, 0.0); glVertex3f(x, y, -100);
            glTexCoord2f(1.0, 0.0); glVertex3f(x, y+50, -100);
            glTexCoord2f(1.0, 1.0); glVertex3f(x+50, y+50, -100);
            glEnd();
        }
    }
    
    /* Draw Front Sky */
    for (int x = -100; x <= 80; x = x+50){
        for (int y = 0; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(x+50, y, 100);
            glTexCoord2f(0.0, 0.0); glVertex3f(x, y, 100);
            glTexCoord2f(1.0, 0.0); glVertex3f(x, y+50, 100);
            glTexCoord2f(1.0, 1.0); glVertex3f(x+50, y+50, 100);
            glEnd();
        }
    }
    
    /* Draw Left Sky */
    for (int x = -100; x <= 80; x = x+50){
        for (int y = 0; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(-100, y+50, x);
            glTexCoord2f(0.0, 0.0); glVertex3f(-100, y, x);
            glTexCoord2f(1.0, 0.0); glVertex3f(-100, y, x+50);
            glTexCoord2f(1.0, 1.0); glVertex3f(-100, y+50, x+50);
            glEnd();
        }
    }
    
    /* Draw Right Sky */
    for (int x = -100; x <= 80; x = x+50){
        for (int y = 0; y <=80; y = y+50){
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(100, y+50, x);
            glTexCoord2f(0.0, 0.0); glVertex3f(100, y, x);
            glTexCoord2f(1.0, 0.0); glVertex3f(100, y, x+50);
            glTexCoord2f(1.0, 1.0); glVertex3f(100, y+50, x+50);
            glEnd();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}
float tVal = 0;
int counter = 0;

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glEnable(GL_LINE_SMOOTH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //Set viewer, object, and camera perspective
    
    gluLookAt( eyePoint.x, eyePoint.y+3, eyePoint.z, centerPoint.x, centerPoint.y, centerPoint.z, upVector.x, upVector.y, upVector.z);
    
    glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);    //scale based on x, y, z values
    glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]); //translate object
    glRotatef (g_vLandRotate[1], 0.0, 1.0, 0.0);    //rotate y dimension
    glRotatef (g_vLandRotate[0], 1.0, 0.0, 0.0);    //rotate x dimension
    glRotatef (g_vLandRotate[2], 0.0, 0.0, 1.0);    //rotate z dimension

    float t;
    struct point v1, v2;	//Interpolated point
    struct point p1,p2,p3,p4;
    struct point normal, binormal, tangent;
    float vectorLength;
    
    if (counter == 0){
        //Starting normal vector is calculated from tangent vector and arbitrary starting Vector V
        struct point startV;
        startV.x = 1;  startV.y = 1;  startV.z = 1;
        
        //Tangent Vector Computation ... Using first two points of the spline
        tangent.x = g_Splines[0].points[1].x - g_Splines[0].points[0].x;
        tangent.y = g_Splines[0].points[1].y - g_Splines[0].points[0].y;
        tangent.z = g_Splines[0].points[1].z - g_Splines[0].points[0].z;
        vectorLength = sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
        tangent.x = tangent.x/vectorLength;       //normalize
        tangent.y = tangent.y/vectorLength;       //normalize
        tangent.z = tangent.z/vectorLength;       //normalize
        
        //Normal Vector Computation
        normal.x = tangent.x * startV.x;
        normal.y = tangent.y * startV.y;
        normal.z = tangent.z * startV.z;
        vectorLength = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x = normal.x/vectorLength;       //normalize
        normal.y = normal.y/vectorLength;       //normalize
        normal.z = normal.z/vectorLength;       //normalize
        
//        normal.x = 0.0;
//        normal.y = 1.0;
//        normal.z = 0.0;
        
        //Binormal Vector Computation
        binormal.x = tangent.x * normal.x;
        binormal.y = tangent.y * normal.y;
        binormal.z = tangent.z * normal.z;
        vectorLength = sqrt(binormal.x * binormal.x + binormal.y * binormal.y + binormal.z * binormal.z);
        binormal.x = binormal.x/vectorLength;       //normalize
        binormal.y = binormal.y/vectorLength;       //normalize
        binormal.z = binormal.z/vectorLength;       //normalize
    }

    drawScene();
    
    if (playCoaster){
        if (tVal >= 0.98){
            tVal = 0;
            counter++;
           // cout << "Normal vals = " << normal.x << " " << normal.y << " " << normal.z << endl;
           // cout << "Tangent vals = " << tangent.x << " " << tangent.y << " " << tangent.z << endl;
           // cout << "BiNormal vals = " << binormal.x << " " << binormal.y << " " << binormal.z << endl;
           // cout.flush();
        }
        else {
            tVal += 0.02;
        }
        
        p1 = g_Splines[0].points[counter];
        p2 = g_Splines[0].points[counter+1];
        p3 = g_Splines[0].points[counter+2];
        p4 = g_Splines[0].points[counter+3];
     //   p5 = g_Splines[0].points[counter+4];
        
        v1 = CatmullRoll(tVal,p1,p2,p3,p4);
      //  v2 = CatmullRoll(tVal,p2,p3,p4,p5);
        
        v2 = CatmullRoll(tVal + 0.02, p1, p2, p3, p4);
        
        eyePoint.x = v1.x;
        eyePoint.y = v1.y;
        eyePoint.z = v1.z;
        
        //Tangent Vector Computation ... Using first two points of the spline
        tangent.x = v2.x - v1.x;
        tangent.y = v2.y - v1.y;
        tangent.z = v2.z - v1.z;
        vectorLength = sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
        tangent.x = tangent.x/vectorLength;       //normalize
        tangent.y = tangent.y/vectorLength;       //normalize
        tangent.z = tangent.z/vectorLength;       //normalize

        cout << "Normal vals = " << normal.x << " " << normal.y << " " << normal.z << endl;
        cout << "Tangent vals = " << tangent.x << " " << tangent.y << " " << tangent.z << endl;
        cout << "BiNormal vals = " << binormal.x << " " << binormal.y << " " << binormal.z << endl<<endl;
        cout.flush();

        
        centerPoint.x = v1.x + tangent.x*10;
        centerPoint.y = v1.y + tangent.y*10;
        centerPoint.z = v1.z + tangent.z*10;
        
        //Compute new up vector (normal vector) based on previous vector
        
        //Normal Vector Computation
        normal.x = tangent.x * binormal.x;
        normal.y = tangent.y * binormal.y;
        normal.z = tangent.z * binormal.z;
        vectorLength = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x = normal.x/vectorLength;       //normalize
        normal.y = normal.y/vectorLength;       //normalize
        normal.z = normal.z/vectorLength;       //normalize
        
        //Binormal Vector Computation
        binormal.x = tangent.x * normal.x;
        binormal.y = tangent.y * normal.y;
        binormal.z = tangent.z * normal.z;
        vectorLength = sqrt(binormal.x * binormal.x + binormal.y * binormal.y + binormal.z * binormal.z);
        binormal.x = binormal.x/vectorLength;       //normalize
        binormal.y = binormal.y/vectorLength;       //normalize
        binormal.z = binormal.z/vectorLength;       //normalize
        
//        upVector.x = normal.x;
//        upVector.y = normal.y;
//        upVector.z = normal.z;
    }

    glLineWidth(5);
    glBegin(GL_LINE_STRIP);
        for (int i = 0; i < g_Splines[0].numControlPoints-3; i++){
        glColor3d(0.4, 0.0, 0.0);
        p1 = g_Splines[0].points[i];
        p2 = g_Splines[0].points[i+1];
        p3 = g_Splines[0].points[i+2];
        p4 = g_Splines[0].points[i+3];
        
        for(t=0;t<1;t+=0.02)
        {
            v1 = CatmullRoll(t,p1,p2,p3,p4);
           // cout << v1.x << endl; cout.flush();
          //  glVertex3f(v.x-15, v.z+1, v.y-15);
            glVertex3f(v1.x, v1.y+1, v1.z);
            
        }
        
//        glVertex3f(g_Splines[0].points[i].x,g_Splines[0].points[i].y, g_Splines[0].points[i].z);
//        glVertex3f(g_Splines[0].points[i+1].x,g_Splines[0].points[i+1].y, g_Splines[0].points[i+1].z);
    }
    glColor3d(1.0, 1.0, 1.0);
     glEnd();
    
    glutSwapBuffers(); // double buffer flush
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
    
    g_iNumOfSplines = 1;
    /* reads through the spline files */
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        fileSpline = fopen("/Users/joshgreenberger/Documents/Graphics/assign2/Xcode/Assign2/splines/myCoaster.sp", "r");
        
        if (fileSpline == NULL) {
            printf ("can't open file\n");
            exit(1);
        }
        
        /* gets length for spline file */
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        /* allocate memory for all the points */
        g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
        g_Splines[j].numControlPoints = iLength;
        
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

void doIdle()
{
    /* do some stuff... */
    
    /* make the screen update */
    glutPostRedisplay();
}

/*
 Reshape Function called before display
 Also set's initial values
 */
void reshape(int w, int h)
{
    // setup image size
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);    //projection model
    glLoadIdentity();
    gluPerspective(60, 640/480, 0.01, 1000.0);  //dimensions of screen and z buffer
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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
    // char * saveFile;
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

/* takes keyboard input for extra functionality
 */

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case '1':
            playCoaster = true;
            break;
        case '2':
            playCoaster = false;
            break;
        default:
            break;
    }
}

int main (int argc, char ** argv)
{
    fileName = "./screenshots/";
    
    if (argc<2)
    {
        printf ("usage: %s heightfield.jpg\n", argv[0]);
        exit(1);
    }
    
//    g_pHeightData = jpeg_read(argv[1], NULL);
//    if (!g_pHeightData)
//    {
//        printf ("error reading %s.\n", argv[1]);
//        exit(1);
//    }
    loadSplines(argv[1]);
   
    glutInit(&argc,argv);
    
    //Window Initialization
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH|GLUT_RGB);
    glutInitWindowSize(640,480);
    glutInitWindowPosition(100,100);
    glutCreateWindow(argv[0]);
    
    /* do initialization */
    
    glGenTextures(1, texture);
    texload(0,argv[1]);
    texload(1,argv[2]);
    
        myinit();
    
    // GLUT callbacks
    
    /* tells glut to use a particular display function to redraw */
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    
    
    /* allow the user to quit using the right mouse button menu */
  //  glutSetMenu(g_iMenuId);
  //  glutAddMenuEntry("Quit",0);
  //  glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    /* replace with any animate code */
    glutIdleFunc(doIdle);
    
    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);
    
    //Callback for keyboard input
    glutKeyboardFunc(keyboard);
  
  glutMainLoop();
    return(0);
}
