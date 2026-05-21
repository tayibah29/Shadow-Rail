#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <windows.h>
#include <stdlib.h>
#include <math.h>


float trainX = -70.0f;     // Starting further back to accommodate longer train
float wheelRotate = 0.0f;
bool isMoving = true;

// Camera configuration (Orbit angles and distance)
float camAngleX = 12.0f;
float camAngleY = 40.0f;   // Inverted angle to view the train moving forward smoothly
float camRadius = 28.0f;   // Zoomed out slightly to fit more compartments in view

// Particle structure for more realistic smoke puffs
struct SmokeParticle {
    float xOffset;
    float yOffset;
    float size;
    float alpha;
};
const int MAX_SMOKE = 12;
SmokeParticle smokeTrail[MAX_SMOKE];


void resize(int width, int height) {
    if(height == 0) height = 1;
    float ratio = (float)width / (float)height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55, ratio, 1, 500);
    glMatrixMode(GL_MODELVIEW);
}


void drawCube(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawCylinder(float baseRad, float topRad, float len, int slices) {
    GLUquadric *q = gluNewQuadric();
    gluCylinder(q, baseRad, topRad, len, slices, 20);
    gluDeleteQuadric(q);
}

void drawWheel(float radius, float width) {
    glPushMatrix();
    glRotatef(wheelRotate, 0.0f, 0.0f, 1.0f);
    glColor3f(0.12f, 0.12f, 0.12f);
    glutSolidTorus(width / 2.0f, radius - (width / 2.0f), 20, 20);

    // Spokes
    glColor3f(0.25f, 0.15f, 0.1f);
    glBegin(GL_LINES);
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(cos(angle) * radius, sin(angle) * radius, 0.0f);
    }
    glEnd();
    glPopMatrix();
}


