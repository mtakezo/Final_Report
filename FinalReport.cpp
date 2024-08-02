#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iterator>
#include <random>
#include <ctime>

// 定数
const float g = 9.81f;
const float scale = 2.0f;
const float M_PI = 3.14;

// 初期設定
float v0 = 4.0f;
float angle = 60.0f;

// 目標のブロックの位置とサイズ
float blockX;
float blockY = 0.45f;
float blockWidth = 0.10f;
float blockHeight = 0.1f;

// 斜方投射運動の位置を計算するクラス
class Projectile {
public:
    Projectile(float v0, float angle)
        : v0(v0), angle(angle) {}

    void calculatePosition(float time, float& x, float& y) const {
        float angleRad = angle * M_PI / 180.0f;
        x = 0.5f + v0 * cos(angleRad) * time * scale;
        y = 0.5f + v0 * sin(angleRad) * time * scale - 0.5f * g * time * time * scale;
    }

    float getV0() const { return v0; }
    float getAngle() const { return angle; }
    void setV0(float newV0) { v0 = newV0; }
    void setAngle(float newAngle) { angle = newAngle; }

private:
    float v0;
    float angle;
};

// 最大距離と最大高さを計算する関数
void calculateMaxDandH(const Projectile& projectile, float& x_max, float& y_max) {
    float angleRad = projectile.getAngle() * M_PI / 180.0f;
    x_max = (projectile.getV0() * projectile.getV0() * sin(2 * angleRad)) / g;
    y_max = (projectile.getV0() * projectile.getV0() * sin(angleRad) * sin(angleRad)) / (2 * g);
}

struct Point {
    float x, y;
};

// 軌跡を管理するクラス
class Trajectory {
public:
    Trajectory(const Projectile& projectile, float dt, float t_max) {
        generatePoints(projectile, dt, t_max);
    }

    void generatePoints(const Projectile& projectile, float dt, float t_max) {
        points.clear();
        float t = 0.0f;
        Point p;
        while (t <= t_max) {
            projectile.calculatePosition(t, p.x, p.y);
            points.push_back(p);
            t += dt;
        }
    }

    const std::vector<Point>& getPoints() const { return points; }

private:
    std::vector<Point> points;
};

// 衝突判定関数
bool checkCollision(float x, float y) {
    return (x >= blockX && x <= blockX + blockWidth && y >= blockY && y <= blockY + blockHeight);
}
// 描画関数
void render(const Trajectory& trajectory, float t, bool hit, const Projectile& projectile) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // 目標のブロックを描画
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(blockX, blockY);
    glVertex2f(blockX + blockWidth, blockY);
    glVertex2f(blockX + blockWidth, blockY + blockHeight);
    glVertex2f(blockX, blockY + blockHeight);
    glEnd();

    // 軌跡を描画
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    for (auto it = trajectory.getPoints().begin(); it != trajectory.getPoints().end(); ++it) {
        float size = 0.05f;
        glVertex2f(it->x - size, it->y - size);
        glVertex2f(it->x + size, it->y - size);
        glVertex2f(it->x + size, it->y + size);
        glVertex2f(it->x - size, it->y + size);
    }
    glEnd();

    // 現在の位置を描画
    float x, y;
    if (!hit) {
        projectile.calculatePosition(t, x, y);
    }
    else {
        const auto& points = trajectory.getPoints();
        x = points.back().x;
        y = points.back().y;
    }
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x - 0.05f, y - 0.05f);
    glVertex2f(x + 0.05f, y - 0.05f);
    glVertex2f(x + 0.05f, y + 0.05f);
    glVertex2f(x - 0.05f, y + 0.05f);
    glEnd();
}

// キー入力のコールバック関数
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        Projectile* projectile = static_cast<Projectile*>(glfwGetWindowUserPointer(window));
        if (!projectile) return;

        switch (key) {
        case GLFW_KEY_W:
            projectile->setV0(projectile->getV0() + 0.1f);// 初速を増加
            break;
        case GLFW_KEY_S:
            projectile->setV0(projectile->getV0() - 0.1f);// 初速を減少
            break;
        case GLFW_KEY_UP:
            projectile->setAngle(projectile->getAngle() + 1.0f);// 発射角度を増加
            if (projectile->getAngle() > 90.0f) projectile->setAngle(90.0f);
            break;
        case GLFW_KEY_DOWN:
            projectile->setAngle(projectile->getAngle() - 1.0f);// 発射角度を減少
            if (projectile->getAngle() < 0.0f) projectile->setAngle(0.0f);
            break;
        }
    }
}

// ウィンドウリサイズのコールバック関数
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float ortho_x = 5.0f;
    float ortho_y = 5.0f;
    glOrtho(0.0, ortho_x, 0.0, ortho_y, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 5.0);
    blockX = dis(gen);

    Projectile projectile(v0, angle);

    float x_max, y_max;
    calculateMaxDandH(projectile, x_max, y_max);

    float ortho_x = x_max * 5.0f;
    float ortho_y = y_max * 5.0f;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    int width = 1200;
    int height = 900;
    GLFWwindow* window = glfwCreateWindow(width, height, "Projectile Motion Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glfwSetWindowUserPointer(window, &projectile);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, ortho_x, 0.0, ortho_y, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

    float dt = 0.05f;
    float t_max = (2 * projectile.getV0() * sin(projectile.getAngle() * M_PI / 180.0f)) / g;
    Trajectory trajectory(projectile, dt, t_max);

    float time = 0.0f;
    bool hit = false;
    bool gameCleared = false;
    bool messageDisplayed = false;
    float clearTime = 0.0f;
    auto startTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        render(trajectory, time, hit, projectile);
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (!hit) {
            time += dt;
            if (time > t_max) {
                time = 0.0f;
            }

            float x, y;
            projectile.calculatePosition(time, x, y);

            if (checkCollision(x, y)) {
                hit = true;
                gameCleared = true;
                clearTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
            }

            t_max = (5 * projectile.getV0() * sin(projectile.getAngle() * M_PI / 180.0f)) / g;
            trajectory.generatePoints(projectile, dt, t_max);
        }

        if (gameCleared && !messageDisplayed) {
            std::cout << "GAME CLEAR!!\n";
            std::cout << "Clear time: " << clearTime << " seconds\n";
            messageDisplayed = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(64));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
