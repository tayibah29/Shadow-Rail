#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
using namespace std;

#define PI 3.14159265f
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


unsigned int mountainTex, brickTex ;


unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    return textureID;
}



// ---- Forward declarations ----
void drawCube(float w, float h, float d);
void drawCylinder(float baseRad, float topRad, float len, int slices);
void drawWheel(float radius, float width);
void drawPassenger(float r, float g, float b);
void drawBell();
void steamEngine();
void coalTender();
void drawInteriorFixtures();
void passengerCarriage();
void drawTree(float x, float z, float scaling);
void drawMountain(float x, float z, float height, float width);
void drawHouse(float x, float z, float r, float g, float b);
void drawWindmill(float x, float z);
void drawCloud(float x, float y, float z);
void drawStationPlatform(float x);
void environment();
void drawSteamSmoke();
void drawIndicators();
float getTrackCurve(float x);
// -----------------------------

// Train State Variables
float trainX = -5.0f;
float maxSpeed = 5.0f;
float minSpeed = -1.5f;
float environmentScrollX = 0.0f;
float speedX = 0.0f;
float friction = 0.005f;
float wheelRotate = 0.0f;
bool isMoving = false;
bool isPaused = false;

// Control Feature States
bool doorsOpen = false;
float doorAnimationTimer = 0.0f;
int cameraMode = 0;
bool showInterior = false;
bool bellRinging = false;
float bellAngle = 0.0f;
float bellTime = 0.0f;
bool headlightOn = true;
bool interiorLightOn = true;
bool leftSignalOn = false;
bool rightSignalOn = false;
bool emergencySignalOn = false;
float signalFlashTimer = 0.0f;
bool signalFlashState = false;

// ============================================================
// PASSENGER BOARDING SIMULATION
// ============================================================
enum BoardingState {
    WAITING,      // Standing on platform, waiting for doors
    WALKING,      // Walking toward train door
    BOARDING,     // Stepping into carriage (transition)
    BOARDED,      // Inside the train (seated)
    EXITING,      // Walking away from platform (alighting)
    GONE          // Off screen / recycled
};

struct PlatformPassenger {
    float x, y, z;            // World position
    float targetX, targetZ;   // Destination (door opening)
    float walkSpeed;
    float r, g, b;            // Shirt colour
    BoardingState state;
    float stateTimer;         // Time in current state
    float legSwing;           // Leg animation angle
    int   carriageIdx;        // Which carriage door (0-2)
    bool  alighting;          // true = was on train, now leaving
};

const int MAX_PLATFORM_PAX = 40;
PlatformPassenger platformPax[MAX_PLATFORM_PAX];

// Station stop trigger positions
const float STATION_POSITIONS[] = { -45.0f, 220.0f };
const int   NUM_STATIONS = 2;
bool  atStation         = false;
int   currentStation    = -1;
float boardingTimer     = 0.0f; // Counts up while doors are open
bool  boardingComplete  = false;
int   seatedPassengers  = 0;    // Tracks count internally

// ============================================================
// Dynamic Track Changing State
bool useStraightTrack = false;
float trackSwitchMorph = 0.0f;
int currentScene = 0;
bool isNight = false;

// Camera configuration
float camAngleX = 90.0f;
float camAngleY = 0.0f;
float camRadius = 45.0f;

// Windmill Rotation Angle
float windmillBladeAngle = 0.0f;

// Centralized Track Curvature Function Matrix
float getTrackCurve(float x) {
    float curvedPath = sin(x * 0.04f) * 4.5f;
    float straightPath = 0.0f;
    return (curvedPath * (1.0f - trackSwitchMorph)) + (straightPath * trackSwitchMorph);
}

// Fixed Smoke Particle System Structure
struct SmokeParticle {
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float size;
    float alpha;
};
const int MAX_SMOKE = 30;
SmokeParticle smokeTrail[MAX_SMOKE];
const int MAX_STARS = 150;
float starPositions[MAX_STARS][3];

// ---- Boarding simulation helpers --------------------------------

void spawnPlatformPassengers(float stationX) {
    float carriageOffsets[] = { 0.0f, -9.5f, -19.0f };
    float doorZ = getTrackCurve(stationX) + 1.37f + 5.0f; // platform side

    seatedPassengers = 0;
    for (int i = 0; i < MAX_PLATFORM_PAX; i++) {
        PlatformPassenger &p = platformPax[i];
        float spreadX = stationX + ((rand() % 30) - 15.0f);
        p.x = spreadX;
        p.y = -1.7f;    // platform surface
        p.z = doorZ + 2.0f + ((rand() % 6) * 0.8f);

        // Assign to a carriage door
        p.carriageIdx = i % 3;
        float cX = stationX - 17.0f + carriageOffsets[p.carriageIdx];
        p.targetX = cX;
        p.targetZ = getTrackCurve(cX) + 1.37f + 0.1f;

        p.walkSpeed = 0.04f + (rand() % 40) / 1000.0f;
        p.r = (rand() % 80 + 20) / 100.0f;
        p.g = (rand() % 80 + 20) / 100.0f;
        p.b = (rand() % 80 + 20) / 100.0f;
        p.state = WAITING;
        p.stateTimer = (rand() % 80) * 0.05f;
        p.legSwing = 0.0f;
        p.alighting = (i >= MAX_PLATFORM_PAX - 4); // last 4 are alighting
    }
}

void updateBoardingSimulation() {
    if (!atStation) return;
    boardingTimer += 0.016f;

    for (int i = 0; i < MAX_PLATFORM_PAX; i++) {
        PlatformPassenger &p = platformPax[i];
        p.stateTimer += 0.016f;

        switch (p.state) {
        case WAITING:
            if (doorsOpen && p.stateTimer > 0.5f)
                p.state = WALKING;
            if (p.alighting && doorsOpen && p.stateTimer > 0.3f)
                p.state = EXITING;
            break;

        case WALKING: {
            float dx = p.targetX - p.x;
            float dz = p.targetZ - p.z;
            float dist = sqrtf(dx*dx + dz*dz);
            if (dist > 0.15f) {
                p.x += (dx / dist) * p.walkSpeed;
                p.z += (dz / dist) * p.walkSpeed;
                p.legSwing += 0.2f;
            } else {
                p.state = BOARDING;
                p.stateTimer = 0.0f;
            }
            break;
        }

        case BOARDING:
            p.z -= 0.035f;
            if (p.stateTimer > 1.2f) {
                p.state = BOARDED;
                seatedPassengers++;
            }
            break;

        case EXITING:
            p.z += 0.06f;
            p.x += 0.01f;
            if (p.z > p.z + 8.0f || p.stateTimer > 4.0f)
                p.state = GONE;
            break;

        case BOARDED:
        case GONE:
            break;
        }
    }

    int boarded = 0, total = MAX_PLATFORM_PAX - 4;
    for (int i = 0; i < total; i++)
        if (platformPax[i].state == BOARDED || platformPax[i].state == GONE)
            boarded++;
    boardingComplete = (boarded >= total);
}