void steamEngine() {
    // 1. Heavy Base Chassis
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, -0.4f, 0.0f);
    drawCube(5.0f, 0.2f, 1.5f);
    glPopMatrix();

    // 2. Main Boiler Cylinder (Facing rightwards)
    glColor3f(0.45f, 0.28f, 0.2f);
    glPushMatrix();
    glTranslatef(0.5f, 0.3f, 0.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f); // Flipped cylinder setup orientation
    glTranslatef(0.0f, 0.0f, -1.4f);
    drawCylinder(0.65f, 0.65f, 3.2f, 30);
    glPopMatrix();

    // Front Boiler Cap
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(2.3f, 0.3f, 0.0f);
    glScalef(0.1f, 1.3f, 1.3f);
    glutSolidSphere(0.5f, 20, 20);
    glPopMatrix();

    // Cowcatcher Front grill
    glColor3f(0.35f, 0.22f, 0.18f);
    glPushMatrix();
    glTranslatef(2.6f, -0.45f, 0.0f);
    glRotatef(-45.0f, 0.0f, 0.0f, 1.0f);
    drawCube(0.4f, 0.4f, 1.4f);
    glPopMatrix();

    // 3. Steam Domes
    glColor3f(0.4f, 0.25f, 0.18f);
    float domePositions[] = {1.4f, 0.7f, 0.0f};
    for(int i = 0; i < 3; i++) {
        glPushMatrix();
        glTranslatef(domePositions[i], 0.95f, 0.0f);
        glScalef(0.3f, 0.4f, 0.3f);
        glutSolidSphere(0.6f, 15, 15);
        glPopMatrix();
    }

    // 4. Smoke Stack (Chimney placed at front end)
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(1.9f, 0.8f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(0.18f, 0.25f, 0.6f, 20);
    glPopMatrix();

    // 5. Driver's Cabin (Moved to back left section)
    glColor3f(0.35f, 0.22f, 0.18f);
    glPushMatrix();
    glTranslatef(-1.8f, 0.6f, 0.0f);
    drawCube(1.4f, 1.2f, 1.4f);
    glPopMatrix();

    // Cabin Roof
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(-1.75f, 1.25f, 0.0f);
    drawCube(1.6f, 0.1f, 1.55f);
    glPopMatrix();

    // Cabin Windows
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(-1.8f, 0.7f, 0.71f); drawCube(0.4f, 0.4f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.8f, 0.7f, -0.71f); drawCube(0.4f, 0.4f, 0.02f); glPopMatrix();

    // 6. Huge Locomotive Driving Wheels
    float wheelPositionsX[] = {1.2f, 0.0f, -1.2f};
    for(int i = 0; i < 3; i++) {
        glPushMatrix(); glTranslatef(wheelPositionsX[i], -0.6f, 0.75f); drawWheel(0.6f, 0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef(wheelPositionsX[i], -0.6f, -0.75f); drawWheel(0.6f, 0.12f); glPopMatrix();
    }

    // Front guide wheel
    glPushMatrix(); glTranslatef(2.2f, -0.8f, 0.65f); drawWheel(0.35f, 0.1f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.2f, -0.8f, -0.65f); drawWheel(0.35f, 0.1f); glPopMatrix();

    // 7. Piston rod link
    glColor3f(0.4f, 0.4f, 0.4f);
    glPushMatrix(); glTranslatef(0.0f, -0.6f, 0.82f); drawCube(2.6f, 0.06f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.6f, -0.82f); drawCube(2.6f, 0.06f, 0.04f); glPopMatrix();
}


void coalTender() {
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix(); glTranslatef(0.0f, -0.4f, 0.0f); drawCube(3.2f, 0.2f, 1.4f); glPopMatrix();

    glColor3f(0.35f, 0.22f, 0.18f);
    glPushMatrix(); glTranslatef(0.0f, 0.2f, 0.0f); drawCube(3.0f, 0.8f, 1.3f); glPopMatrix();

    glColor3f(0.08f, 0.08f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, 0.65f, 0.0f); glScalef(2.6f, 0.2f, 1.1f); glutSolidSphere(0.5f, 10, 10); glPopMatrix();

    for (float wX = -1.1f; wX <= 1.2f; wX += 0.7f) {
        glPushMatrix(); glTranslatef(wX, -0.75f, 0.65f); drawWheel(0.4f, 0.1f); glPopMatrix();
        glPushMatrix(); glTranslatef(wX, -0.75f, -0.65f); drawWheel(0.4f, 0.1f); glPopMatrix();
    }
}


void passengerCompartment() {
    // Main Wooden Brown Coach Body
    glColor3f(0.42f, 0.26f, 0.18f);
    glPushMatrix();
    glTranslatef(0.0f, 0.35f, 0.0f);
    drawCube(4.4f, 1.2f, 1.3f);
    glPopMatrix();

    // Roof Top
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    drawCube(4.6f, 0.12f, 1.4f);
    glPopMatrix();

    // Row of passenger side windows
    glColor3f(0.15f, 0.15f, 0.15f);
    for(float winX = -1.6f; winX <= 1.7f; winX += 0.8f) {
        glPushMatrix(); glTranslatef(winX, 0.5f, 0.66f); drawCube(0.4f, 0.4f, 0.02f); glPopMatrix();
        glPushMatrix(); glTranslatef(winX, 0.5f, -0.66f); drawCube(0.4f, 0.4f, 0.02f); glPopMatrix();
    }

    // Compartment Wheels
    glPushMatrix(); glTranslatef(-1.5f, -0.65f, 0.6f); drawWheel(0.45f, 0.1f); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, -0.65f, 0.6f);  drawWheel(0.45f, 0.1f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.5f, -0.65f, -0.6f); drawWheel(0.45f, 0.1f); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, -0.65f, -0.6f);  drawWheel(0.45f, 0.1f); glPopMatrix();
}


void drawSteamSmoke() {
    // Setup basic transparency blending for realistic clouds
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING); // Disable lighting to make smoke feel volumetric

    for(int i = 0; i < MAX_SMOKE; i++) {
        glColor4f(0.4f, 0.4f, 0.4f, smokeTrail[i].alpha);
        glPushMatrix();
        // Spawns over the chimney and shifts backward relatively as train goes right
        glTranslatef(trainX + 1.9f + smokeTrail[i].xOffset,
                     1.4f + smokeTrail[i].yOffset,
                     0.0f);
        glutSolidSphere(smokeTrail[i].size, 15, 15);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}


