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

/* represents one control point along the spline */
struct point {
    double x;
    double y;
    double z;
};

struct vector: public point{
    static vector cross_product (vector v1, vector v2) {
        vector v3;
        v3.x = v1.y*v2.z - v1.z*v2.y;
        v3.y = v2.x*v1.z - v1.x*v2.z;
        v3.z = v1.x*v2.y - v1.y*v2.x;
        return v3;
    }
    
    void normalize (){
        double vectorLength = sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
        this->x = this->x/vectorLength;
        this->y = this->y/vectorLength;
        this->z = this->z/vectorLength;
    }
};

int g_iMenuId;
int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;
GLuint texture[3];
GLUquadricObj *sphere = NULL;
string fileName;

typedef enum { ROTATE, TRANSLATE, SCALE} CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;


/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

bool playCoaster = false;
float heightmap [20] = {0.0, 3.0, 5.0, -4.0, 4.0, 2.0, 0.0, 3.0, 2.0, -1.0};
struct point eyePoint;
struct point centerPoint;
struct point upVector;
struct vector normal, binormal, tangent;
struct vector normal2, binormal2, tangent2;

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
    v.x = 0.5 * ((2*p2.x) + (-p1.x+p3.x) * t + (2*p1.x - 5*p2.x + 4*p3.x - p4.x) * t2 + (-p1.x+3*p2.x-3*p3.x + p4.x) * t3);
    v.y = 0.5 * ((2*p2.y) + (-p1.y+p3.y) * t + (2*p1.y - 5*p2.y + 4*p3.y - p4.y) * t2 + (-p1.y+3*p2.y-3*p3.y + p4.y) * t3);
    v.z = 0.5 * ((2*p2.z) + (-p1.z+p3.z) * t + (2*p1.z - 5*p2.z + 4*p3.z - p4.z) * t2 + (-p1.z+3*p2.z-3*p3.z + p4.z) * t3);
                 
	return v;
}

struct vector CatmullRollDeriv(float t, struct point p1, struct point p2, struct point p3, struct point p4)
{

	float t2 = t*t;
	struct vector v; // Interpolated point

	/* Catmull Rom spline Calculation */

	v.x = ((-3*t2 + 4*t-1)*(p1.x) + (9*t2-10*t)*(p2.x) + (-9*t2+8*t+1)* (p3.x) + (3*t2-2*t)*(p4.x))/2;
    v.y = ((-3*t2 + 4*t-1)*(p1.y) + (9*t2-10*t)*(p2.y) + (-9*t2+8*t+1)* (p3.y) + (3*t2-2*t)*(p4.y))/2;
    v.z = ((-3*t2 + 4*t-1)*(p1.z) + (9*t2-10*t)*(p2.z) + (-9*t2+8*t+1)* (p3.z) + (3*t2-2*t)*(p4.z))/2;
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
    
    //Sphere
    sphere = gluNewQuadric();
    gluQuadricDrawStyle(sphere, GLU_FILL);
    gluQuadricTexture(sphere, GL_TRUE);
    gluQuadricNormals(sphere, GLU_SMOOTH);
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
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    
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
    
    //SPHERE
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    gluSphere(sphere, 200.0, 50, 50);
        glDisable(GL_TEXTURE_2D);
}
float tVal = 0;
int counter = 0;

