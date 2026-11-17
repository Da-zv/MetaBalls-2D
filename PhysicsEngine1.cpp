// PhysicsEngine1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#include <iostream>
#include <freeglut\include\GL\freeglut.h>
#include <glm-master\glm\glm.hpp>
#include <cmath>
#include <vector>
#include <cstdio>
#include <thread>
#include <algorithm>

using namespace std;

// Window dimensions
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

//constants
const float METABALL_THRESHOLD = 0.8f;  // Threshold to visualize the metaball
const float GRAVITY = 100.0f;
const float TIME_STEP = 0.064f;   //  0.016f = 60 FPS
const float ELASTICITY = 0.8f;    // ~0.7f
const float FRICTION_COEFF = 0.001f;  // Friction coefficient
const float BALLS_VELOCITY = 1500.0f;
const int BALLS_SIZE = 15;
const int BALLS_MASS = 15;

const int stepSize = 2;


// Ball object
struct Ball {
    glm::vec2 position;
    glm::vec2 velocity;
    float radius;
    float mass;
    bool isStatic;

    Ball(float x, float y, float radius, float mass, bool isStatic = false)
        : position(x, y), velocity(0, 0), radius(radius), mass(mass), isStatic(isStatic) {}
};

struct Box {
    glm::vec2 position;        // Center position of the box
    glm::vec2 velocity;        // Linear velocity
    glm::vec2 size;            // Half-width and half-height of the box
    glm::vec2 angularVelocity; // Angular velocity (rotation speed)
    float mass;                // Mass of the box
    float rotation;            // Rotation angle (in radians)
    bool isStatic;             // If true, the box does not move

    Box(glm::vec2 pos, glm::vec2 s, float m = 1.0f, bool isStatic = false)
        : position(pos), size(s), velocity(0.0f, 0.0f), angularVelocity(0.0f, 0.0f),
        mass(m), rotation(0.0f), isStatic(isStatic) {}
};


vector<Box> boxes;
vector<Ball> balls;

// Wall boundaries
float LEFT_WALL, RIGHT_WALL, TOP_WALL, BOTTOM_WALL;

bool isDragging = false;
glm::vec2 dragStart;
glm::vec2 dragCurrent;
Ball* selectedBall = nullptr;
float dragForce = 0.0f;
char ballsText[50];
float mouseX = 1;
float mouseY = 1;

// Initialize the scene with one ball
void initializeScene() {
    balls.push_back(Ball(0, 200, BALLS_SIZE, BALLS_MASS));
}

void clampVelocity(glm::vec2& velocity) {
    float speed = glm::length(velocity);
    if (speed > BALLS_VELOCITY) {
        velocity = glm::normalize(velocity) * BALLS_VELOCITY;
    }
}

void handleBallBoxCollision(Ball& ball, Box& box) {
    // Calculate the closest point on the box to the ball
    glm::vec2 boxHalfSize = box.size * 0.5f;
    glm::vec2 closestPoint = glm::clamp(ball.position, box.position - boxHalfSize, box.position + boxHalfSize);

    // Calculate the distance and penetration
    glm::vec2 delta = ball.position - closestPoint;
    float distanceSquared = glm::dot(delta, delta);
    float radius = ball.radius;

    if (distanceSquared < radius * radius) {
        float distance = glm::sqrt(distanceSquared);
        glm::vec2 collisionNormal = (distance > 0.0f) ? glm::normalize(delta) : glm::vec2(0.0f, 1.0f);
        float penetrationDepth = radius - distance;

        // Resolve the collision
        ball.position += collisionNormal * penetrationDepth;

        // Reflect the velocity of the ball
        ball.velocity = glm::reflect(ball.velocity, collisionNormal) * ELASTICITY;

        // Apply impulse to the box if it's dynamic
        if (!box.isStatic) {
            box.velocity -= collisionNormal * (penetrationDepth / 2.0f); // Simple impulse transfer
        }
    }
}

void updateBoxes() {
    for (auto& box : boxes) {
        if (box.isStatic) continue;

        // Apply gravity to dynamic boxes
        box.velocity.y -= GRAVITY * TIME_STEP;

        // Update position
        box.position += box.velocity * TIME_STEP;

        // Clamp velocity to avoid instability
        clampVelocity(box.velocity);

        // Check for collisions with walls
        if (box.position.x - box.size.x / 2 < LEFT_WALL || box.position.x + box.size.x / 2 > RIGHT_WALL) {
            box.velocity.x = -box.velocity.x * ELASTICITY;
        }
        if (box.position.y - box.size.y / 2 < BOTTOM_WALL || box.position.y + box.size.y / 2 > TOP_WALL) {
            box.velocity.y = -box.velocity.y * ELASTICITY;
        }
    }
}

