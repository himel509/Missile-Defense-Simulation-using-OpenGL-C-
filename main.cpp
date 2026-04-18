#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

const float PI = 3.14159265358979323846f;

static HDC g_hdc = nullptr;
static HGLRC g_glContext = nullptr;
static LARGE_INTEGER g_qpcFrequency = {};
static LARGE_INTEGER g_lastTick = {};
static float g_animationTime = 0.0f;

constexpr UINT_PTR kAnimationTimerId = 1;
constexpr UINT kAnimationIntervalMs = 16;

enum class BattleState {
    AwaitingLaunch,
    InterceptInProgress,
    PlaneDestroyed,
    TankDestroyed
};

static BattleState g_battleState = BattleState::AwaitingLaunch;
static float g_battleTimer = 0.0f;
static int g_viewportWidth = 1000;
static int g_viewportHeight = 600;
static float g_mouseWorldX = -1.0f;
static float g_mouseWorldY = -1.0f;

constexpr float kWorldWidth = 1000.0f;
constexpr float kWorldHeight = 600.0f;

constexpr float kLaunchButtonLeft = 56.0f;
constexpr float kLaunchButtonRight = 222.0f;
constexpr float kLaunchButtonBottom = 244.0f;
constexpr float kLaunchButtonTop = 288.0f;

constexpr float kAwaitTimeout = 4.6f;
constexpr float kInterceptDuration = 2.8f;
constexpr float kPlaneFallDuration = 2.5f;
constexpr float kTankBlastDuration = 2.6f;

float wrapValue(float value, float minValue, float maxValue) {
    float range = maxValue - minValue;
    float wrapped = std::fmod(value - minValue, range);
    if (wrapped < 0.0f) {
        wrapped += range;
    }
    return minValue + wrapped;
}

float clamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

bool pointInRect(float px, float py, float left, float bottom, float right, float top) {
    return px >= left && px <= right && py >= bottom && py <= top;
}

void screenToWorld(int mouseX, int mouseY, float& worldX, float& worldY) {
    float nx = (g_viewportWidth > 0) ? static_cast<float>(mouseX) / static_cast<float>(g_viewportWidth) : 0.0f;
    float ny = (g_viewportHeight > 0) ? static_cast<float>(mouseY) / static_cast<float>(g_viewportHeight) : 0.0f;
    worldX = nx * kWorldWidth;
    worldY = (1.0f - ny) * kWorldHeight;
}

void triggerLaunch() {
    if (g_battleState == BattleState::AwaitingLaunch) {
        g_battleState = BattleState::InterceptInProgress;
        g_battleTimer = 0.0f;
    }
}

void updateBattleState(float deltaSeconds) {
    g_battleTimer += deltaSeconds;

    switch (g_battleState) {
        case BattleState::AwaitingLaunch:
            if (g_battleTimer >= kAwaitTimeout) {
                g_battleState = BattleState::TankDestroyed;
                g_battleTimer = 0.0f;
            }
            break;
        case BattleState::InterceptInProgress:
            if (g_battleTimer >= kInterceptDuration) {
                g_battleState = BattleState::PlaneDestroyed;
                g_battleTimer = 0.0f;
            }
            break;
        case BattleState::PlaneDestroyed:
            if (g_battleTimer > kPlaneFallDuration) {
                g_battleTimer = kPlaneFallDuration;
            }
            break;
        case BattleState::TankDestroyed:
            if (g_battleTimer > kTankBlastDuration) {
                g_battleTimer = kTankBlastDuration;
            }
            break;
    }
}

