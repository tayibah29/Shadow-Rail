#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>

// Train movement
float trainX = -40;
float wheelRotate = 0;

// Camera
float camX = 5;
float camY = 5;
float camZ = 15;

// ----------------------------------------------------
// Resize
// ----------------------------------------------------
void resize(int width, int height)
{
    if(height == 0)
        height = 1;

    float ratio = (float)width / (float)height;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(60, ratio, 1, 200);

    glMatrixMode(GL_MODELVIEW);
}

// ----------------------------------------------------
// Wheels
// ----------------------------------------------------
void wheel(float x, float y, float z)
{
    glPushMatrix();

    glTranslatef(x, y, z);

    glRotatef(wheelRotate, 0, 0, 1);

    glColor3f(0, 0, 0);

    glutSolidTorus(0.1, 0.3, 20, 20);

    glPopMatrix();
}

// ----------------------------------------------------
// Engine
// ----------------------------------------------------
void engine()
{
    // Main body
    glPushMatrix();

    glColor3f(0.8, 0, 0);

    glScalef(3, 1, 1);

    glutSolidCube(1);

    glPopMatrix();

    // Cabin
    glPushMatrix();

    glTranslatef(0.7, 0.8, 0);

    glColor3f(0, 0, 1);

    glScalef(1.5, 1, 1);

    glutSolidCube(1);

    glPopMatrix();

    // Chimney
    glPushMatrix();

    glTranslatef(-1, 1, 0);

    glRotatef(-90, 1, 0, 0);

    glColor3f(0.2, 0.2, 0.2);

    GLUquadric *q = gluNewQuadric();

    gluCylinder(q, 0.2, 0.2, 1, 20, 20);

    glPopMatrix();

    // Wheels
    wheel(-1, -0.7, 0.6);
    wheel(1, -0.7, 0.6);
    wheel(-1, -0.7, -0.6);
    wheel(1, -0.7, -0.6);
}

// ----------------------------------------------------
// Compartment
// ----------------------------------------------------
void compartment()
{
    // Body
    glPushMatrix();

    glColor3f(0.1, 0.7, 0.2);

    glScalef(3, 1, 1);

    glutSolidCube(1);

    glPopMatrix();

    // Roof
    glPushMatrix();

    glTranslatef(0, 0.8, 0);

    glColor3f(0.3, 0.3, 0.3);

    glScalef(3.2, 0.3, 1.2);

    glutSolidCube(1);

    glPopMatrix();

    // Wheels
    wheel(-1, -0.7, 0.6);
    wheel(1, -0.7, 0.6);
    wheel(-1, -0.7, -0.6);
    wheel(1, -0.7, -0.6);
}

// ----------------------------------------------------
// Track
// ----------------------------------------------------
void track()
{
    // Rails
    glColor3f(0.3, 0.3, 0.3);

    // Left rail
    glPushMatrix();

    glTranslatef(0, -1, -0.7);

    glScalef(100, 0.1, 0.1);

    glutSolidCube(1);

    glPopMatrix();

    // Right rail
    glPushMatrix();

    glTranslatef(0, -1, 0.7);

    glScalef(100, 0.1, 0.1);

    glutSolidCube(1);

    glPopMatrix();

    // Sleepers
    for(float i = -50; i <= 50; i += 1.5)
    {
        glPushMatrix();

        glTranslatef(i, -1.1, 0);

        glColor3f(0.5, 0.3, 0.1);

        glScalef(0.4, 0.1, 2);

        glutSolidCube(1);

        glPopMatrix();
    }
}

// ----------------------------------------------------
// Trees
// ----------------------------------------------------
void tree(float x, float z)
{
    // Trunk
    glPushMatrix();

    glTranslatef(x, 0, z);

    glColor3f(0.5, 0.3, 0.1);

    glRotatef(-90, 1, 0, 0);

    GLUquadric *q = gluNewQuadric();

    gluCylinder(q, 0.2, 0.2, 2, 20, 20);

    glPopMatrix();

    // Leaves
    glPushMatrix();

    glTranslatef(x, 2.5, z);

    glColor3f(0, 0.7, 0);

    glutSolidSphere(1, 20, 20);

    glPopMatrix();
}

// ----------------------------------------------------
// Display
// ----------------------------------------------------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(camX, camY, camZ,
              0, 0, 0,
              0, 1, 0);

    // Ground
    glPushMatrix();

    glColor3f(0.2, 0.7, 0.2);

    glTranslatef(0, -2, 0);

    glScalef(150, 0.1, 40);

    glutSolidCube(1);

    glPopMatrix();

    // Track
    track();

    // Trees
    for(int i = -40; i <= 40; i += 10)
    {
        tree(i, -8);
        tree(i, 8);
    }

    // Whole train
    glPushMatrix();

    glTranslatef(trainX, 0, 0);

    // Engine
    engine();

    // Compartments
    for(int i = 1; i <= 4; i++)
    {
        glPushMatrix();

        glTranslatef(i * -4, 0, 0);

        compartment();

        glPopMatrix();
    }

    glPopMatrix();

    glutSwapBuffers();
}

// ----------------------------------------------------
// Animation
// ----------------------------------------------------
void idle()
{
    trainX += 0.03;

    wheelRotate -= 2;

    if(trainX > 60)
        trainX = -60;

    glutPostRedisplay();
}

// ----------------------------------------------------
// Keyboard
// ----------------------------------------------------
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27:
    case 'q':
        exit(0);
        break;

    // Camera zoom
    case 'w':
        camZ--;
        break;

    case 's':
        camZ++;
        break;

    // Camera left/right
    case 'a':
        camX--;
        break;

    case 'd':
        camX++;
        break;
    }

    glutPostRedisplay();
}

// ----------------------------------------------------
// Init
// ----------------------------------------------------
void init()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.5, 0.8, 1.0, 1.0);
}

// ----------------------------------------------------
// Main
// ----------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(1000, 700);

    glutCreateWindow("3D Train Project");

    init();

    glutDisplayFunc(display);

    glutReshapeFunc(resize);

    glutKeyboardFunc(keyboard);

    glutIdleFunc(idle);

    glutMainLoop();

    return 0;
}