// Physics update function
void updatePhysics() {
    for (auto& ball : balls) {
        if (ball.isStatic || (selectedBall && &ball == selectedBall && isDragging)) {
            continue;  // Skip the physics update for static balls or the selected ball while dragging
        }

        // gravity based on the ball's mass
        ball.velocity.y -= GRAVITY * TIME_STEP;

        // Update position
        ball.position = ball.position + ball.velocity * TIME_STEP;

        // friction when ball is on a surface
        if (ball.position.y - ball.radius <= BOTTOM_WALL) {
            float frictionForce = FRICTION_COEFF * (ball.mass * GRAVITY);
            glm::vec2 friction = glm::normalize(ball.velocity) * -frictionForce * TIME_STEP;

            if (glm::length(friction) < glm::length(ball.velocity)) {
                ball.velocity += friction;  // Apply friction in the opposite direction of velocity
            }
            else {
                ball.velocity = glm::vec2(0, 0);  // Stop the ball if friction exceeds current velocity
            }
        }

        // Handle wall collisions with elasticity
        if (ball.position.x - ball.radius < LEFT_WALL) {
            ball.position.x = LEFT_WALL + ball.radius;
            ball.velocity.x = -ball.velocity.x * ELASTICITY;
        }
        if (ball.position.x + ball.radius > RIGHT_WALL) {
            ball.position.x = RIGHT_WALL - ball.radius;
            ball.velocity.x = -ball.velocity.x * ELASTICITY;
        }
        if (ball.position.y - ball.radius < BOTTOM_WALL) {
            ball.position.y = BOTTOM_WALL + ball.radius;
            ball.velocity.y = -ball.velocity.y * ELASTICITY;
        }
        if (ball.position.y + ball.radius > TOP_WALL) {
            ball.position.y = TOP_WALL - ball.radius;
            ball.velocity.y = -ball.velocity.y * ELASTICITY;
        }

        // Handle ball-ball collisions
        for (auto& other : balls) {
            if (&ball == &other || other.isStatic) continue;

            glm::vec2 delta = ball.position - other.position;
            float dist = glm::length(delta);
            float minDist = ball.radius + other.radius;

            // Check if balls are colliding
            if (dist < minDist) {
                glm::vec2 normal = glm::normalize(delta);

                // Calculate relative velocity along the collision normal
                glm::vec2 relativeVelocity = ball.velocity - other.velocity;
                float separatingVelocity = glm::dot(relativeVelocity, normal);

                // Only resolve collision if the balls are moving toward each other
                if (separatingVelocity < 0) {
                    // Calculate impulse magnitude based on the elasticity and masses of the balls
                    float impulse = -(1 + ELASTICITY) * separatingVelocity / (1 / ball.mass + 1 / other.mass);

                    // Apply impulse to each ball's velocity
                    glm::vec2 impulseVector = normal * impulse;
                    ball.velocity += impulseVector * (1.0f / ball.mass);
                    other.velocity -= impulseVector * (1.0f / other.mass);

                    // Position correction: Separate balls to avoid overlap
                    float overlap = minDist - dist + 0.01f;  // Small buffer to ensure no overlap
                    glm::vec2 correction = normal * (overlap / 2);
                    ball.position += correction;
                    other.position -= correction;
                }
            }
        }

        // Handle collision with static balls
        for (const auto& other : balls) {
            if (&ball == &other || !other.isStatic) continue; // Only check against static balls

            glm::vec2 delta = ball.position - other.position;
            float dist = glm::length(delta);
            float minDist = ball.radius + other.radius;

            // Check if moving ball is colliding with static ball
            if (dist < minDist) {
                glm::vec2 normal = glm::normalize(delta);

                // Calculate relative velocity along the collision normal
                glm::vec2 relativeVelocity = ball.velocity; // Static ball has zero velocity
                float separatingVelocity = glm::dot(relativeVelocity, normal);

                // Only resolve collision if the moving ball is moving toward the static ball
                if (separatingVelocity < 0) {
                    // Calculate impulse magnitude based on the elasticity and mass of the moving ball
                    float impulse = -(1 + ELASTICITY) * separatingVelocity / (1 / ball.mass);

                    // Apply impulse to the moving ball's velocity
                    glm::vec2 impulseVector = normal * impulse;
                    ball.velocity += impulseVector * (1.0f / ball.mass);

                    // Position correction: Separate balls to avoid overlap
                    float overlap = minDist - dist + 0.01f;  // Small buffer to ensure no overlap
                    glm::vec2 correction = normal * overlap;
                    ball.position += correction; // Move the ball out of overlap
                }
            }
        }
        // Check for collisions with boxes
        for (auto& box : boxes) {
            handleBallBoxCollision(ball, box);
        }
        clampVelocity(ball.velocity);
    }
    updateBoxes();
}