void drawLaunchButton() {
    bool enabled = g_battleState == BattleState::AwaitingLaunch;
    bool hovered = enabled && pointInRect(g_mouseWorldX, g_mouseWorldY,
                                          kLaunchButtonLeft, kLaunchButtonBottom,
                                          kLaunchButtonRight, kLaunchButtonTop);

    // Shadow
    glColor3f(0.04f, 0.07f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(kLaunchButtonLeft + 4.0f, kLaunchButtonBottom - 4.0f);
    glVertex2f(kLaunchButtonRight + 4.0f, kLaunchButtonBottom - 4.0f);
    glVertex2f(kLaunchButtonRight + 4.0f, kLaunchButtonTop - 4.0f);
    glVertex2f(kLaunchButtonLeft + 4.0f, kLaunchButtonTop - 4.0f);
    glEnd();

    if (!enabled) {
        glColor3f(0.14f, 0.18f, 0.22f);
    } else if (hovered) {
        glColor3f(0.24f, 0.46f, 0.22f);
    } else {
        glColor3f(0.18f, 0.34f, 0.18f);
    }
    glBegin(GL_QUADS);
    glVertex2f(kLaunchButtonLeft, kLaunchButtonBottom);
    glVertex2f(kLaunchButtonRight, kLaunchButtonBottom);
    glVertex2f(kLaunchButtonRight, kLaunchButtonTop);
    glVertex2f(kLaunchButtonLeft, kLaunchButtonTop);
    glEnd();

    glColor3f(0.62f, 0.78f, 0.60f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(kLaunchButtonLeft, kLaunchButtonBottom);
    glVertex2f(kLaunchButtonRight, kLaunchButtonBottom);
    glVertex2f(kLaunchButtonRight, kLaunchButtonTop);
    glVertex2f(kLaunchButtonLeft, kLaunchButtonTop);
    glEnd();

    // Launch icon
    glColor3f(0.90f, 0.26f, 0.16f);
    glBegin(GL_TRIANGLES);
    glVertex2f(kLaunchButtonLeft + 106.0f, kLaunchButtonBottom + 12.0f);
    glVertex2f(kLaunchButtonLeft + 140.0f, kLaunchButtonBottom + 22.0f);
    glVertex2f(kLaunchButtonLeft + 112.0f, kLaunchButtonBottom + 32.0f);
    glEnd();

    glColor3f(0.94f, 0.96f, 0.97f);
    glBegin(GL_TRIANGLES);
    glVertex2f(kLaunchButtonLeft + 140.0f, kLaunchButtonBottom + 22.0f);
    glVertex2f(kLaunchButtonLeft + 150.0f, kLaunchButtonBottom + 24.0f);
    glVertex2f(kLaunchButtonLeft + 112.0f, kLaunchButtonBottom + 32.0f);
    glEnd();

    glColor3f(0.20f, 0.22f, 0.26f);
    glBegin(GL_QUADS);
    glVertex2f(kLaunchButtonLeft + 18.0f, kLaunchButtonBottom + 10.0f);
    glVertex2f(kLaunchButtonLeft + 86.0f, kLaunchButtonBottom + 10.0f);
    glVertex2f(kLaunchButtonLeft + 86.0f, kLaunchButtonBottom + 34.0f);
    glVertex2f(kLaunchButtonLeft + 18.0f, kLaunchButtonBottom + 34.0f);
    glEnd();

    glColor3f(0.72f, 0.80f, 0.72f);
    glLineWidth(1.8f);
    glBegin(GL_LINES);
    glVertex2f(kLaunchButtonLeft + 24.0f, kLaunchButtonBottom + 28.0f);
    glVertex2f(kLaunchButtonLeft + 80.0f, kLaunchButtonBottom + 28.0f);
    glVertex2f(kLaunchButtonLeft + 24.0f, kLaunchButtonBottom + 20.0f);
    glVertex2f(kLaunchButtonLeft + 80.0f, kLaunchButtonBottom + 20.0f);
    glEnd();

    if (enabled) {
        float timeLeft = clamp01(1.0f - g_battleTimer / kAwaitTimeout);
        glColor3f(0.74f, 0.18f, 0.14f);
        glBegin(GL_QUADS);
        glVertex2f(kLaunchButtonLeft + 8.0f, kLaunchButtonTop - 7.0f);
        glVertex2f(kLaunchButtonLeft + 8.0f + timeLeft * (kLaunchButtonRight - kLaunchButtonLeft - 16.0f), kLaunchButtonTop - 7.0f);
        glVertex2f(kLaunchButtonLeft + 8.0f + timeLeft * (kLaunchButtonRight - kLaunchButtonLeft - 16.0f), kLaunchButtonTop - 3.0f);
        glVertex2f(kLaunchButtonLeft + 8.0f, kLaunchButtonTop - 3.0f);
        glEnd();
    }
}

void drawWheelSpokes(float cx, float cy, float radius, float angleDeg) {
    glColor3f(0.78f, 0.82f, 0.78f);
    glLineWidth(1.4f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; ++i) {
        float rad = (angleDeg + static_cast<float>(i) * 90.0f) * PI / 180.0f;
        glVertex2f(cx, cy);
        glVertex2f(cx + radius * std::cos(rad), cy + radius * std::sin(rad));
    }
    glEnd();
}

void drawFriendlyMissile(float x, float y) {
    glColor3f(0.70f, 0.72f, 0.75f);
    glBegin(GL_POLYGON);
    glVertex2f(x, y);
    glVertex2f(x + 52.0f, y + 32.0f);
    glVertex2f(x + 58.0f, y + 22.0f);
    glVertex2f(x + 6.0f, y - 10.0f);
    glEnd();

    glColor3f(0.92f, 0.93f, 0.95f);
    glBegin(GL_POLYGON);
    glVertex2f(x + 2.0f, y - 2.0f);
    glVertex2f(x + 50.0f, y + 28.0f);
    glVertex2f(x + 53.0f, y + 24.0f);
    glVertex2f(x + 5.0f, y - 6.0f);
    glEnd();

    glColor3f(0.60f, 0.63f, 0.66f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y);
    glVertex2f(x - 5.0f, y - 3.0f);
    glVertex2f(x + 4.0f, y - 5.0f);
    glEnd();

    glColor3f(0.90f, 0.12f, 0.11f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x + 52.0f, y + 32.0f);
    glVertex2f(x + 67.0f, y + 34.0f);
    glVertex2f(x + 58.0f, y + 22.0f);
    glEnd();
}

void drawEnemyMissile(float x, float y, float angleDeg, float flameScale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);

    glColor3f(0.70f, 0.71f, 0.73f);
    glBegin(GL_POLYGON);
    glVertex2f(-16.0f, 4.0f);
    glVertex2f(10.0f, 4.0f);
    glVertex2f(10.0f, -4.0f);
    glVertex2f(-16.0f, -4.0f);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex2f(10.0f, 4.0f);
    glVertex2f(18.0f, 0.0f);
    glVertex2f(10.0f, -4.0f);

    glVertex2f(-11.0f, 4.0f);
    glVertex2f(-18.0f, 8.0f);
    glVertex2f(-8.0f, 4.0f);

    glVertex2f(-11.0f, -4.0f);
    glVertex2f(-18.0f, -8.0f);
    glVertex2f(-8.0f, -4.0f);
    glEnd();

    glColor3f(0.98f, 0.30f, 0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-16.0f, 0.0f);
    glVertex2f(-24.0f * flameScale, 3.5f * flameScale);
    glVertex2f(-24.0f * flameScale, -3.5f * flameScale);
    glEnd();

    glPopMatrix();
}

void drawFighterPlane(float x, float y, float scale, float rollDeg, bool damaged) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);
    glScalef(scale, scale, 1.0f);

    float pulse = 0.5f + 0.5f * std::sin(g_animationTime * 10.0f + x * 0.02f);

    // Main fuselage
    glColor3f(0.78f, 0.80f, 0.84f);
    glBegin(GL_POLYGON);
    glVertex2f(-54.0f, -2.0f);
    glVertex2f(-38.0f, 8.0f);
    glVertex2f(-6.0f, 11.0f);
    glVertex2f(28.0f, 8.0f);
    glVertex2f(52.0f, 2.0f);
    glVertex2f(40.0f, -6.0f);
    glVertex2f(2.0f, -11.0f);
    glVertex2f(-32.0f, -9.0f);
    glEnd();

    // Top shading strip
    glColor3f(0.90f, 0.92f, 0.95f);
    glBegin(GL_POLYGON);
    glVertex2f(-38.0f, 5.0f);
    glVertex2f(-8.0f, 8.0f);
    glVertex2f(24.0f, 6.0f);
    glVertex2f(34.0f, 2.0f);
    glVertex2f(-30.0f, 1.0f);
    glEnd();

    // Belly shading strip
    glColor3f(0.54f, 0.58f, 0.63f);
    glBegin(GL_POLYGON);
    glVertex2f(-26.0f, -2.0f);
    glVertex2f(26.0f, -3.0f);
    glVertex2f(34.0f, -6.0f);
    glVertex2f(-20.0f, -7.0f);
    glEnd();

    // Nose cone
    glColor3f(0.98f, 0.98f, 0.99f);
    glBegin(GL_TRIANGLES);
    glVertex2f(52.0f, 2.0f);
    glVertex2f(64.0f, 0.0f);
    glVertex2f(40.0f, -6.0f);
    glEnd();

    // Main wings
    glColor3f(0.86f, 0.88f, 0.92f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-2.0f, 6.0f);
    glVertex2f(30.0f, 30.0f);
    glVertex2f(22.0f, 5.0f);

    glVertex2f(-2.0f, -5.0f);
    glVertex2f(30.0f, -30.0f);
    glVertex2f(22.0f, -7.0f);
    glEnd();

    // Rear fins
    glColor3f(0.70f, 0.74f, 0.79f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-24.0f, 5.0f);
    glVertex2f(-7.0f, 22.0f);
    glVertex2f(1.0f, 6.0f);

    glVertex2f(-24.0f, -3.0f);
    glVertex2f(-4.0f, -20.0f);
    glVertex2f(0.0f, -5.0f);
    glEnd();

    // Cockpit canopy and glass highlight
    glColor3f(0.09f, 0.12f, 0.17f);
    glBegin(GL_POLYGON);
    glVertex2f(-4.0f, 5.0f);
    glVertex2f(16.0f, 5.0f);
    glVertex2f(20.0f, 1.0f);
    glVertex2f(4.0f, -1.0f);
    glEnd();

    glColor3f(0.68f, 0.78f, 0.90f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 4.0f);
    glVertex2f(9.0f, 4.0f);
    glVertex2f(10.0f, 2.0f);
    glVertex2f(2.0f, 1.0f);
    glEnd();

    // Engine nozzle
    glColor3f(0.34f, 0.38f, 0.44f);
    glBegin(GL_POLYGON);
    glVertex2f(-54.0f, -2.0f);
    glVertex2f(-64.0f, 0.0f);
    glVertex2f(-54.0f, 3.0f);
    glVertex2f(-48.0f, 1.0f);
    glEnd();

    // Underslung pods
    glColor3f(0.62f, 0.66f, 0.71f);
    glBegin(GL_POLYGON);
    glVertex2f(6.0f, -10.0f);
    glVertex2f(16.0f, -10.0f);
    glVertex2f(15.0f, -14.0f);
    glVertex2f(7.0f, -14.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(22.0f, -9.0f);
    glVertex2f(31.0f, -8.0f);
    glVertex2f(30.0f, -12.0f);
    glVertex2f(22.0f, -12.0f);
    glEnd();

    // Panel lines
    glColor3f(0.46f, 0.50f, 0.56f);
    glLineWidth(1.4f);
    glBegin(GL_LINES);
    glVertex2f(-32.0f, -1.0f);
    glVertex2f(34.0f, -1.0f);
    glVertex2f(-16.0f, 6.0f);
    glVertex2f(26.0f, 3.0f);
    glVertex2f(-12.0f, -6.0f);
    glVertex2f(22.0f, -5.0f);
    glEnd();

    // Nose probe and fuselage outline
    glColor3f(0.20f, 0.24f, 0.30f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2f(64.0f, 0.0f);
    glVertex2f(74.0f, 0.0f);
    glEnd();

    glLineWidth(1.1f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-54.0f, -2.0f);
    glVertex2f(-38.0f, 8.0f);
    glVertex2f(-6.0f, 11.0f);
    glVertex2f(28.0f, 8.0f);
    glVertex2f(52.0f, 2.0f);
    glVertex2f(40.0f, -6.0f);
    glVertex2f(2.0f, -11.0f);
    glVertex2f(-32.0f, -9.0f);
    glEnd();

    // Wingtip missiles
    glColor3f(0.86f, 0.14f, 0.12f);
    glBegin(GL_POLYGON);
    glVertex2f(28.0f, 28.0f);
    glVertex2f(37.0f, 25.0f);
    glVertex2f(43.0f, 27.0f);
    glVertex2f(36.0f, 32.0f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(28.0f, -28.0f);
    glVertex2f(37.0f, -25.0f);
    glVertex2f(43.0f, -27.0f);
    glVertex2f(36.0f, -32.0f);
    glEnd();

    glColor3f(0.96f, 0.96f, 0.98f);
    glBegin(GL_TRIANGLES);
    glVertex2f(43.0f, 27.0f);
    glVertex2f(50.0f, 28.0f);
    glVertex2f(36.0f, 32.0f);

    glVertex2f(43.0f, -27.0f);
    glVertex2f(50.0f, -28.0f);
    glVertex2f(36.0f, -32.0f);
    glEnd();

    // Roundel and rivet details
    glColor3f(0.24f, 0.30f, 0.38f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    for (int a = 0; a < 360; a += 20) {
        float rad = static_cast<float>(a) * PI / 180.0f;
        glVertex2f(-14.0f + 4.8f * std::cos(rad), 0.0f + 4.8f * std::sin(rad));
    }
    glEnd();

    glColor3f(0.72f, 0.78f, 0.86f);
    glBegin(GL_POLYGON);
    for (int a = 0; a < 360; a += 24) {
        float rad = static_cast<float>(a) * PI / 180.0f;
        glVertex2f(-14.0f + 2.2f * std::cos(rad), 0.0f + 2.2f * std::sin(rad));
    }
    glEnd();

    glColor3f(0.52f, 0.56f, 0.62f);
    glPointSize(1.9f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 7; ++i) {
        glVertex2f(-30.0f + static_cast<float>(i) * 10.0f, -1.0f);
    }
    glEnd();

    if (damaged) {
        glColor3f(1.0f, 0.64f, 0.20f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-55.0f, 0.0f);
        glVertex2f(-82.0f, 8.0f);
        glVertex2f(-82.0f, -8.0f);
        glEnd();

        glColor3f(1.0f, 0.30f, 0.10f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-60.0f, 0.0f);
        glVertex2f(-94.0f, 6.0f);
        glVertex2f(-94.0f, -6.0f);
        glEnd();

        glColor3f(0.98f, 0.88f, 0.34f);
        glPointSize(2.6f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 18; ++i) {
            float tx = -50.0f - static_cast<float>(i) * 4.8f;
            float ty = static_cast<float>((i % 7) - 3) * 1.9f;
            glVertex2f(tx, ty);
        }
        glEnd();

        glColor3f(0.18f, 0.18f, 0.20f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 16; ++i) {
            float tx = -70.0f - static_cast<float>(i) * 5.5f;
            float ty = static_cast<float>((i % 6) - 3) * 2.6f;
            glVertex2f(tx, ty);
        }
        glEnd();
    } else {
        glColor3f(0.95f, 0.58f + 0.22f * pulse, 0.18f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-55.0f, 0.0f);
        glVertex2f(-71.0f - 5.0f * pulse, 4.0f);
        glVertex2f(-71.0f - 5.0f * pulse, -4.0f);
        glEnd();
    }

    glPopMatrix();
}

void initAnimationClock() {
    QueryPerformanceFrequency(&g_qpcFrequency);
    QueryPerformanceCounter(&g_lastTick);
}

void updateAnimationClock() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    double deltaSeconds = static_cast<double>(now.QuadPart - g_lastTick.QuadPart) /
                          static_cast<double>(g_qpcFrequency.QuadPart);
    if (deltaSeconds < 0.0) {
        deltaSeconds = 0.0;
    }
    if (deltaSeconds > 0.05) {
        deltaSeconds = 0.05;
    }

    float deltaTime = static_cast<float>(deltaSeconds);
    g_animationTime += deltaTime;
    updateBattleState(deltaTime);
    g_lastTick = now;
}

bool setupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) {
        return false;
    }

    return SetPixelFormat(hdc, pixelFormat, &pfd) == TRUE;
}