// Draw a walking/standing platform passenger
void drawPlatformPassenger(const PlatformPassenger &p) {
    if (p.state == BOARDED || p.state == GONE) return;
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);

    if (p.state == WALKING || p.state == BOARDING) {
        float dx = p.targetX - p.x;
        float dz = p.targetZ - p.z;
        float angle = atan2f(dx, dz) * 180.0f / PI;
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
    }

    // Body
    glColor3f(p.r, p.g, p.b);
    glPushMatrix(); glTranslatef(0.0f, 0.3f, 0.0f); drawCube(0.3f, 0.45f, 0.25f); glPopMatrix();

    // Head
    glColor3f(0.95f, 0.78f, 0.65f);
    glPushMatrix(); glTranslatef(0.0f, 0.7f, 0.0f); glutSolidSphere(0.12f, 8, 8); glPopMatrix();

    // Hair
    glColor3f(0.12f, 0.10f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, 0.77f, 0.02f); glutSolidSphere(0.10f, 6, 6); glPopMatrix();

    // Legs
    float swing = (p.state == WALKING || p.state == EXITING) ? sinf(p.legSwing) * 18.0f : 0.0f;
    glColor3f(0.15f, 0.22f, 0.42f);

    // Left leg
    glPushMatrix();
    glTranslatef(-0.07f, 0.0f, 0.0f);
    glRotatef(swing, 1.0f, 0.0f, 0.0f);
    drawCube(0.1f, 0.38f, 0.1f);
    glPopMatrix();

    // Right leg
    glPushMatrix();
    glTranslatef(0.07f, 0.0f, 0.0f);
    glRotatef(-swing, 1.0f, 0.0f, 0.0f);
    drawCube(0.1f, 0.38f, 0.1f);
    glPopMatrix();

    glPopMatrix();
}

void drawBoardingPassengers() {
    for (int i = 0; i < MAX_PLATFORM_PAX; i++)
        drawPlatformPassenger(platformPax[i]);
}

// Removed the top left text and overlay elements completely
void drawBoardingHUD() {
    // Left empty intentionally to completely clear out the layout display text
}

void checkStationProximity() {
    bool wasAtStation = atStation;
    atStation = false;
    if (fabs(speedX) > 0.3f) { currentStation = -1; return; }

    float scrolledX = -environmentScrollX;
    for (int s = 0; s < NUM_STATIONS; s++) {
        float stX = STATION_POSITIONS[s] + scrolledX;
        if (fabs(trainX - stX) < 12.0f) {
            atStation = true;
            if (currentStation != s) {
                currentStation = s;
                boardingTimer = 0.0f;
                boardingComplete = false;
                doorsOpen = false;
                spawnPlatformPassengers(stX);
            }
            break;
        }
    }
    if (wasAtStation && !atStation) {
        doorsOpen = false;
    }
}

void initStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        starPositions[i][0] = (rand() % 600) - 300.0f;
        starPositions[i][1] = (rand() % 150) + 30.0f;
        starPositions[i][2] = (rand() % 600) - 300.0f;
    }
}

void resize(int width, int height) {
    if (height == 0) height = 1;
    float ratio = (float)width / (float)height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55, ratio, 0.05, 1000);
    glMatrixMode(GL_MODELVIEW);
}

void drawCube(float w, float h, float d) {

    float x = w / 2.0f;
    float y = h / 2.0f;
    float z = d / 2.0f;

    glBegin(GL_QUADS);

    // Front
    glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-x,-y, z);
    glTexCoord2f(1,0); glVertex3f( x,-y, z);
    glTexCoord2f(1,1); glVertex3f( x, y, z);
    glTexCoord2f(0,1); glVertex3f(-x, y, z);

    // Back
    glNormal3f(0,0,-1);
    glTexCoord2f(0,0); glVertex3f( x,-y,-z);
    glTexCoord2f(1,0); glVertex3f(-x,-y,-z);
    glTexCoord2f(1,1); glVertex3f(-x, y,-z);
    glTexCoord2f(0,1); glVertex3f( x, y,-z);

    // Left
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0); glVertex3f(-x,-y,-z);
    glTexCoord2f(1,0); glVertex3f(-x,-y, z);
    glTexCoord2f(1,1); glVertex3f(-x, y, z);
    glTexCoord2f(0,1); glVertex3f(-x, y,-z);

    // Right
    glNormal3f(1,0,0);
    glTexCoord2f(0,0); glVertex3f( x,-y, z);
    glTexCoord2f(1,0); glVertex3f( x,-y,-z);
    glTexCoord2f(1,1); glVertex3f( x, y,-z);
    glTexCoord2f(0,1); glVertex3f( x, y, z);

    // Top
    glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-x, y, z);
    glTexCoord2f(1,0); glVertex3f( x, y, z);
    glTexCoord2f(1,1); glVertex3f( x, y,-z);
    glTexCoord2f(0,1); glVertex3f(-x, y,-z);

    // Bottom
    glNormal3f(0,-1,0);
    glTexCoord2f(0,0); glVertex3f(-x,-y,-z);
    glTexCoord2f(1,0); glVertex3f( x,-y,-z);
    glTexCoord2f(1,1); glVertex3f( x,-y, z);
    glTexCoord2f(0,1); glVertex3f(-x,-y, z);

    glEnd();
}

void drawCylinder(float baseRad, float topRad, float len, int slices) {
    GLUquadric *q = gluNewQuadric();
    gluCylinder(q, baseRad, topRad, len, slices, 20);
    gluDeleteQuadric(q);
}

void drawWheel(float radius, float width) {
    glPushMatrix();
    glRotatef(wheelRotate, 0.0f, 0.0f, 1.0f);
    glColor3f(0.08f, 0.08f, 0.08f);
    glutSolidTorus(width / 2.0f, radius - (width / 2.0f), 15, 20);

    glColor3f(0.4f, 0.35f, 0.25f);
    for (int i = 0; i < 12; i++) {
        float angle = i * PI / 6.0f;
        glPushMatrix();
        glRotatef(angle * 180.0f / PI, 0.0f, 0.0f, 1.0f);
        glTranslatef(radius / 2.0f, 0.0f, 0.0f);
        drawCube(radius, 0.06f, 0.06f);
        glPopMatrix();
    }
    glPopMatrix();
}