// Timer callback for periodic physics updates
void timer(int) {
    updatePhysics();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  //16 = ~60 FPS
}

// Mouse callback to create new balls
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float mouseX = x - WINDOW_WIDTH / 2;
        float mouseY = WINDOW_HEIGHT / 2 - y;
        balls.push_back(Ball(mouseX, mouseY, BALLS_SIZE, BALLS_MASS)); // New ball with same radius, mass
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        float mouseX = x - WINDOW_WIDTH / 2;
        float mouseY = WINDOW_HEIGHT / 2 - y;
        balls.push_back(Ball(mouseX, mouseY, BALLS_SIZE, BALLS_MASS, true)); // New ball with same radius, static
    }

}

void spawnBox(glm::vec2 position, glm::vec2 size, bool isStatic = true) {
    boxes.emplace_back(position, size, isStatic);
}

void keyDown(unsigned char key, int x, int y) {
    if (key == 32 && !isDragging) {  // ASCII code 32 for spacebar
        float mouseX = x - WINDOW_WIDTH / 2;
        float mouseY = WINDOW_HEIGHT / 2 - y;

        // Find the closest non-static ball to start dragging
        for (auto& ball : balls) {
            float dist = sqrt(pow(mouseX - ball.position.x, 2) + pow(mouseY - ball.position.y, 2));
            if (dist <= ball.radius && !ball.isStatic) {
                isDragging = true;
                dragStart = glm::vec2(mouseX, mouseY);
                selectedBall = &ball;
                selectedBall->velocity = glm::vec2(0, 0);  // Stop ball movement while dragging
                break;
            }
        }
    }

    if (key == 'b') {
        float mouseX = x - WINDOW_WIDTH / 2;
        float mouseY = WINDOW_HEIGHT / 2 - y;
        spawnBox(glm::vec2(mouseX, mouseY), glm::vec2(50.0f, 50.0f), false); // Example: spawn dynamic box
    }

    // Calculate the force if dragging
    if (isDragging && selectedBall) {
        float mouseX = x - WINDOW_WIDTH / 2;
        float mouseY = WINDOW_HEIGHT / 2 - y;
        glm::vec2 dragEnd(mouseX, mouseY);
        dragForce = (dragStart - dragEnd).length();  // Calculate force based on drag distance
    }
    // Press r to remove all balls
    if (key == 114) {
        balls.clear();
        boxes.clear();
    }
}


void keyUp(unsigned char key, int x, int y) {
    if (key == 32 && isDragging && selectedBall) {  // Spacebar released
        mouseX = x - WINDOW_WIDTH / 2;
        mouseY = WINDOW_HEIGHT / 2 - y;

        // Calculate drag vector and apply opposite velocity to the ball
        glm::vec2 dragEnd(mouseX, mouseY);
        glm::vec2 dragVector = dragStart - dragEnd;  // Vector from drag start to current mouse position
        float forceMagnitude = dragVector.length();

        // Set the ball's velocity based on the drag distance (scaling factor to control strength)
        float launchSpeed = 1000.0f;//forceMagnitude * 2.0f;  // Adjust scaling factor as needed  // todo fix this to calculate the force correctly 
        selectedBall->velocity = normalize(dragVector) * launchSpeed;

        // Reset drag state and force display
        isDragging = false;
        dragForce = 0.0f;  // Clear force display
        selectedBall = nullptr;
    }
}

void passiveMouseMotion(int x, int y) {
    if (isDragging && selectedBall) {
        // Get the mouse position relative to the center of the window
        mouseX = x - WINDOW_WIDTH / 2;
        mouseY = WINDOW_HEIGHT / 2 - y;

        // Update the dragCurrent position
        dragCurrent = glm::vec2(mouseX, mouseY);

        // Calculate drag distance (force)
        dragForce = (dragStart - dragCurrent).length(); // Calculate the force
    }
}

// Reshape callback for resizing the window
void reshape(int width, int height) {
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;

    // Adjust the viewport
    glViewport(0, 0, width, height);

    // Adjust the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-width / 2, width / 2, -height / 2, height / 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Update wall boundaries
    LEFT_WALL = -width / 2;
    RIGHT_WALL = width / 2;
    BOTTOM_WALL = -height / 2;
    TOP_WALL = height / 2;
}

void renderText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}


struct BallData {
    float x;
    float y;
    float radiusSquared;
    bool isStatic;
};

struct RenderPoint {
    float x;
    float y;
};

struct BallSnapshot {
    float x;
    float y;
    float radiusSquared;
};