void set_start_vector (point v1, point v2, vector tangent, vector &normal, vector &binormal){
    
    //Starting normal vector is calculated from tangent vector and arbitrary starting Vector V
    struct vector startV;
    startV.x = -1;  startV.y = 1;  startV.z = -1;
    
    //Normal Vector Computation
    normal = vector::cross_product(tangent, startV);
    normal.normalize();
    
    //Binormal Vector Computation
    binormal = vector::cross_product(tangent, normal);
    binormal.normalize();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LINE_SMOOTH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    //Set viewer, object, and camera perspective
    gluLookAt( eyePoint.x, eyePoint.y+3, eyePoint.z,
              centerPoint.x, centerPoint.y, centerPoint.z,
              upVector.x, upVector.y, upVector.z);
    
    glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);    //scale based on x, y, z values
    glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]); //translate object
    glRotatef (g_vLandRotate[1], 0.0, 1.0, 0.0);    //rotate y dimension
    glRotatef (g_vLandRotate[0], 1.0, 0.0, 0.0);    //rotate x dimension
    glRotatef (g_vLandRotate[2], 0.0, 0.0, 1.0);    //rotate z dimension
    
    float t;
    struct point v1, v2;	//Interpolated point
    struct point p1,p2,p3,p4;
    double point_length, step_size = 0.02;
    
    drawScene();
    
    if (playCoaster){
        if (tVal >= 1 - step_size){
            tVal = 0;
            counter++;
        }
        else {
            tVal += step_size;
        }
        
        p1 = g_Splines[0].points[counter];
        p2 = g_Splines[0].points[counter+1];
        p3 = g_Splines[0].points[counter+2];
        p4 = g_Splines[0].points[counter+3];
        
//        point_length = sqrt(pow(p3.x-p2.x, 2) + pow(p3.y-p2.y, 2) + pow(p3.z-p2.z, 2));
//        step_size = 2/point_length;
        
        v1 = CatmullRoll(tVal,p1,p2,p3,p4);
        v2 = CatmullRoll(tVal + step_size, p1, p2, p3, p4);
        
        eyePoint.x = v1.x;
        eyePoint.y = v1.y;
        eyePoint.z = v1.z;
        
        //Tangent Vector Computation
        tangent = CatmullRollDeriv(tVal, p1, p2, p3, p4);
        tangent.normalize();
        
        centerPoint.x = v1.x + tangent.x*10;
        centerPoint.y = v1.y + tangent.y*10;
        centerPoint.z = v1.z + tangent.z*10;
        
        if (counter == 0 && tVal <= 0.02) {
            set_start_vector(v1, v2, tangent, normal, binormal);
        }
        else {
            //Compute new up vector (normal vector) based on previous vector
            
            //Normal Vector Computation
            normal = vector::cross_product(binormal, tangent);
            normal.normalize();
            
            //Binormal Vector Computation
            binormal = vector::cross_product(tangent, normal);
            binormal.normalize();
            
            upVector.x = normal.x/30;
            upVector.y = normal.y;
            upVector.z = normal.z/10;
        }
    }
    
    //DRAW TRACK
    
    //LEFT RAIL
    for (int i = 0; i < g_Splines[0].numControlPoints-3; i++){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

        glColor3d(0.4, 0.0, 0.0);
        p1 = g_Splines[0].points[i];
        p2 = g_Splines[0].points[i+1];
        p3 = g_Splines[0].points[i+2];
        p4 = g_Splines[0].points[i+3];
        int draw_cross = 0, draw_supports = 0;
        for(t=0;t<1;t+=0.02)
        {
            v1 = CatmullRoll(t,p1,p2,p3,p4);
            v2 = CatmullRoll(t + 0.02, p1, p2, p3, p4);
            
            //Tangent Vector Computation ... Using first two points of the spline
            tangent2 = CatmullRollDeriv(t, p1, p2, p3, p4);
            tangent2.normalize();
            if (i == 0 &&  t == 0) {
                set_start_vector(v1, v2, tangent2, normal2, binormal2);
            }
            else {
            
            //Compute new up vector (normal vector) based on previous vector
            
            //Normal Vector Computation
            normal2 = vector::cross_product(binormal2, tangent2);
            normal2.normalize();
            
            //Binormal Vector Computation
            binormal2 = vector::cross_product(tangent2, normal2);
            binormal2.normalize();
            }
            
            //LEFT RAIL
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0);
            glVertex3f(v1.x - binormal2.x, v1.y+1 - binormal2.y, v1.z - binormal2.z);
            glTexCoord2f(0.0, 1.0);
            glVertex3f(v2.x - binormal2.x, v2.y+1 - binormal2.y, v2.z - binormal2.z);
            glTexCoord2f(1.0, 1.0);
            glVertex3f(v2.x - binormal2.x + 0.1, v2.y+1 - binormal2.y + 0.1, v2.z - binormal2.z + 0.1);
            glTexCoord2f(1.0, 0.0);
            glVertex3f(v1.x - binormal2.x + 0.1, v1.y+1 - binormal2.y + 0.1, v1.z - binormal2.z + 0.1);
            glEnd();
            
            //RIGHT RAIL
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0);
            glVertex3f(v1.x + binormal2.x, v1.y+1 + binormal2.y, v1.z + binormal2.z);
            glTexCoord2f(0.0, 1.0);
            glVertex3f(v2.x + binormal2.x, v2.y+1 + binormal2.y, v2.z + binormal2.z);
            glTexCoord2f(1.0, 1.0);
            glVertex3f(v2.x + binormal2.x + 0.1, v2.y+1 + binormal2.y + 0.1, v2.z + binormal2.z + 0.1);
            glTexCoord2f(1.0, 0.0);
            glVertex3f(v1.x + binormal2.x + 0.1, v1.y+1 + binormal2.y + 0.1, v1.z + binormal2.z + 0.1);
            glEnd();
            
            //CROSS BAR
            if (draw_cross < 3){
                draw_cross++;
            }
            else {
                glBegin(GL_QUADS);
                glTexCoord2f(0.0, 0.0);
                glVertex3f(v1.x - binormal2.x, v1.y+1 - binormal2.y, v1.z - binormal2.z);
                glTexCoord2f(1.0, 0.0);
                glVertex3f(v1.x + binormal2.x, v1.y+1 + binormal2.y, v1.z + binormal2.z);
                glTexCoord2f(1.0, 1.0);
                glVertex3f(v2.x + binormal2.x, v2.y+1 + binormal2.y, v2.z + binormal2.z);
                glTexCoord2f(0.0, 1.0);
                glVertex3f(v2.x - binormal2.x, v2.y+1 - binormal2.y, v2.z - binormal2.z);
                glEnd();
                draw_cross = 0;
            }
            
            //SUPPORTS
            if (draw_supports < 20)
                draw_supports++;
            else {
                //LEFT SUPPORT
                glLineWidth(5);
                glBegin(GL_LINE_STRIP);
                glVertex3f(v1.x - binormal2.x, v1.y+1 - binormal2.y, v1.z - binormal2.z);
                glVertex3f(v1.x - binormal2.x, 0, v1.z - binormal2.z);
                glEnd();
                
                //RIGHT SUPPORT
                glBegin(GL_LINE_STRIP);
                glVertex3f(v2.x + binormal2.x, v2.y+1 + binormal2.y, v2.z + binormal2.z);
                glVertex3f(v2.x + binormal2.x, 0, v2.z + binormal2.z);
                glEnd();
               
                draw_supports = 0;
            }
        }
    }
    glDisable(GL_TEXTURE_2D);
    
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
    /* reads through the spline finexles */
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        fileSpline = fopen(argv, "r");
       
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