void drawPassenger(float r, float g, float b){
    glPushMatrix();

    // ---------------- NECK ----------------
    // Metallic Silver-Grey Neck Joint
    glColor3f(0.70f, 0.70f, 0.70f);
    glPushMatrix();
    glTranslatef(0.0f, 0.36f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(0.05f, 0.05f, 0.08f, 12);
    glPopMatrix();


    // ---------------- HEAD & FACE ----------------
    // Warm Skin Tone Head
    glColor3f(0.98f, 0.82f, 0.69f);
    glPushMatrix();
    glTranslatef(0.0f, 0.52f, 0.0f);
    glutSolidSphere(0.13f, 16, 16);
    glPopMatrix();

    // Deep Chestnut Brown Hair
    glColor3f(0.30f, 0.18f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 0.61f, 0.02f);
    glutSolidSphere(0.09f, 12, 12);
    glPopMatrix();

    // Glossy Black Eyes
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(-0.04f, 0.54f, 0.11f);
    glutSolidSphere(0.02f, 8, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.04f, 0.54f, 0.11f);
    glutSolidSphere(0.02f, 8, 8);
    glPopMatrix();

    // Slightly Rosy Nose Tone
    glColor3f(0.92f, 0.72f, 0.60f);
    glPushMatrix();
    glTranslatef(0.0f, 0.51f, 0.125f);
    drawCube(0.02f, 0.03f, 0.03f);
    glPopMatrix();


    // ---------------- BODY / TORSO ----------------
    // Dark Navy Blue Shirt/Jacket
    glColor3f(0.12f, 0.20f, 0.35f);
    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    drawCube(0.32f, 0.42f, 0.28f);
    glPopMatrix();

    // Matte Black Waist / Belt Line
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, -0.10f, 0.0f);
    drawCube(0.26f, 0.10f, 0.24f);
    glPopMatrix();


    // ---------------- ARMS ----------------
    // Light Heather Grey Sleeves
    glColor3f(0.75f, 0.75f, 0.78f);

    // Left Arm
    glPushMatrix();
    glTranslatef(-0.21f, 0.12f, 0.02f);
    glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    drawCube(0.08f, 0.42f, 0.08f);
    glPopMatrix();

    // Right Arm
    glPushMatrix();
    glTranslatef(0.21f, 0.12f, 0.02f);
    glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
    drawCube(0.08f, 0.42f, 0.08f);
    glPopMatrix();

    // Matching Skin-Tone Hands
    glColor3f(0.98f, 0.82f, 0.69f);
    glPushMatrix();
    glTranslatef(-0.21f, -0.12f, 0.06f);
    drawCube(0.07f, 0.07f, 0.09f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.21f, -0.12f, 0.06f);
    drawCube(0.07f, 0.07f, 0.09f);
    glPopMatrix();


    // ---------------- LEGS ----------------
    // Upper Legs: Dark Denim/Slate Blue Trousers
    glColor3f(0.20f, 0.28f, 0.40f);

    // Left Upper Leg
    glPushMatrix();
    glTranslatef(-0.09f, -0.12f, 0.15f);
    drawCube(0.09f, 0.09f, 0.32f);
    glPopMatrix();

    // Right Upper Leg
    glPushMatrix();
    glTranslatef(0.09f, -0.12f, 0.15f);
    drawCube(0.09f, 0.09f, 0.32f);
    glPopMatrix();

    // Lower Legs: Charcoal Grey Shins/Socks
    glColor3f(0.30f, 0.30f, 0.32f);

    // Left Lower Leg
    glPushMatrix();
    glTranslatef(-0.09f, -0.34f, 0.28f);
    drawCube(0.09f, 0.42f, 0.09f);
    glPopMatrix();

    // Right Lower Leg
    glPushMatrix();
    glTranslatef(0.09f, -0.34f, 0.28f);
    drawCube(0.09f, 0.42f, 0.09f);
    glPopMatrix();


    // ---------------- FEET ----------------
    // Classic Dark Brown / Leather Shoes
    glColor3f(0.25f, 0.15f, 0.10f);

    // Left Foot
    glPushMatrix();
    glTranslatef(-0.09f, -0.56f, 0.32f);
    drawCube(0.11f, 0.05f, 0.16f);
    glPopMatrix();

    // Right Foot
    glPushMatrix();
    glTranslatef(0.09f, -0.56f, 0.32f);
    drawCube(0.11f, 0.05f, 0.16f);
    glPopMatrix();

    glPopMatrix();
}
void drawIndicators() {
    if (!signalFlashState) return;
    GLfloat emissiveYellow[] = { 1.0f, 0.6f, 0.0f, 1.0f };
    GLfloat noEmissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float tCurve = getTrackCurve(trainX);

    if (leftSignalOn || emergencySignalOn) {
        glPushMatrix();
        glTranslatef(trainX + 5.2f, -1.9f, tCurve - 1.45f);
        glMaterialfv(GL_FRONT, GL_EMISSION, emissiveYellow);
        glColor3f(1.0f, 0.6f, 0.0f);
        glutSolidSphere(0.2f, 12, 12);
        glPopMatrix();
    }

    if (rightSignalOn || emergencySignalOn) {
        glPushMatrix();
        glTranslatef(trainX + 5.2f, -1.9f, tCurve + 1.45f);
        glMaterialfv(GL_FRONT, GL_EMISSION, emissiveYellow);
        glColor3f(1.0f, 0.6f, 0.0f);
        glutSolidSphere(0.2f, 12, 12);
        glPopMatrix();
    }
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmissive);
}