void renderMetaballs() {

    // copy metaballs data, so threads have safer, unchanging data set.
    std::vector<BallSnapshot> activeBalls;

    for (const auto& ball : balls) {
        if (ball.isStatic)
            continue;

        activeBalls.push_back({
            ball.position.x,
            ball.position.y,
            ball.radius * ball.radius
        });
    }

    const int yStart = -WINDOW_HEIGHT / 2;
    const int yEnd = WINDOW_HEIGHT / 2;

    const int rows =
        (yEnd - yStart + stepSize - 1) / stepSize;

    // Amount of available cpu cores
    unsigned int threadCount = std::thread::hardware_concurrency();

    if (threadCount == 0)
        threadCount = 1;

    threadCount = std::min(
        threadCount,
        static_cast<unsigned int>(rows)
    );

    // Each thread has its own list of points
    std::vector<std::vector<RenderPoint>> results(threadCount);

    std::vector<std::thread> threads;

    // Start threads
    for (unsigned int t = 0; t < threadCount; ++t) {

        threads.emplace_back([&, t]() {

            int rowBegin = rows * t / threadCount;
            int rowEnd = rows * (t + 1) / threadCount;

            auto& output = results[t];

            for (int row = rowBegin; row < rowEnd; ++row) {

                int j = yStart + row * stepSize;

                for (int i = -WINDOW_WIDTH / 2;
                     i < WINDOW_WIDTH / 2;
                     i += stepSize) {

                    float influence = 0.0f;

                    for (const auto& ball : activeBalls) {

                        float dx = i - ball.x;
                        float dy = j - ball.y;

                        float distSq = dx * dx + dy * dy;

                        if (distSq > 0.0f) {
                            influence += ball.radiusSquared / distSq;
                        }

                        if (influence >= METABALL_THRESHOLD)
                            break;
                    }

                    if (influence > METABALL_THRESHOLD) {
                        output.push_back({
                            static_cast<float>(i),
                            static_cast<float>(j)
                        });
                    }
                }
            }
        });
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Render only in main thread OpenGL
    glPointSize(stepSize);
    glBegin(GL_POINTS);

    for (const auto& output : results) {
        for (const auto& point : output) {
            glVertex2f(point.x, point.y);
        }
    }

    glEnd();
}



void renderBoxes() {
    glBegin(GL_QUADS);
    for (const auto& box : boxes) {
        glm::vec2 halfSize = box.size * 0.5f;
        glVertex2f(box.position.x - halfSize.x, box.position.y - halfSize.y);
        glVertex2f(box.position.x + halfSize.x, box.position.y - halfSize.y);
        glVertex2f(box.position.x + halfSize.x, box.position.y + halfSize.y);
        glVertex2f(box.position.x - halfSize.x, box.position.y + halfSize.y);
    }
    glEnd();
}

// Display callback for OpenGL rendering
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    renderMetaballs();
    renderBoxes();
    
    // Draw balls
    for (const auto& ball : balls) {
        if (!ball.isStatic) {
            // Skip rendering if the ball is static
            continue;
        }
        glPushMatrix();
        glTranslatef(ball.position.x, ball.position.y, 0);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < 360; i += 10) {
            float angle = i * 3.14159 / 180;
            glVertex2f(cos(angle) * ball.radius, sin(angle) * ball.radius);
        }
        glEnd();
        glPopMatrix();
    }

    // If dragging, display the line from dragStart to current mouse position
    if (isDragging) {
        glColor3f(1.0f, 1.0f, 1.0f);  // R,G,B color for the line
        glBegin(GL_LINES);
        glVertex2f(dragStart.x, dragStart.y);  // Start of the line (dragStart)
        glVertex2f(dragCurrent.x, dragCurrent.y);  // End of the line (current mouse position)
        glEnd();

        // Display the force if dragging
        char forceText[50];
        snprintf(forceText, sizeof(forceText), "Force: %.2f", dragForce);
        renderText(-WINDOW_WIDTH / 2 + 10, WINDOW_HEIGHT / 2 - 40, forceText);  // Display in top-left corner
    }

    snprintf(ballsText, sizeof(ballsText), "Balls: %d", balls.size());
    renderText(-WINDOW_WIDTH / 2 + 10, WINDOW_HEIGHT / 2 - 20, ballsText);  // Display in top-left corner

    glutSwapBuffers();
}





// OpenGL initialization
void initOpenGL() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WINDOW_WIDTH / 2, WINDOW_WIDTH / 2, -WINDOW_HEIGHT / 2, WINDOW_HEIGHT / 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("2D Physics Simulation with Friction");

    initOpenGL();
    initializeScene();

    // Register the callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);

    // Register passive motion callback for mouse tracking
    glutPassiveMotionFunc(passiveMouseMotion);

    glutMainLoop();

    return 0;
}