void draw() {
    glClearColor(0.03f, 0.06f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1000.0, 0.0, 600.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const float wheelSpinDeg = g_animationTime * 620.0f;
    const float tankOffsetX = 20.0f * std::sin(g_animationTime * 0.45f);
    const float tankOffsetY = 2.4f * std::sin(g_animationTime * 2.10f);
    const float redCarOffsetX = wrapValue(430.0f + g_animationTime * 95.0f, -220.0f, 1220.0f) - 430.0f;
    const float blueCarOffsetX = wrapValue(820.0f - g_animationTime * 120.0f, -220.0f, 1220.0f) - 820.0f;
    const float planeLeadX = wrapValue(1080.0f - g_animationTime * 86.0f, -220.0f, 1260.0f);
    const float planeLeadY = 536.0f + 7.0f * std::sin(g_animationTime * 2.2f);
    const float targetPreHitX = 914.0f - g_animationTime * 40.0f;
    const float targetPreHitY = 514.0f - g_animationTime * 12.0f;
    const float hitX = 770.0f;
    const float hitY = 470.0f;
    const float tankLaunchX = 360.0f + tankOffsetX;
    const float tankLaunchY = 314.0f + tankOffsetY;
    const bool waitingLaunch = g_battleState == BattleState::AwaitingLaunch;
    const bool intercepting = g_battleState == BattleState::InterceptInProgress;
    const bool planeDestroyed = g_battleState == BattleState::PlaneDestroyed;
    const bool tankDestroyed = g_battleState == BattleState::TankDestroyed;
    const float interceptT = intercepting ? clamp01(g_battleTimer / kInterceptDuration) : (planeDestroyed ? 1.0f : 0.0f);
    const float planeFallT = planeDestroyed ? clamp01(g_battleTimer / kPlaneFallDuration) : 0.0f;
    const float tankBlastT = tankDestroyed ? clamp01(g_battleTimer / kTankBlastDuration) : 0.0f;

    // Night sky gradient
    glBegin(GL_QUADS);
    glColor3f(0.02f, 0.05f, 0.18f);
    glVertex2i(0, 600);
    glVertex2i(1000, 600);
    glColor3f(0.04f, 0.08f, 0.28f);
    glVertex2i(1000, 340);
    glVertex2i(0, 340);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.04f, 0.08f, 0.28f);
    glVertex2i(0, 340);
    glVertex2i(1000, 340);
    glColor3f(0.05f, 0.10f, 0.33f);
    glVertex2i(1000, 170);
    glVertex2i(0, 170);
    glEnd();

    // Distant skyline layer (reduced count)
    for (int i = 0; i < 5; ++i) {
        int x = 30 + i * 190;
        int w = 120;
        int h = 144 + (i % 3) * 30;
        float t = static_cast<float>(i % 3) * 0.01f;
        glColor3f(0.07f + t, 0.13f + t, 0.30f + t);
        glBegin(GL_QUADS);
        glVertex2i(x, 120);
        glVertex2i(x + w, 120);
        glVertex2i(x + w, 120 + h);
        glVertex2i(x, 120 + h);
        glEnd();

        for (int wx = x + 8; wx <= x + w - 12; wx += 13) {
            for (int wy = 132; wy <= 120 + h - 12; wy += 18) {
                bool lit = ((wx / 13 + wy / 11 + i) % 3) != 0;
                if (lit) {
                    glColor3f(0.56f, 0.72f, 0.92f);
                } else {
                    glColor3f(0.17f, 0.27f, 0.46f);
                }
                glBegin(GL_QUADS);
                glVertex2i(wx, wy);
                glVertex2i(wx + 6, wy);
                glVertex2i(wx + 6, wy + 8);
                glVertex2i(wx, wy + 8);
                glEnd();
            }
        }
    }

    // Near skyline layer (reduced count)
    for (int i = 0; i < 4; ++i) {
        int x = 70 + i * 230;
        int w = 150;
        int h = 198 + (i % 3) * 44;
        float s = static_cast<float>(i % 2) * 0.015f;
        glColor3f(0.11f + s, 0.21f + s, 0.36f + s);
        glBegin(GL_QUADS);
        glVertex2i(x, 120);
        glVertex2i(x + w, 120);
        glVertex2i(x + w, 120 + h);
        glVertex2i(x, 120 + h);
        glEnd();

        for (int wx = x + 10; wx <= x + w - 12; wx += 15) {
            for (int wy = 132; wy <= 120 + h - 14; wy += 20) {
                bool lit = ((wx / 8 + wy / 10 + i) % 4) != 0;
                if (lit) {
                    glColor3f(0.68f, 0.82f, 0.96f);
                } else {
                    glColor3f(0.20f, 0.30f, 0.50f);
                }
                glBegin(GL_QUADS);
                glVertex2i(wx, wy);
                glVertex2i(wx + 8, wy);
                glVertex2i(wx + 8, wy + 11);
                glVertex2i(wx, wy + 11);
                glEnd();
            }
        }
    }

    // Road and lane
    glColor3f(0.16f, 0.19f, 0.28f);
    glBegin(GL_POLYGON);
    glVertex2i(0, 0);
    glVertex2i(1000, 0);
    glVertex2i(1000, 120);
    glVertex2i(0, 120);
    glEnd();

    glColor3f(0.24f, 0.28f, 0.40f);
    glBegin(GL_POLYGON);
    glVertex2i(0, 120);
    glVertex2i(1000, 120);
    glVertex2i(1000, 134);
    glVertex2i(0, 134);
    glEnd();

    glColor3f(0.90f, 0.92f, 0.95f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    for (int x = 20; x < 1000; x += 88) {
        glVertex2i(x, 64);
        glVertex2i(x + 42, 64);
    }
    glEnd();

    drawLaunchButton();

    glPushMatrix();
    glTranslatef(tankOffsetX, tankOffsetY, 0.0f);

    // Tank track and chassis silhouette
    glColor3f(0.11f, 0.23f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex2i(32, 52);
    glVertex2i(48, 40);
    glVertex2i(410, 40);
    glVertex2i(434, 58);
    glVertex2i(438, 92);
    glVertex2i(418, 116);
    glVertex2i(62, 116);
    glVertex2i(36, 96);
    glEnd();

    glColor3f(0.18f, 0.35f, 0.23f);
    glBegin(GL_POLYGON);
    glVertex2i(46, 56);
    glVertex2i(60, 48);
    glVertex2i(392, 48);
    glVertex2i(420, 64);
    glVertex2i(422, 88);
    glVertex2i(404, 108);
    glVertex2i(74, 108);
    glVertex2i(50, 92);
    glEnd();

    // Track chain nodes
    glColor3f(0.34f, 0.57f, 0.37f);
    glPointSize(2.2f);
    glBegin(GL_POINTS);
    for (int tx = 56; tx <= 404; tx += 8) {
        for (int r = 0; r <= 2; ++r) {
            for (int a = 0; a <= 360; a += 18) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(static_cast<float>(tx) + static_cast<float>(r) * std::cos(rad),
                           48.0f + static_cast<float>(r) * std::sin(rad));
                glVertex2f(static_cast<float>(tx) + static_cast<float>(r) * std::cos(rad),
                           108.0f + static_cast<float>(r) * std::sin(rad));
            }
        }
    }
    for (int ty = 56; ty <= 100; ty += 8) {
        for (int r = 0; r <= 2; ++r) {
            for (int a = 0; a <= 360; a += 20) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(44.0f + static_cast<float>(r) * std::cos(rad),
                           static_cast<float>(ty) + static_cast<float>(r) * std::sin(rad));
                glVertex2f(420.0f + static_cast<float>(r) * std::cos(rad),
                           static_cast<float>(ty) + static_cast<float>(r) * std::sin(rad));
            }
        }
    }
    glEnd();

    // Tank wheels
    int wheelStart = 78;
    int wheelGap = 42;
    int wheelY = 76;
    glPointSize(2.0f);
    for (int i = 0; i < 8; ++i) {
        int cx = wheelStart + i * wheelGap;

        glColor3f(0.06f, 0.07f, 0.06f);
        glBegin(GL_POINTS);
        for (int r = 0; r <= 20; ++r) {
            for (int a = 0; a <= 360; a += 2) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(static_cast<float>(cx) + static_cast<float>(r) * std::cos(rad),
                           static_cast<float>(wheelY) + static_cast<float>(r) * std::sin(rad));
            }
        }
        glEnd();

        glColor3f(0.24f, 0.28f, 0.24f);
        glBegin(GL_POINTS);
        for (int r = 0; r <= 8; ++r) {
            for (int a = 0; a <= 360; a += 3) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(static_cast<float>(cx) + static_cast<float>(r) * std::cos(rad),
                           static_cast<float>(wheelY) + static_cast<float>(r) * std::sin(rad));
            }
        }
        glEnd();

        glColor3f(0.48f, 0.57f, 0.50f);
        glBegin(GL_POINTS);
        for (int a = 0; a <= 360; a += 3) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(static_cast<float>(cx) + 20.0f * std::cos(rad), static_cast<float>(wheelY) + 20.0f * std::sin(rad));
        }
        glEnd();

        drawWheelSpokes(static_cast<float>(cx), static_cast<float>(wheelY), 14.0f,
                        wheelSpinDeg + static_cast<float>(i) * 24.0f);
    }

    // Tank lower and middle body
    glColor3f(0.22f, 0.39f, 0.24f);
    glBegin(GL_POLYGON);
    glVertex2i(30, 116);
    glVertex2i(396, 116);
    glVertex2i(430, 136);
    glVertex2i(432, 186);
    glVertex2i(28, 186);
    glVertex2i(28, 136);
    glEnd();

    glColor3f(0.18f, 0.31f, 0.19f);
    glBegin(GL_POLYGON);
    glVertex2i(84, 186);
    glVertex2i(342, 186);
    glVertex2i(374, 212);
    glVertex2i(106, 212);
    glEnd();

    glColor3f(0.15f, 0.28f, 0.17f);
    glBegin(GL_POLYGON);
    glVertex2i(132, 212);
    glVertex2i(322, 212);
    glVertex2i(322, 238);
    glVertex2i(132, 238);
    glEnd();

    // Body detail panels
    glColor3f(0.14f, 0.26f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex2i(68, 124);
    glVertex2i(164, 124);
    glVertex2i(164, 146);
    glVertex2i(68, 146);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(184, 124);
    glVertex2i(280, 124);
    glVertex2i(280, 146);
    glVertex2i(184, 146);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(300, 124);
    glVertex2i(396, 124);
    glVertex2i(396, 146);
    glVertex2i(300, 146);
    glEnd();

    glColor3f(0.52f, 0.63f, 0.55f);
    glPointSize(2.6f);
    glBegin(GL_POINTS);
    for (int rx = 46; rx <= 418; rx += 28) {
        glVertex2i(rx, 121);
        glVertex2i(rx, 181);
    }
    for (int ry = 132; ry <= 172; ry += 20) {
        glVertex2i(36, ry);
        glVertex2i(424, ry);
    }
    glEnd();

    glColor3f(0.34f, 0.58f, 0.36f);
    glLineWidth(1.8f);
    glBegin(GL_LINES);
    for (int gx = 164; gx <= 298; gx += 19) {
        glVertex2i(gx, 216);
        glVertex2i(gx, 235);
    }
    for (int gy = 221; gy <= 232; gy += 6) {
        glVertex2i(152, gy);
        glVertex2i(310, gy);
    }
    glEnd();

    // Launcher top platform and pedestal housings
    glColor3f(0.19f, 0.38f, 0.21f);
    glBegin(GL_POLYGON);
    glVertex2i(112, 206);
    glVertex2i(332, 206);
    glVertex2i(356, 236);
    glVertex2i(136, 236);
    glEnd();

    glColor3f(0.17f, 0.33f, 0.18f);
    glBegin(GL_POLYGON);
    glVertex2i(128, 194);
    glVertex2i(172, 194);
    glVertex2i(172, 236);
    glVertex2i(128, 236);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(220, 194);
    glVertex2i(264, 194);
    glVertex2i(264, 236);
    glVertex2i(220, 236);
    glEnd();

    // Gearbox ring between launchers
    glPointSize(2.0f);
    glColor3f(0.47f, 0.60f, 0.46f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 17; ++r) {
        for (int a = 0; a <= 360; a += 5) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(206.0f + static_cast<float>(r) * std::cos(rad), 222.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    glColor3f(0.13f, 0.21f, 0.15f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 9; ++r) {
        for (int a = 0; a <= 360; a += 5) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(206.0f + static_cast<float>(r) * std::cos(rad), 222.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    glColor3f(0.60f, 0.72f, 0.58f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2i(189, 222);
    glVertex2i(223, 222);
    glVertex2i(206, 205);
    glVertex2i(206, 239);
    glVertex2i(194, 210);
    glVertex2i(218, 234);
    glVertex2i(218, 210);
    glVertex2i(194, 234);
    glEnd();

    // Two parabolic launcher dishes
    int dishCenterX[] = {154, 244};
    for (int d = 0; d < 2; ++d) {
        int cx = dishCenterX[d];

        glColor3f(0.54f, 0.64f, 0.42f);
        glBegin(GL_POLYGON);
        glVertex2i(cx, 236);
        for (int a = 20; a <= 150; a += 2) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(static_cast<float>(cx) + 76.0f * std::cos(rad) + 14.0f * std::sin(rad),
                       236.0f + 56.0f * std::sin(rad));
        }
        glEnd();

        glColor3f(0.46f, 0.56f, 0.36f);
        glBegin(GL_POLYGON);
        glVertex2i(cx + 1, 236);
        for (int a = 26; a <= 146; a += 2) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(static_cast<float>(cx) + 60.0f * std::cos(rad) + 10.0f * std::sin(rad),
                       236.0f + 44.0f * std::sin(rad));
        }
        glEnd();

        glColor3f(0.72f, 0.80f, 0.58f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_STRIP);
        for (int a = 20; a <= 150; a += 2) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(static_cast<float>(cx) + 76.0f * std::cos(rad) + 14.0f * std::sin(rad),
                       236.0f + 56.0f * std::sin(rad));
        }
        glEnd();

        // Dish truss frame
        glColor3f(0.32f, 0.55f, 0.32f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex2i(cx - 34, 216);
        glVertex2i(cx + 2, 242);
        glVertex2i(cx + 34, 216);
        glVertex2i(cx + 2, 242);
        glVertex2i(cx - 34, 216);
        glVertex2i(cx + 34, 216);
        glVertex2i(cx + 2, 216);
        glVertex2i(cx + 2, 242);
        glEnd();

        // Feed horn inside dish
        glColor3f(0.05f, 0.06f, 0.07f);
        glBegin(GL_POINTS);
        for (int r = 0; r <= 4; ++r) {
            for (int a = 0; a <= 360; a += 8) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(static_cast<float>(cx) + 10.0f + static_cast<float>(r) * std::cos(rad),
                           238.0f + static_cast<float>(r) * std::sin(rad));
            }
        }
        glEnd();

        glColor3f(0.90f, 0.92f, 0.93f);
        glBegin(GL_POINTS);
        for (int r = 0; r <= 2; ++r) {
            for (int a = 0; a <= 360; a += 12) {
                float rad = static_cast<float>(a) * PI / 180.0f;
                glVertex2f(static_cast<float>(cx) + 13.0f + static_cast<float>(r) * std::cos(rad),
                           240.0f + static_cast<float>(r) * std::sin(rad));
            }
        }
        glEnd();
    }

    // Structural braces and hydraulic arms
    glColor3f(0.30f, 0.50f, 0.30f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2i(118, 206);
    glVertex2i(154, 236);
    glVertex2i(182, 206);
    glVertex2i(154, 236);
    glVertex2i(206, 206);
    glVertex2i(244, 236);
    glVertex2i(272, 206);
    glVertex2i(244, 236);

    glVertex2i(148, 212);
    glVertex2i(214, 270);
    glVertex2i(226, 212);
    glVertex2i(292, 280);
    glVertex2i(176, 212);
    glVertex2i(238, 262);
    glVertex2i(252, 212);
    glVertex2i(322, 292);
    glEnd();

    glColor3f(0.76f, 0.80f, 0.76f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2i(156, 220);
    glVertex2i(212, 266);
    glVertex2i(234, 220);
    glVertex2i(286, 274);
    glEnd();

    // Twin launcher rails behind dishes
    glColor3f(0.17f, 0.30f, 0.18f);
    glBegin(GL_POLYGON);
    glVertex2i(188, 236);
    glVertex2i(203, 227);
    glVertex2i(360, 330);
    glVertex2i(345, 339);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2i(196, 248);
    glVertex2i(211, 239);
    glVertex2i(368, 342);
    glVertex2i(353, 351);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(258, 236);
    glVertex2i(273, 227);
    glVertex2i(440, 336);
    glVertex2i(425, 345);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2i(266, 248);
    glVertex2i(281, 239);
    glVertex2i(448, 348);
    glVertex2i(433, 357);
    glEnd();

    glColor3f(0.27f, 0.48f, 0.29f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2i(203, 227);
    glVertex2i(360, 330);
    glVertex2i(211, 239);
    glVertex2i(368, 342);
    glVertex2i(273, 227);
    glVertex2i(440, 336);
    glVertex2i(281, 239);
    glVertex2i(448, 348);
    glEnd();

    // Missiles mounted on launcher rails (button-triggered launch)
    float launchProgress = 0.0f;
    if (intercepting) {
        launchProgress = interceptT;
    } else if (planeDestroyed) {
        launchProgress = 1.0f;
    }

    float shiftA = clamp01((launchProgress - 0.00f) / 0.55f) * 44.0f;
    float shiftB = clamp01((launchProgress - 0.22f) / 0.55f) * 44.0f;
    float shiftC = clamp01((launchProgress - 0.44f) / 0.56f) * 44.0f;

    drawFriendlyMissile(305.0f + shiftA, 291.0f + shiftA * 0.62f);
    drawFriendlyMissile(339.0f + shiftB, 311.0f + shiftB * 0.62f);
    drawFriendlyMissile(371.0f + shiftC, 293.0f + shiftC * 0.62f);

    if (launchProgress > 0.02f) {
        glColor3f(1.0f, 0.66f, 0.20f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(300.0f + shiftA, 289.0f + shiftA * 0.62f);
        glVertex2f(286.0f + shiftA, 282.0f + shiftA * 0.62f);

        glVertex2f(334.0f + shiftB, 309.0f + shiftB * 0.62f);
        glVertex2f(320.0f + shiftB, 302.0f + shiftB * 0.62f);

        glVertex2f(366.0f + shiftC, 291.0f + shiftC * 0.62f);
        glVertex2f(352.0f + shiftC, 284.0f + shiftC * 0.62f);
        glEnd();
    }

    glPopMatrix();

    // Enemy missile paths (dashed)
    glColor3f(0.95f, 0.95f, 0.97f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 14; i += 2) {
        float t0 = static_cast<float>(i) / 14.0f;
        float t1 = static_cast<float>(i + 1) / 14.0f;
        glVertex2f(955.0f + (705.0f - 955.0f) * t0, 510.0f + (392.0f - 510.0f) * t0);
        glVertex2f(955.0f + (705.0f - 955.0f) * t1, 510.0f + (392.0f - 510.0f) * t1);
    }
    for (int i = 0; i < 14; i += 2) {
        float t0 = static_cast<float>(i) / 14.0f;
        float t1 = static_cast<float>(i + 1) / 14.0f;
        glVertex2f(875.0f + (646.0f - 875.0f) * t0, 436.0f + (320.0f - 436.0f) * t0);
        glVertex2f(875.0f + (646.0f - 875.0f) * t1, 436.0f + (320.0f - 436.0f) * t1);
    }
    glEnd();

    // Enemy missiles (animated)
    float enemyPhase0 = std::fmod(g_animationTime * 0.24f, 1.0f);
    float enemyPhase1 = std::fmod(enemyPhase0 + 0.52f, 1.0f);

    float enemy0X = 955.0f + (705.0f - 955.0f) * enemyPhase0;
    float enemy0Y = 510.0f + (392.0f - 510.0f) * enemyPhase0;
    float enemy1X = 875.0f + (646.0f - 875.0f) * enemyPhase1;
    float enemy1Y = 436.0f + (320.0f - 436.0f) * enemyPhase1;

    float enemyAngle0 = std::atan2(392.0f - 510.0f, 705.0f - 955.0f) * 180.0f / PI;
    float enemyAngle1 = std::atan2(320.0f - 436.0f, 646.0f - 875.0f) * 180.0f / PI;

    float flamePulse = 0.85f + 0.25f * std::sin(g_animationTime * 16.0f);
    drawEnemyMissile(enemy0X, enemy0Y, enemyAngle0, flamePulse);
    drawEnemyMissile(enemy1X, enemy1Y, enemyAngle1, 1.10f - (flamePulse - 0.85f));

    // Intercept and destruction effects
    int burstR[] = {45, 26, 40, 20, 36, 24, 44, 22, 38, 18, 42, 24, 34, 20, 46, 25};
    float burstStrength = 0.0f;
    if (intercepting) {
        burstStrength = clamp01((interceptT - 0.76f) / 0.24f);
    } else if (planeDestroyed) {
        burstStrength = 1.0f - 0.7f * planeFallT;
    }

    if (burstStrength > 0.02f) {
        float ex = hitX;
        float ey = hitY;
        float burstPulse = burstStrength * (0.84f + 0.26f * std::sin(g_animationTime * 10.0f));
        for (int i = 0; i < 16; ++i) {
            float mid = static_cast<float>(i) * 22.5f * PI / 180.0f;
            float left = (static_cast<float>(i) * 22.5f - 8.0f) * PI / 180.0f;
            float right = (static_cast<float>(i) * 22.5f + 8.0f) * PI / 180.0f;
            float outer = static_cast<float>(burstR[i]) * burstPulse;

            if ((i % 2) == 0) {
                glColor3f(1.0f, 0.88f, 0.20f);
            } else {
                glColor3f(0.98f, 0.50f, 0.08f);
            }

            glBegin(GL_TRIANGLES);
            glVertex2f(ex, ey);
            glVertex2f(ex + 10.0f * std::cos(left), ey + 10.0f * std::sin(left));
            glVertex2f(ex + outer * std::cos(mid), ey + outer * std::sin(mid));
            glEnd();

            glBegin(GL_TRIANGLES);
            glVertex2f(ex, ey);
            glVertex2f(ex + outer * std::cos(mid), ey + outer * std::sin(mid));
            glVertex2f(ex + 10.0f * std::cos(right), ey + 10.0f * std::sin(right));
            glEnd();
        }
    }

    // Fighter planes and button-driven interception
    float planeRoll = -8.0f + 1.8f * std::sin(g_animationTime * 2.0f);
    drawFighterPlane(planeLeadX, planeLeadY, 1.0f, planeRoll, false);

    if (waitingLaunch) {
        drawFighterPlane(targetPreHitX, targetPreHitY, 0.92f, planeRoll - 2.5f, false);
    } else if (intercepting) {
        float targetX = targetPreHitX + (hitX - targetPreHitX) * interceptT;
        float targetY = targetPreHitY + (hitY - targetPreHitY) * interceptT;
        drawFighterPlane(targetX, targetY, 0.92f, planeRoll - 2.5f, false);

        float interceptorX = tankLaunchX + (hitX - tankLaunchX) * interceptT;
        float interceptorY = tankLaunchY + (hitY - tankLaunchY) * interceptT;

        glColor3f(0.95f, 0.95f, 0.97f);
        glLineWidth(2.2f);
        glBegin(GL_LINES);
        for (int i = 0; i < 10; i += 2) {
            float t0 = (static_cast<float>(i) / 10.0f) * interceptT;
            float t1 = (static_cast<float>(i + 1) / 10.0f) * interceptT;
            glVertex2f(tankLaunchX + (hitX - tankLaunchX) * t0, tankLaunchY + (hitY - tankLaunchY) * t0);
            glVertex2f(tankLaunchX + (hitX - tankLaunchX) * t1, tankLaunchY + (hitY - tankLaunchY) * t1);
        }
        glEnd();

        drawFriendlyMissile(interceptorX - 38.0f, interceptorY - 18.0f);
    } else if (planeDestroyed) {
        float damagedX = hitX - 138.0f * planeFallT;
        float damagedY = hitY - 124.0f * planeFallT;
        drawFighterPlane(damagedX, damagedY, 0.92f, planeRoll - 2.5f - 18.0f * planeFallT, true);

        glColor3f(0.82f, 0.84f, 0.88f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (int i = 0; i < 12; i += 2) {
            float t0 = static_cast<float>(i) / 12.0f;
            float t1 = static_cast<float>(i + 1) / 12.0f;
            glVertex2f(damagedX - 30.0f - t0 * 130.0f, damagedY + 8.0f + t0 * 26.0f);
            glVertex2f(damagedX - 30.0f - t1 * 130.0f, damagedY + 8.0f + t1 * 26.0f);
        }
        glEnd();
    } else {
        // Launch missed: target plane survives and escapes while tank is destroyed.
        float escapeX = targetPreHitX - 110.0f * tankBlastT;
        float escapeY = targetPreHitY + 24.0f * tankBlastT;
        drawFighterPlane(escapeX, escapeY, 0.92f, planeRoll - 2.5f, false);
    }

    if (tankDestroyed) {
        float tx = 236.0f + tankOffsetX;
        float ty = 184.0f + tankOffsetY;
        float tankPulse = (1.0f - 0.35f * tankBlastT) * (0.86f + 0.32f * std::sin(g_animationTime * 9.0f));
        for (int i = 0; i < 14; ++i) {
            float mid = static_cast<float>(i) * (360.0f / 14.0f) * PI / 180.0f;
            float left = (static_cast<float>(i) * (360.0f / 14.0f) - 9.0f) * PI / 180.0f;
            float right = (static_cast<float>(i) * (360.0f / 14.0f) + 9.0f) * PI / 180.0f;
            float outer = (30.0f + static_cast<float>(i % 3) * 12.0f) * tankPulse;

            glColor3f((i % 2) == 0 ? 1.0f : 0.98f, (i % 2) == 0 ? 0.82f : 0.45f, 0.14f);
            glBegin(GL_TRIANGLES);
            glVertex2f(tx, ty);
            glVertex2f(tx + 9.0f * std::cos(left), ty + 9.0f * std::sin(left));
            glVertex2f(tx + outer * std::cos(mid), ty + outer * std::sin(mid));
            glEnd();

            glBegin(GL_TRIANGLES);
            glVertex2f(tx, ty);
            glVertex2f(tx + outer * std::cos(mid), ty + outer * std::sin(mid));
            glVertex2f(tx + 9.0f * std::cos(right), ty + 9.0f * std::sin(right));
            glEnd();
        }
    }

    // Debris near explosion
    glColor3f(0.90f, 0.92f, 0.95f);
    glBegin(GL_TRIANGLES);
    glVertex2i(892, 498);
    glVertex2i(900, 504);
    glVertex2i(904, 495);

    glVertex2i(875, 468);
    glVertex2i(882, 474);
    glVertex2i(886, 466);

    glVertex2i(905, 472);
    glVertex2i(914, 476);
    glVertex2i(916, 467);
    glEnd();

    // Red car
    glPushMatrix();
    glTranslatef(redCarOffsetX, 0.0f, 0.0f);

    glColor3f(0.64f, 0.16f, 0.33f);
    glBegin(GL_POLYGON);
    glVertex2i(430, 58);
    glVertex2i(540, 58);
    glVertex2i(540, 80);
    glVertex2i(430, 80);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(454, 80);
    glVertex2i(516, 80);
    glVertex2i(500, 96);
    glVertex2i(466, 96);
    glEnd();

    glColor3f(0.12f, 0.13f, 0.14f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 12; ++r) {
        for (int a = 0; a <= 360; a += 4) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(456.0f + static_cast<float>(r) * std::cos(rad), 58.0f + static_cast<float>(r) * std::sin(rad));
            glVertex2f(512.0f + static_cast<float>(r) * std::cos(rad), 58.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    glColor3f(0.66f, 0.68f, 0.72f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 5; ++r) {
        for (int a = 0; a <= 360; a += 6) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(456.0f + static_cast<float>(r) * std::cos(rad), 58.0f + static_cast<float>(r) * std::sin(rad));
            glVertex2f(512.0f + static_cast<float>(r) * std::cos(rad), 58.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    drawWheelSpokes(456.0f, 58.0f, 9.0f, wheelSpinDeg * 1.15f);
    drawWheelSpokes(512.0f, 58.0f, 9.0f, wheelSpinDeg * 1.15f);

    glPopMatrix();

    // Blue car (right side)
    glPushMatrix();
    glTranslatef(blueCarOffsetX, 0.0f, 0.0f);

    glColor3f(0.18f, 0.36f, 0.70f);
    glBegin(GL_POLYGON);
    glVertex2i(820, 60);
    glVertex2i(920, 60);
    glVertex2i(920, 84);
    glVertex2i(820, 84);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2i(844, 84);
    glVertex2i(898, 84);
    glVertex2i(886, 98);
    glVertex2i(854, 98);
    glEnd();

    glColor3f(0.12f, 0.13f, 0.14f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 12; ++r) {
        for (int a = 0; a <= 360; a += 4) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(846.0f + static_cast<float>(r) * std::cos(rad), 60.0f + static_cast<float>(r) * std::sin(rad));
            glVertex2f(896.0f + static_cast<float>(r) * std::cos(rad), 60.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    drawWheelSpokes(846.0f, 60.0f, 9.0f, -wheelSpinDeg * 1.20f);
    drawWheelSpokes(896.0f, 60.0f, 9.0f, -wheelSpinDeg * 1.20f);

    glPopMatrix();

    glColor3f(0.66f, 0.68f, 0.72f);
    glBegin(GL_POINTS);
    for (int r = 0; r <= 5; ++r) {
        for (int a = 0; a <= 360; a += 6) {
            float rad = static_cast<float>(a) * PI / 180.0f;
            glVertex2f(846.0f + static_cast<float>(r) * std::cos(rad), 60.0f + static_cast<float>(r) * std::sin(rad));
            glVertex2f(896.0f + static_cast<float>(r) * std::cos(rad), 60.0f + static_cast<float>(r) * std::sin(rad));
        }
    }
    glEnd();

    glFlush();
    if (g_hdc != nullptr) {
        SwapBuffers(g_hdc);
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE: {
            int mouseX = static_cast<int>(LOWORD(lParam));
            int mouseY = static_cast<int>(HIWORD(lParam));
            screenToWorld(mouseX, mouseY, g_mouseWorldX, g_mouseWorldY);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int mouseX = static_cast<int>(LOWORD(lParam));
            int mouseY = static_cast<int>(HIWORD(lParam));
            float worldX = 0.0f;
            float worldY = 0.0f;
            screenToWorld(mouseX, mouseY, worldX, worldY);
            g_mouseWorldX = worldX;
            g_mouseWorldY = worldY;

            if (pointInRect(worldX, worldY,
                            kLaunchButtonLeft, kLaunchButtonBottom,
                            kLaunchButtonRight, kLaunchButtonTop)) {
                triggerLaunch();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_TIMER:
            if (wParam == kAnimationTimerId) {
                updateAnimationClock();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_SIZE: {
            int width = static_cast<int>(LOWORD(lParam));
            int height = static_cast<int>(HIWORD(lParam));
            if (width == 0) {
                width = 1;
            }
            if (height == 0) {
                height = 1;
            }
            g_viewportWidth = width;
            g_viewportHeight = height;

            if (g_hdc != nullptr) {
                glViewport(0, 0, width, height);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (g_hdc != nullptr) {
                draw();
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, kAnimationTimerId);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t kWindowClassName[] = L"Tank2DWindowClass";

    WNDCLASSW wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClassName;

    if (RegisterClassW(&wc) == 0) {
        return -1;
    }

    RECT windowRect = {0, 0, 1000, 600};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"2D Missile Defense Scene - Win32 OpenGL",
        WS_OVERLAPPEDWINDOW,
        120,
        40,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (hwnd == nullptr) {
        return -1;
    }

    g_hdc = GetDC(hwnd);
    if (g_hdc == nullptr) {
        DestroyWindow(hwnd);
        return -1;
    }

    if (!setupPixelFormat(g_hdc)) {
        ReleaseDC(hwnd, g_hdc);
        g_hdc = nullptr;
        DestroyWindow(hwnd);
        return -1;
    }

    g_glContext = wglCreateContext(g_hdc);
    if (g_glContext == nullptr) {
        ReleaseDC(hwnd, g_hdc);
        g_hdc = nullptr;
        DestroyWindow(hwnd);
        return -1;
    }

    if (wglMakeCurrent(g_hdc, g_glContext) == FALSE) {
        wglDeleteContext(g_glContext);
        g_glContext = nullptr;
        ReleaseDC(hwnd, g_hdc);
        g_hdc = nullptr;
        DestroyWindow(hwnd);
        return -1;
    }

    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    g_viewportWidth = clientRect.right - clientRect.left;
    g_viewportHeight = clientRect.bottom - clientRect.top;
    if (g_viewportWidth == 0) {
        g_viewportWidth = 1;
    }
    if (g_viewportHeight == 0) {
        g_viewportHeight = 1;
    }
    glViewport(0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    initAnimationClock();

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetTimer(hwnd, kAnimationTimerId, kAnimationIntervalMs, nullptr);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    wglMakeCurrent(nullptr, nullptr);
    if (g_glContext != nullptr) {
        wglDeleteContext(g_glContext);
        g_glContext = nullptr;
    }
    if (g_hdc != nullptr) {
        ReleaseDC(hwnd, g_hdc);
        g_hdc = nullptr;
    }

    return static_cast<int>(msg.wParam);
}