void drawBell() {
    glPushMatrix();
    glTranslatef(1.2f, 2.3f, 0.0f);
    glColor3f(0.15f, 0.15f, 0.15f);
    drawCube(0.15f, 0.6f, 0.6f);

    glPushMatrix();
    glRotatef(bellAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(0.0f, -0.3f, 0.0f);
    glColor3f(0.9f, 0.75f, 0.2f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(0.25f, 0.1f, 0.4f, 12);
    glPopMatrix();
    glPopMatrix();
}

void steamEngine() {
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f, -0.4f, 0.0f);
    drawCube(10.2f, 0.4f, 3.1f); glPopMatrix();

    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(5.3f, -0.4f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0f, 0.4f, 1.5f);     glVertex3f(0.8f, -0.2f, 0.0f);    glVertex3f(0.0f, -0.2f, 1.5f);
    glVertex3f(0.0f, 0.4f, -1.5f);    glVertex3f(0.0f, -0.2f, -1.5f);   glVertex3f(0.8f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.4f, 1.5f);     glVertex3f(0.0f, 0.4f, -1.5f);    glVertex3f(0.8f, -0.2f, 0.0f);
    glEnd();
    glPopMatrix();

    glColor3f(0.22f, 0.32f, 0.42f);
    glPushMatrix();
    glTranslatef(1.2f, 1.0f, 0.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -3.8f);
    drawCylinder(1.25f, 1.25f, 7.6f, 32);
    glPopMatrix();

    glColor3f(0.8f, 0.65f, 0.2f);
    float bandsX[] = { -1.8f, 0.0f, 1.8f, 3.6f };
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(bandsX[i], 1.0f, 0.0f);
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        drawCylinder(1.27f, 1.27f, 0.15f, 32);
        glPopMatrix();
    }

    glColor3f(0.12f, 0.12f, 0.12f);
    glPushMatrix();
    glTranslatef(5.0f, 1.0f, 0.0f);
    glScalef(0.2f, 2.5f, 2.5f); glutSolidSphere(0.5f, 20, 20); glPopMatrix();

    if (headlightOn) {
        glEnable(GL_LIGHT1);
        GLfloat lightPos[] = { 6.0f, 0.6f, 0.0f, 1.0f };
        GLfloat lightDir[] = { 1.0f, -0.05f, 0.0f };
        GLfloat lightColor[] = { 1.0f, 0.98f, 0.85f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, lightDir);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor);
        glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 35.0f);

        glPushMatrix();
        glTranslatef(5.1f, 1.4f, 0.0f);
        GLfloat emissive[] = { 1.0f, 1.0f, 0.9f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emissive);
        glColor3f(1.0f, 1.0f, 0.9f);
        glutSolidSphere(0.35f, 16, 16);
        GLfloat noEmissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmissive);
        glPopMatrix();
    } else {
        glDisable(GL_LIGHT1);
    }

    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(4.0f, 1.9f, 0.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); drawCylinder(0.35f, 0.5f, 1.4f, 20); glPopMatrix();
    drawBell();

    glColor3f(0.22f, 0.32f, 0.42f);
    glPushMatrix(); glTranslatef(-3.5f, 1.3f, 0.0f);
    drawCube(3.4f, 2.4f, 3.0f); glPopMatrix();

    glColor3f(0.12f, 0.12f, 0.12f);
    glPushMatrix(); glTranslatef(-3.5f, 2.55f, 0.0f); drawCube(3.8f, 0.15f, 3.2f); glPopMatrix();

    glColor3f(0.1f, 0.13f, 0.16f);
    glPushMatrix();
    glTranslatef(-3.5f, 1.6f, 1.51f); drawCube(1.4f, 0.8f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(-3.5f, 1.6f, -1.51f); drawCube(1.4f, 0.8f, 0.02f); glPopMatrix();

    glColor3f(0.15f, 0.15f, 0.15f);
    float wheelPositionsX[] = {2.6f, 0.0f, -2.6f};
    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        glTranslatef(wheelPositionsX[i], -0.65f, 1.56f); drawWheel(0.65f, 0.28f); glPopMatrix();
        glPushMatrix(); glTranslatef(wheelPositionsX[i], -0.65f, -1.56f); drawWheel(0.65f, 0.28f); glPopMatrix();
    }
}

void coalTender() {
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f, -0.4f, 0.0f);
    drawCube(7.0f, 0.3f, 3.0f); glPopMatrix();

    glColor3f(0.18f, 0.24f, 0.32f);
    glPushMatrix();
    glTranslatef(0.0f, 0.6f, 0.0f); drawCube(6.6f, 1.6f, 2.8f); glPopMatrix();

    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, 1.45f, 0.0f);
    glScalef(5.8f, 0.5f, 2.4f);
    glutSolidSphere(0.5f, 16, 12);
    glPopMatrix();

    for (float wX = -2.2f; wX <= 2.3f; wX += 1.4f) {
        glPushMatrix();
        glTranslatef(wX, -0.65f, 1.35f); drawWheel(0.55f, 0.22f); glPopMatrix();
        glPushMatrix(); glTranslatef(wX, -0.65f, -1.35f); drawWheel(0.55f, 0.22f); glPopMatrix();
    }
}