void environment() {
    // Ground
    glColor3f(0.92f, 0.92f, 0.92f);
    glPushMatrix(); glTranslatef(0.0f, -1.35f, 0.0f); drawCube(400.0f, 0.1f, 80.0f); glPopMatrix();

    // Rails
    glColor3f(0.4f, 0.4f, 0.4f);
    glPushMatrix(); glTranslatef(0.0f, -1.22f, 0.65f); drawCube(400.0f, 0.08f, 0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -1.22f, -0.65f); drawCube(400.0f, 0.08f, 0.08f); glPopMatrix();

    // Track Ties
    glColor3f(0.3f, 0.18f, 0.1f);
    for(float i = -185; i <= 185; i += 3.0f) {
        glPushMatrix(); glTranslatef(i, -1.28f, 0.0f); drawCube(0.8f, 0.06f, 2.0f); glPopMatrix();
    }
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Spherical Orbit Camera Setup
    float radX = camAngleX * 3.14159f / 180.0f;
    float radY = camAngleY * 3.14159f / 180.0f;
    float posX = camRadius * cos(radX) * sin(radY);
    float posY = camRadius * sin(radX);
    float posZ = camRadius * cos(radX) * cos(radY);

    gluLookAt(posX, posY, posZ,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    environment();

    // Draw active smokestack billows
    drawSteamSmoke();

    // Assembly Translation System
    glPushMatrix();
    glTranslatef(trainX, -0.3f, 0.0f);

    // 1. Steam Locomotive Lead Head Engine
    steamEngine();

    // 2. Attached Coal Tender right behind engine
    glPushMatrix();
    glTranslatef(-4.3f, 0.0f, 0.0f);
    coalTender();
    glPopMatrix();

    // 3. Multi-compartment cascade setup (Adding 3 passenger coaches)
    int additionalCoaches = 3;
    for(int i = 0; i < additionalCoaches; i++) {
        glPushMatrix();
        // Calculate offset chain behind the coal tender car
        glTranslatef(-8.3f - (i * 4.7f), -0.1f, 0.0f);
        passengerCompartment();
        glPopMatrix();
    }

    glPopMatrix();

    glutSwapBuffers();
}


// ANIMATION LOGIC & SMOKE SIM

void idle() {
    if (isMoving) {
        trainX += 0.08f;        // Train progresses FORWARD (Left to Right)
        wheelRotate -= 4.0f;    // Correct rotational sync direction

        // Custom physics updates for modern fluid-looking smoke tracking arrays
        for(int i = 0; i < MAX_SMOKE; i++) {
            smokeTrail[i].xOffset -= 0.06f; // Blow back dynamically
            smokeTrail[i].yOffset += 0.03f; // Float upwards
            smokeTrail[i].size    += 0.01f; // Expand naturally
            smokeTrail[i].alpha   -= 0.015f;// Fade out slowly

            // Reset particle if completely faded away
            if(smokeTrail[i].alpha <= 0.0f) {
                smokeTrail[i].xOffset = 0.0f;
                smokeTrail[i].yOffset = 0.0f;
                smokeTrail[i].size    = 0.2f + (rand() % 10 / 100.0f);
                smokeTrail[i].alpha   = 0.8f;
            }
        }

        if (trainX > 160.0f) {
            trainX = -160.0f;   // Wrap setup tracking loop borders
        }
    }
    glutPostRedisplay();
}


void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break;
        case ' ': isMoving = !isMoving; break; // Pause/Play
        case 'w': case 'W': camRadius -= 1.0f; break; // Zoom In
        case 's': case 'S': camRadius += 1.0f; break; // Zoom Out
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:    camAngleX += 4.0f; break;
        case GLUT_KEY_DOWN:  camAngleX -= 4.0f; break;
        case GLUT_KEY_LEFT:  camAngleY -= 4.0f; break;
        case GLUT_KEY_RIGHT: camAngleY += 4.0f; break;
    }
    if (camAngleX > 85.0f)  camAngleX = 85.0f;
    if (camAngleX < -5.0f)  camAngleX = -5.0f;

    glutPostRedisplay();
}


void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightPos[] = {20.0f, 40.0f, 30.0f, 1.0f};
    GLfloat lightAmbient[] = {0.45f, 0.45f, 0.45f, 1.0f};
    GLfloat lightDiffuse[] = {0.85f, 0.85f, 0.85f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);

    // Populate baseline values inside smoke arrays initial structures
    for(int i = 0; i < MAX_SMOKE; i++) {
        smokeTrail[i].xOffset = -i * 0.8f;
        smokeTrail[i].yOffset = i * 0.4f;
        smokeTrail[i].size    = 0.2f + (i * 0.1f);
        smokeTrail[i].alpha   = 0.8f - (i * 0.07f);
    }
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 750);
    glutCreateWindow("Realistic 3D Steam Locomotive with Coaches");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}