//Screenshot variables
int fileNum = 0;
string screenshotName;

/* Write a screenshot to the specified filename */
void saveScreenshot (string file)
{
    string str_file = file;
    char screenshotName [str_file.size()+1];
    screenshotName[str_file.size()]=0;
    memcpy(screenshotName,str_file.c_str(),str_file.size());
    char num[10];
    
    sprintf(num, "%d", fileNum);
    if (fileNum < 10)
        strcat(screenshotName, "00");
    else if (fileNum < 100)
        strcat(screenshotName, "0");
    strcat(screenshotName, num);
    strcat(screenshotName, ".jpg");
    fileNum++;    //increase screenshot count
    
    Pic *in = NULL;
    
    Pic *out = NULL;
    
    if (screenshotName == NULL)
        return;
    
    in = pic_alloc(640, 480, 3, NULL);
    
    out = pic_alloc(640, 480, 3, NULL);
    
    printf("File to save to: %s\n", screenshotName);
    
    glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_BYTE, &in->pix[0]);
    
    for ( int j=0; j<480; j++ ) {
        
        for ( int i=0; i<640; i++ ) {
            
            PIC_PIXEL(out, i, j, 0) = PIC_PIXEL(in, i, 480-1-j, 0);
            
            PIC_PIXEL(out, i, j, 1) = PIC_PIXEL(in, i, 480-1-j, 1);
            
            PIC_PIXEL(out, i, j, 2) = PIC_PIXEL(in, i, 480-1-j, 2);
            
        }
        
    }
    
    if (jpeg_write(screenshotName, out))
        
        printf("File saved Successfully\n");
    
    else
        
        printf("Error in Saving\n");
    
    pic_free(in);
    
    pic_free(out);
    
}

/* takes keyboard input for extra functionality
 */

int counter1 = 0;
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case '1':
            playCoaster = true;
            break;
        case '2':
            playCoaster = false;
            break;
        case '3':
            cout << " x = " << x << endl << endl;
            cout.flush();
            counter1++;
            saveScreenshot(screenshotName);   //saves screenshot
            if (counter1 >= 5)
                playCoaster = true;
            break;
        default:
            break;
    }
}

int main (int argc, char ** argv)
{
    screenshotName = "/Users/joshgreenberger/Documents/Graphics/assign2/Xcode/screenshots/";
    
    if (argc<2)
    {
        printf ("usage: %s heightfield.jpg\n", argv[0]);
        exit(1);
    }
    
    loadSplines(argv[1]);
    glutInit(&argc,argv);
    
    //Window Initialization
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH|GLUT_RGB);
    glutInitWindowSize(640,480);
    glutInitWindowPosition(100,100);
    glutCreateWindow(argv[0]);
    
    /* do initialization */
    
    //Texture Initialization
    glGenTextures(5, texture);
    texload(0,argv[2]);
    texload(1,argv[3]);
    texload(2,argv[4]);
    
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