void drawInteriorFixtures() {
    glColor3f(0.28f, 0.28f, 0.3f);
    glPushMatrix();
    glTranslatef(0.0f, -0.6f, -1.15f); drawCube(8.8f, 0.3f, 0.4f); glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, -0.3f, -1.38f); drawCube(8.8f, 0.5f, 0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.6f, 1.15f);
    drawCube(8.8f, 0.3f, 0.4f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.3f, 1.38f);
    drawCube(8.8f, 0.5f, 0.05f); glPopMatrix();

    float seatPositionsX[] = { -3.6f, -2.0f, -0.4f, 1.2f, 2.8f, 4.0f };
    float colorsR[] = { 0.85f, 0.15f, 0.20f, 0.70f, 0.10f, 0.90f };
    float colorsG[] = { 0.20f, 0.60f, 0.75f, 0.10f, 0.80f, 0.50f };
    float colorsB[] = { 0.30f, 0.20f, 0.15f, 0.80f, 0.30f, 0.10f };
    for(int p = 0; p < 6; p++) {
        glPushMatrix();
        glTranslatef(seatPositionsX[p], -0.25f, -1.1f);
        glRotatef(0.0f, 0.0f, 1.0f, 0.0f);
        drawPassenger(colorsR[p], colorsG[p], colorsB[p]);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(seatPositionsX[p] + 0.4f, -0.25f, 1.1f);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        drawPassenger(colorsB[p], colorsR[p], colorsG[p]);
        glPopMatrix();
    }

    glColor3f(0.7f, 0.7f, 0.72f);
    float poleX[] = { -3.2f, -1.0f, 1.0f, 3.2f };
    for(int i = 0; i < 4; i++) {
        glPushMatrix(); glTranslatef(poleX[i], -0.9f, -1.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); drawCylinder(0.025f, 0.025f, 2.3f, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(poleX[i], -0.9f, 1.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(0.025f, 0.025f, 2.3f, 8); glPopMatrix();
    }

    glPushMatrix(); glTranslatef(-4.4f, 1.15f, -0.9f); drawCylinder(0.022f, 0.022f, 8.8f, 8); glPopMatrix();
    glPushMatrix();
    glTranslatef(-4.4f, 1.15f, 0.9f); drawCylinder(0.022f, 0.022f, 8.8f, 8); glPopMatrix();

    glColor3f(0.15f, 0.15f, 0.15f);
    for (float strapX = -3.8f; strapX <= 3.9f; strapX += 0.9f) {
        glPushMatrix();
        glTranslatef(strapX, 1.05f, -0.9f); drawCube(0.02f, 0.12f, 0.02f); glTranslatef(0.0f, -0.1f, 0.0f); glutSolidTorus(0.012f, 0.06f, 8, 12); glPopMatrix();
        glPushMatrix(); glTranslatef(strapX, 1.05f, 0.9f);
        drawCube(0.02f, 0.12f, 0.02f); glTranslatef(0.0f, -0.1f, 0.0f); glutSolidTorus(0.012f, 0.06f, 8, 12); glPopMatrix();
    }
}

void passengerCarriage() {
    glColor3f(0.5f, 0.5f, 0.52f);
    glPushMatrix(); glTranslatef(0.0f, -0.85f, 0.0f); drawCube(9.2f, 0.1f, 2.7f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.45f, 0.0f); drawCube(9.2f, 0.12f, 2.72f); glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.45f, -1.36f); drawCube(9.2f, 0.7f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, -0.45f, 1.36f);  drawCube(9.2f, 0.7f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 1.2f, -1.36f); drawCube(9.2f, 0.4f, 0.04f); glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 1.36f);  drawCube(9.2f, 0.4f, 0.04f); glPopMatrix();

    glPushMatrix(); glTranslatef(-4.55f, 0.3f, 0.0f);
    drawCube(0.12f, 2.3f, 2.7f); glPopMatrix();
    glPushMatrix(); glTranslatef(4.55f, 0.3f, 0.0f); drawCube(0.12f, 2.3f, 2.7f); glPopMatrix();

    float pillarX[] = { -4.1f, -2.5f, -0.9f, 0.9f, 2.5f, 4.1f };
    for(int i = 0; i < 6; i++) {
        glPushMatrix(); glTranslatef(pillarX[i], 0.375f, -1.36f);
        drawCube(0.15f, 1.25f, 0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(pillarX[i], 0.375f, 1.36f);  drawCube(0.15f, 1.25f, 0.05f); glPopMatrix();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(0.55f, 0.8f, 0.95f, 0.3f);
    glPushMatrix(); glTranslatef(0.0f, 0.375f, -1.35f); drawCube(9.0f, 1.25f, 0.01f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.375f, 1.35f);  drawCube(9.0f, 1.25f, 0.01f); glPopMatrix();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (interiorLightOn) {
        GLfloat emissiveWarm[] = { 0.95f, 0.9f, 0.75f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emissiveWarm);
        glColor3f(1.0f, 0.95f, 0.8f);
        glPushMatrix(); glTranslatef(0.0f, 1.38f, -0.5f); drawCube(8.0f, 0.04f, 0.15f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.0f, 1.38f, 0.5f);
        drawCube(8.0f, 0.04f, 0.15f); glPopMatrix();
    } else {
        glColor3f(0.15f, 0.15f, 0.15f);
        glPushMatrix();
        glTranslatef(0.0f, 1.38f, -0.5f); drawCube(8.0f, 0.04f, 0.15f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.0f, 1.38f, 0.5f); drawCube(8.0f, 0.04f, 0.15f); glPopMatrix();
    }
    GLfloat noEmissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmissive);

    drawInteriorFixtures();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.25f, 0.3f, 0.35f, 0.5f);
    float slideShift = doorAnimationTimer * 0.85f;
    glPushMatrix(); glTranslatef(-0.4f - slideShift, 0.1f, 1.37f); drawCube(0.76f, 1.7f, 0.03f); glPopMatrix();
    glPushMatrix();
    glTranslatef(0.4f + slideShift, 0.1f, 1.37f); drawCube(0.76f, 1.7f, 0.03f); glPopMatrix();
    glDisable(GL_BLEND);

    float wheelOffsets[] = { -3.0f, 3.0f };
    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(wheelOffsets[i], -1.15f, 1.15f); drawWheel(0.55f, 0.2f); glPopMatrix();
        glPushMatrix(); glTranslatef(wheelOffsets[i], -1.15f, -1.15f); drawWheel(0.55f, 0.2f); glPopMatrix();
    }
}

void drawTree(float x, float z, float scaling) {
    glPushMatrix();
    glTranslatef(x, -2.4f, z + getTrackCurve(x));
    glScalef(scaling * 2.2f, scaling * 2.5f, scaling * 2.2f);
    glColor3f(0.35f, 0.2f, 0.1f);
    glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawCylinder(0.4f, 0.25f, 2.0f, 8); glPopMatrix();
    glColor3f(0.12f, 0.42f, 0.12f);
    glPushMatrix(); glTranslatef(0.0f, 1.5f, 0.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); drawCylinder(1.5f, 0.5f, 2.0f, 12);
    glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 2.8f, 0.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); drawCylinder(1.1f, 0.0f, 1.8f, 12); glPopMatrix();
    glPopMatrix();
}

void drawMountain(float x, float z, float height, float width)
{
    glPushMatrix();
    glTranslatef(x, -2.5f, z);

    glDisable(GL_COLOR_MATERIAL);

    GLfloat matAmbient[] = { 0.35f, 0.55f, 0.25f, 1.0f };
    GLfloat matDiffuse[] = { 0.60f, 0.85f, 0.45f, 1.0f };

    if (currentScene == 3)
    {
        matAmbient[0] = 0.7f;
        matAmbient[1] = 0.75f;
        matAmbient[2] = 0.8f;

        matDiffuse[0] = 0.85f;
        matDiffuse[1] = 0.9f;
        matDiffuse[2] = 0.95f;
    }

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);

    // ===== Texture Part =====
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mountainTex);
    // Green tint
    glColor3f(0.8f, 1.0f, 0.8f);

    glBegin(GL_TRIANGLES);

    glNormal3f(0.0f, 0.447f, 0.894f);

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-width / 2.0f, 0.0f, 0.0f);

    glTexCoord2f(0.5f, 1.0f);
    glVertex3f(0.0f, height * 1.5f, -5.0f);

    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(width / 2.0f, 0.0f, 0.0f);

    glEnd();

    glDisable(GL_TEXTURE_2D);
    // ===== End Texture Part =====

    glEnable(GL_COLOR_MATERIAL);
    glPopMatrix();
}

void drawHouse(float x, float z, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, -2.5f, z);
    glScalef(4.5f, 4.5f, 4.5f);

    glColor3f(0.35f, 0.35f, 0.37f);
    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    drawCube(2.4f, 0.3f, 2.0f);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, brickTex);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(1.0f,1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    drawCube(2.2f, 1.4f, 1.8f);
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(0.5f, 1.0f, 0.91f); drawCube(0.5f, 0.5f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.11f, 1.0f, 0.0f);
    drawCube(0.02f, 0.5f, 0.5f); glPopMatrix();

    glColor3f(0.85f, 0.25f, 0.2f);
    glPushMatrix();
    glTranslatef(0.0f, 1.7f, 0.0f);
    glBegin(GL_TRIANGLES);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-1.2f, 0.0f, 0.95f);
        glVertex3f(1.2f, 0.0f, 0.95f);
        glVertex3f(0.0f, 0.8f, 0.95f);

        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-1.2f, 0.0f, -0.95f);
        glVertex3f(0.0f, 0.8f, -0.95f);
        glVertex3f(1.2f, 0.0f, -0.95f);
    glEnd();

    glBegin(GL_QUADS);
        glNormal3f(0.58f, 0.81f, 0.0f);
        glVertex3f(1.2f, 0.0f, 0.95f);
        glVertex3f(1.2f, 0.0f, -0.95f);
        glVertex3f(0.0f, 0.8f, -0.95f);
        glVertex3f(0.0f, 0.8f, 0.95f);

        glNormal3f(-0.58f, 0.81f, 0.0f);
        glVertex3f(0.0f, 0.8f, 0.95f);
        glVertex3f(0.0f, 0.8f, -0.95f);
        glVertex3f(-1.2f, 0.0f, -0.95f);
        glVertex3f(-1.2f, 0.0f, 0.95f);
    glEnd();
    glPopMatrix();

    glColor3f(0.9f, 0.9f, 0.9f);
    glPushMatrix();
    glTranslatef(0.4f, 2.1f, -0.3f);
    drawCube(0.25f, 0.7f, 0.25f);
    glPopMatrix();
    glPopMatrix();
}

void drawWindmill(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -2.5f, z);
    glScalef(1.8f, 1.8f, 1.8f);
    glColor3f(0.85f, 0.85f, 0.8f);
    glPushMatrix(); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); drawCylinder(1.2f, 0.6f, 8.0f, 12); glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    glColor3f(0.4f, 0.4f, 0.4f);
    glutSolidSphere(0.7f, 10, 10);
    glTranslatef(0.0f, 0.0f, 0.8f);
    glRotatef(windmillBladeAngle, 0.0f, 0.0f, 1.0f);
    glColor3f(0.95f, 0.95f, 0.95f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glRotatef(i * 90.0f, 0.0f, 0.0f, 1.0f);
        glTranslatef(0.0f, 2.5f, 0.0f);
        drawCube(0.35f, 4.5f, 0.05f);
        glPopMatrix();
    }
    glPopMatrix();
    glPopMatrix();
}

void drawCloud(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y + 10.0f, z);
    glScalef(2.5f, 2.0f, 2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glPushMatrix(); glutSolidSphere(2.0f, 12, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, -0.3f, 0.0f); glutSolidSphere(1.4f, 10, 10); glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.5f, -0.3f, 0.0f); glutSolidSphere(1.4f, 10, 10); glPopMatrix();
    glPopMatrix();
}

void drawStationPlatform(float x) {
    float pCurve = getTrackCurve(x);
    glPushMatrix();
    glTranslatef(x, -2.5f, pCurve + 6.0f);
    glColor3f(0.35f, 0.35f, 0.38f);
    drawCube(35.0f, 1.4f, 4.5f);
    glColor3f(0.2f, 0.2f, 0.22f);
    for (float pX = -14.0f; pX <= 14.0f; pX += 7.0f) {
        glPushMatrix();
        glTranslatef(pX, 0.7f, 1.8f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(0.2f, 0.2f, 5.0f, 8);
        glPopMatrix();
    }
    glColor3f(0.18f, 0.24f, 0.32f);
    glPushMatrix();
    glTranslatef(0.0f, 5.8f, 0.5f); drawCube(36.0f, 0.3f, 6.0f); glPopMatrix();
    glPopMatrix();
}

void environment() {
    switch(currentScene) {
        case 0: glColor3f(0.32f, 0.58f, 0.22f); break;
        case 1: glColor3f(0.68f, 0.5f, 0.18f); break;
        case 2: glColor3f(0.12f, 0.12f, 0.18f); break;
        case 3: glColor3f(0.85f, 0.88f, 0.92f); break;
    }

    glPushMatrix(); glTranslatef(0.0f, -2.55f, 0.0f); drawCube(600.0f, 0.1f, 200.0f); glPopMatrix();
    glColor3f(0.15f, 0.45f, 0.85f);
    glPushMatrix(); glTranslatef(0.0f, -2.52f, 38.0f);
    drawCube(600.0f, 0.12f, 30.0f); glPopMatrix();

    glPushMatrix();
    glTranslatef(-environmentScrollX, 0.0f, 0.0f);

    for (float trackXSeg = -300.0f; trackXSeg <= 600.0f; trackXSeg += 2.0f) {
        float curveZ = getTrackCurve(trackXSeg);
        glColor3f(0.22f, 0.22f, 0.22f);
        glPushMatrix(); glTranslatef(trackXSeg, -2.54f, curveZ); drawCube(2.2f, 0.02f, 7.5f); glPopMatrix();
        glColor3f(0.38f, 0.38f, 0.38f);
        glPushMatrix(); glTranslatef(trackXSeg, -2.52f, curveZ);
        drawCube(2.2f, 0.05f, 6.2f); glPopMatrix();
        glColor3f(0.55f, 0.55f, 0.58f);
        glPushMatrix(); glTranslatef(trackXSeg, -2.35f, curveZ + 1.5f); drawCube(2.2f, 0.15f, 0.15f); glPopMatrix();
        glPushMatrix();
        glTranslatef(trackXSeg, -2.35f, curveZ - 1.5f); drawCube(2.2f, 0.15f, 0.15f); glPopMatrix();

        if (fmod(abs((int)trackXSeg), 4) == 0) {
            glColor3f(0.22f, 0.12f, 0.06f);
            glPushMatrix(); glTranslatef(trackXSeg, -2.45f, curveZ); drawCube(1.0f, 0.12f, 4.5f); glPopMatrix();
        }
    }

    drawMountain(-160.0f, -65.0f, 35.0f, 110.0f);
    drawMountain(-40.0f, -85.0f, 48.0f, 140.0f);
    drawMountain(90.0f, -75.0f, 42.0f, 120.0f);
    drawMountain(220.0f, -90.0f, 55.0f, 160.0f);
    drawMountain(380.0f, -70.0f, 40.0f, 130.0f);

    drawWindmill(-100.0f, -38.0f);
    drawWindmill(110.0f, -42.0f);
    drawWindmill(310.0f, -39.0f);

    drawHouse(-160.0f, -53.0f, 0.8f, 0.5f, 0.4f);
    drawHouse(-50.0f, -53.0f, 0.2f, 0.6f, 0.7f);
    drawHouse(60.0f, -53.0f, 0.85f, 0.35f, 0.2f);
    drawHouse(170.0f, -53.0f, 0.9f, 0.8f, 0.3f);
    drawHouse(290.0f, -53.0f, 0.4f, 0.7f, 0.4f);
    drawStationPlatform(-45.0f);
    drawStationPlatform(220.0f);

    // Render passengers onto platforms if active
    drawBoardingPassengers();

    for (float xPos = -260.0f; xPos <= 580.0f; xPos += 55.0f) {
        drawTree(xPos - 10.0f, -16.0f, 1.2f);
        drawTree(xPos + 15.0f, 11.0f, 1.4f);
    }

    drawCloud(-200.0f, 25.0f, -40.0f);
    drawCloud(-70.0f, 28.0f, -30.0f);
    drawCloud(60.0f, 26.0f, -50.0f);
    drawCloud(190.0f, 29.0f, -35.0f);

    glPopMatrix();
}

void drawSteamSmoke() {
    if (speedX == 0.0f) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    float tCurve = getTrackCurve(trainX);
    for (int i = 0; i < MAX_SMOKE; i++) {
        if (smokeTrail[i].alpha <= 0.0f) continue;
        glColor4f(0.8f, 0.8f, 0.82f, smokeTrail[i].alpha);
        glPushMatrix();
        glTranslatef(trainX + 4.0f + smokeTrail[i].x, 1.9f + smokeTrail[i].y, tCurve + smokeTrail[i].z);
        glScalef(smokeTrail[i].size, smokeTrail[i].size, smokeTrail[i].size);
        glutSolidIcosahedron();
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void updateLightingConfiguration() {
    glShadeModel(GL_SMOOTH);
    if (currentScene == 2) {
        glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
    } else {
        glClearColor(0.4f, 0.65f, 0.88f, 1.0f);
    }
    GLfloat lightAmb[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat lightDif[] = { 0.9f, 0.9f, 0.85f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
    GLfloat lightPos[] = { 60.0f, 90.0f, 50.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
}

void display() {
    updateLightingConfiguration();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (showInterior) {
        float carriageCenterX = trainX - 17.0f;
        float currentZ = getTrackCurve(carriageCenterX);

        float aheadSampleX = carriageCenterX + 1.0f;
        float aheadSampleZ = getTrackCurve(aheadSampleX);
        gluLookAt(carriageCenterX, -1.2f, currentZ,
                  aheadSampleX, -1.2f, aheadSampleZ,
                  0.0f, 1.0f, 0.0f);
    }
    else {
        float tX = trainX;
        float tZ = getTrackCurve(tX);

        switch (cameraMode) {
            case 0: {
                float radX = camAngleX * PI / 180.0f;
                float radY = camAngleY * PI / 180.0f;
                gluLookAt(tX + camRadius * cos(radX) * sin(radY),
                          camRadius * sin(radX) - 1.5f,
                          tZ + camRadius * cos(radX) * cos(radY),
                          tX, -1.5f, tZ,
                          0.0f, 1.0f, 0.0f);
                break;
            }
            case 1:
                gluLookAt(0.0f, 6.0f, 55.0f, 0.0f, -1.5f, tZ, 0.0f, 1.0f, 0.0f);
                break;
            case 2:
                gluLookAt(tX + 12.0f, -1.0f, getTrackCurve(tX + 12.0f), tX, -1.4f, tZ, 0.0f, 1.0f, 0.0f);
                break;
            case 3:
                gluLookAt(tX - 50.0f, 2.5f, tZ, tX - 25.0f, -1.2f, tZ, 0.0f, 1.0f, 0.0f);
                break;
            case 4:
                gluLookAt(tX - 10.0f, 1.0f, tZ - 25.0f, tX - 10.0f, -1.5f, tZ, 0.0f, 1.0f, 0.0f);
                break;
            case 5:
                gluLookAt(tX - 10.0f, 1.0f, tZ + 25.0f, tX - 10.0f, -1.5f, tZ, 0.0f, 1.0f, 0.0f);
                break;
            case 6:
                gluLookAt(tX - 12.0f, 40.0f, tZ, tX - 12.0f, -2.0f, tZ, -1.0f, 0.0f, 0.0f);
                break;
        }
    }

    environment();
    float bobEngine = sin(trainX * 1.5f) * 0.02f * (fabs(speedX) / maxSpeed);

    float engineAhead = getTrackCurve(trainX + 0.1f);
    float engineBehind = getTrackCurve(trainX - 0.1f);
    float angleEngine = atan2(engineAhead - engineBehind, 0.2f) * 180.0f / PI;

    glPushMatrix();
    glTranslatef(trainX, -1.7f + bobEngine, getTrackCurve(trainX));
    glRotatef(angleEngine, 0.0f, 1.0f, 0.0f);
    steamEngine();
    glPopMatrix();

    float tenderX = trainX - 8.6f;
    float sampleAheadTender = getTrackCurve(tenderX + 0.1f);
    float sampleBehindTender = getTrackCurve(tenderX - 0.1f);
    float angleTender = atan2(sampleAheadTender - sampleBehindTender, 0.2f) * 180.0f / PI;

    glPushMatrix();
    glTranslatef(tenderX, -1.7f + bobEngine, getTrackCurve(tenderX));
    glRotatef(angleTender, 0.0f, 1.0f, 0.0f);
    coalTender();
    glPopMatrix();

    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(trainX - 4.8f, -2.1f, getTrackCurve(trainX - 4.8f)); drawCube(0.8f, 0.2f, 0.3f);
    glPopMatrix();

    float startCarriageOffset = trainX - 17.0f;
    for (int i = 0; i < 3; i++) {
        float segX = startCarriageOffset - (i * 9.5f);
        float sampleAhead = getTrackCurve(segX + 0.1f);
        float sampleBehind = getTrackCurve(segX - 0.1f);
        float angleDeg = atan2(sampleAhead - sampleBehind, 0.2f) * 180.0f / PI;

        glColor3f(0.1f, 0.1f, 0.1f);
        glPushMatrix();
        glTranslatef(segX + 4.7f, -2.1f, getTrackCurve(segX + 4.7f)); drawCube(0.6f, 0.15f, 0.25f); glPopMatrix();

        glPushMatrix();
        glTranslatef(segX, -1.45f, getTrackCurve(segX));
        glRotatef(angleDeg, 0.0f, 1.0f, 0.0f);
        passengerCarriage();
        glPopMatrix();
    }

    drawSteamSmoke();
    drawIndicators();
    drawBoardingHUD();

    glutSwapBuffers();
}

void idle() {
    if (isPaused) {
        Sleep(10);
        return;
    }

    checkStationProximity();
    updateBoardingSimulation();

    if (useStraightTrack && trackSwitchMorph < 1.0f) {
        trackSwitchMorph += 0.02f;
        if (trackSwitchMorph > 1.0f) trackSwitchMorph = 1.0f;
    } else if (!useStraightTrack && trackSwitchMorph > 0.0f) {
        trackSwitchMorph -= 0.02f;
        if (trackSwitchMorph < 0.0f) trackSwitchMorph = 0.0f;
    }

    windmillBladeAngle += 0.6f + (fabs(speedX) * 0.15f);
    if (windmillBladeAngle > 360.0f) windmillBladeAngle -= 360.0f;

    signalFlashTimer += 0.03f;
    if (signalFlashTimer > 0.4f) {
        signalFlashState = !signalFlashState;
        signalFlashTimer = 0.0f;
    }

    if (bellRinging) {
        bellTime += 0.25f;
        bellAngle = sin(bellTime) * 35.0f;
    } else {
        bellAngle *= 0.9f;
    }

    if (doorsOpen && doorAnimationTimer < 1.0f) {
        doorAnimationTimer += 0.06f;
        if (doorAnimationTimer > 1.0f) doorAnimationTimer = 1.0f;
    } else if (!doorsOpen && doorAnimationTimer > 0.0f) {
        doorAnimationTimer -= 0.06f;
        if (doorAnimationTimer < 0.0f) doorAnimationTimer = 0.0f;
    }

    if (speedX > 0.0f) { speedX -= friction;
        if (speedX < 0.0f) speedX = 0.0f; }
    else if (speedX < 0.0f) { speedX += friction;
        if (speedX > 0.0f) speedX = 0.0f; }

    isMoving = (fabs(speedX) > 0.001f);
    if (isMoving) {
        environmentScrollX += speedX;
        wheelRotate -= (speedX * 30.0f);
        for (int i = 0; i < MAX_SMOKE; i++) {
            if (smokeTrail[i].alpha <= 0.0f) {
                smokeTrail[i].x = 0.0f;
                smokeTrail[i].y = 0.0f;
                smokeTrail[i].z = 0.0f;

                smokeTrail[i].vx = -(speedX * 0.4f) + ((rand() % 100) / 4000.0f - 0.0125f);
                smokeTrail[i].vy = 0.08f + ((rand() % 100) / 2000.0f);
                smokeTrail[i].vz = (rand() % 100) / 2000.0f - 0.025f;
                smokeTrail[i].size = 0.3f;
                smokeTrail[i].alpha = 0.8f;
            } else {
                smokeTrail[i].x += smokeTrail[i].vx;
                smokeTrail[i].y += smokeTrail[i].vy;
                smokeTrail[i].z += smokeTrail[i].vz;

                smokeTrail[i].size += 0.015f;
                smokeTrail[i].alpha -= 0.015f;
            }
        }

        if (environmentScrollX > 350.0f && speedX > 0.0f) {
            currentScene = (currentScene + 1) % 5;
            environmentScrollX = 0.0f;
            isNight = (currentScene == 2);
        }
        else if (environmentScrollX < 0.0f && speedX < 0.0f) {
            currentScene = (currentScene - 1 + 5) % 5;
            environmentScrollX = 350.0f;
            isNight = (currentScene == 2);
        }
    }

    Sleep(15);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key >= '0' && key <= '6') {
        if (!showInterior) {
            cameraMode = key - '0';
        }
    }

    switch (key) {
        case 27: exit(0); break;
        case 9: isPaused = !isPaused; break;
        case 'w': case 'W':
            if (!isPaused) { speedX += 0.20f;
                if (speedX > maxSpeed) speedX = maxSpeed; }
            break;
        case 's': case 'S':
            if (!isPaused) { speedX -= 0.20f;
                if (speedX < minSpeed) speedX = minSpeed; }
            break;
        case 'd': case 'D': doorsOpen = !doorsOpen; break;
        case 'c': case 'C':
            if (!showInterior) cameraMode = (cameraMode + 1) % 7;
            break;
        case 'i': case 'I':
            showInterior = !showInterior;
            break;
        case 'h': case 'H': bellRinging = !bellRinging; break;
        case 'l': case 'L': headlightOn = !headlightOn; break;
        case 'k': case 'K': interiorLightOn = !interiorLightOn; break;
        case 't': case 'T': useStraightTrack = !useStraightTrack; break;
        case 'q': case 'Q':
            leftSignalOn = !leftSignalOn;
            if(leftSignalOn) { rightSignalOn = false; emergencySignalOn = false; }
            break;
        case 'e': case 'E':
            rightSignalOn = !rightSignalOn;
            if(rightSignalOn) { leftSignalOn = false; emergencySignalOn = false; }
            break;
        case 'z': case 'Z':
            emergencySignalOn = !emergencySignalOn;
            if(emergencySignalOn) { leftSignalOn = false; rightSignalOn = false; }
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (cameraMode == 0 && !showInterior) {
        switch (key) {
            case GLUT_KEY_UP:    camAngleX += 3.0f; break;
            case GLUT_KEY_DOWN:  camAngleX -= 3.0f; break;
            case GLUT_KEY_LEFT:  camAngleY -= 3.0f; break;
            case GLUT_KEY_RIGHT: camAngleY += 3.0f; break;
        }
        if (camAngleX > 85.0f) camAngleX = 85.0f;
        if (camAngleX < 2.0f)  camAngleX = 2.0f;
    }
    glutPostRedisplay();
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    // Texture Load
    brickTex = loadTexture("brick.jpg");
    cout << "brickTex = " << brickTex << endl;
    initStars();

    for (int i = 0; i < MAX_SMOKE; i++) {
        smokeTrail[i].alpha = 0.0f;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Dynamic Hollow 3D Train Simulator Desk App Console");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    mountainTex = loadTexture("C:\\Users\\Hp\\OneDrive\\Documents\\train project\\mountain.jpg");
    brickTex = loadTexture("C:\\Users\\Hp\\OneDrive\\Documents\\train project\\brick.jpg");




    glutMainLoop();
    return 0;
}
