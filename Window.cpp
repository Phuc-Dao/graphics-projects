#include <iostream>

#include "Window.h"
#include "Time.h"
#include "components/ConstraintAnimator.h"

Window::Window() {
    setupCallbacks();
    initializeProgram();
    initializeObjects();

    // Initial size will not fire callback
    // force to fire
    resizeCallback(width, height);
}

void Window::initializeProgram() {
    try {
        shaders[0] = std::make_shared<Shader>("shaders/normal_coloring.vert",
                                              "shaders/normal_coloring.frag");
        shaders[1] = std::make_shared<Shader>("shaders/reflection_map.vert",
                                              "shaders/reflection_map.frag");
    } catch (...) {
        std::cerr << "Failed to initialize shader program" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Window::initializeObjects() {
    glm::vec3 eye(0, 0, 20);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                            float(width) / float(height), 1.0f, 1000.0f);

    camCtl = std::make_shared<FreeFlying>(glm::translate(eye));
    cam = static_cast<Camera *>(scene.addChild(std::make_unique<Camera>(projection, camCtl)));

    skybox = std::make_unique<Skybox>();

    bezier = std::make_shared<BezierCurve>();
    for (int i = 0; i < 3 * 8; ++i) {
        auto theta = 2 * glm::pi<float>() / (3 * 8) * i;
        bezier->controlPoints.emplace_back(5.0f * glm::cos(theta),
                                           i % 2 ? 1.0f : -1.0f,
                                           5.0f * glm::sin(theta));
    }
    for (size_t i = 1; i <= bezier->controlPoints.size() / 3; ++i) {
        controls.emplace_back(&bezier->controlPoints[3 * i - 1],
                              &bezier->controlPoints[(3 * i) % bezier->controlPoints.size()],
                              &bezier->controlPoints[(3 * i + 1) % bezier->controlPoints.size()]);
    }
    bezier->upload();
    scene.addComponent(bezier);

    auto sphere = std::make_shared<Mesh>(Mesh::fromObjFile("meshes/bunny.obj"));
    sphere->useShader(shaders[0]);
    auto scaled_sphere = std::make_unique<Node>(glm::scale(glm::vec3(0.2f)));
    scaled_sphere->addComponent(sphere);

    auto animation = std::make_shared<ConstraintAnimator>(bezier);
    auto mover = std::make_unique<Node>(animation->control);
    mover->addChild(std::move(scaled_sphere));
    scene.addChild(std::move(mover));
    scene.addComponent(animation);
}

void Window::resizeCallback(int width, int height) {
#ifdef __APPLE__
    // In case your Mac has a retina display.
    glfwGetFramebufferSize(window, &width, &height);
#endif
    if (width && height) {
        this->width = width;
        this->height = height;
        // Set the viewport size.
        glViewport(0, 0, width, height);

        // Set the projection matrix.
        cam->projection = glm::perspective(glm::radians(60.0f),
                                           float(width) / float(height), 1.0f, 1000.0f);
    }
}

void Window::update() {
    scene.update();
}

void Window::draw() {
    if (shouldCull) {
        scene.cull(cam->projection * cam->view);
    }

    // Clear the color and depth buffers.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    skybox->draw(glm::mat4(1.0f), cam->projection, cam->view, cam->eye);

    for (int i = 0; i < controls.size(); ++i) {
        controls[i].draw(cam->projection, cam->view, cam->eye, 3 * i + 1);
    }

    // Use cube map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->cubeMap);

    // Render the object.
    scene.draw(glm::mat4(1.0f), cam->projection, cam->view, cam->eye);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Swap buffers.
    glfwSwapBuffers(window);
}

void Window::setupCallbacks() {
    // Set the key callback.
    glfwSetKeyCallback(window,
                       [](GLFWwindow *window, int key, int scancode, int action, int mods) {
                           retrieve(window)->keyCallback(key, scancode, action, mods);
                       });
    // Set the window resize callback.
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow *window, int width, int height) {
                                  retrieve(window)->resizeCallback(width, height);
                              });
    // Set the mouse button callback.
    glfwSetMouseButtonCallback(window,
                               [](GLFWwindow *window, int button, int action, int mods) {
                                   retrieve(window)->mouseButtonCallback(button, action, mods);
                               });
    // Set the cursor position callback.
    glfwSetCursorPosCallback(window,
                             [](GLFWwindow *window, double x, double y) {
                                 retrieve(window)->cursorPosCallback(x, y);
                             });

    // Set the scroll callback.
    glfwSetScrollCallback(window,
                          [](GLFWwindow *window, double xoffset, double yoffset) {
                              retrieve(window)->scrollCallback(xoffset, yoffset);
                          });
}

void Window::keyCallback(int key, int scancode, int action, int mods) {
    // Check for a key press.
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                // Close the window. This causes the program to also terminate.
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case GLFW_KEY_W:
                camCtl->forward = true;
                break;
            case GLFW_KEY_A:
                camCtl->left = true;
                break;
            case GLFW_KEY_S:
                camCtl->backward = true;
                break;
            case GLFW_KEY_D:
                camCtl->right = true;
                break;
            case GLFW_KEY_Q:
                camCtl->up = true;
                break;
            case GLFW_KEY_E:
                camCtl->down = true;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                camCtl->local = false;
                break;
            case GLFW_KEY_C:
                shouldCull = !shouldCull;
                std::cout << "shouldCull: " << shouldCull << std::endl;
                break;
            case GLFW_KEY_N:
                std::swap(*shaders[0], *shaders[1]);
                break;
            default:
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W:
                camCtl->forward = false;
                break;
            case GLFW_KEY_A:
                camCtl->left = false;
                break;
            case GLFW_KEY_S:
                camCtl->backward = false;
                break;
            case GLFW_KEY_D:
                camCtl->right = false;
                break;
            case GLFW_KEY_Q:
                camCtl->up = false;
                break;
            case GLFW_KEY_E:
                camCtl->down = false;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                camCtl->local = true;
                break;
            default:
                break;
        }
    }
}

void Window::mouseButtonCallback(int button, int action, int mods) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                camCtl->startRotate(FreeFlying::windowCoordToCamDir(x, y, width, height, cam->projection));
            } else if (action == GLFW_RELEASE) {
                camCtl->stopRotate();
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                glReadPixels(x, height - 1 - y, 1, 1, GL_STENCIL_INDEX, GL_INT, &selected);
            } else if (action == GLFW_RELEASE) {
                selected = 0;
            }
            break;
        default:
            break;
    }
}

void Window::cursorPosCallback(double x, double y) {
    camCtl->rotate(FreeFlying::windowCoordToCamDir(x, y, width, height, cam->projection));

    if (selected) {
        auto viewport = glm::vec4(0.0f, 0.0f, width, height);
        auto &c = controls[(selected - 1) / 3];
        auto i = (selected - 1) % 3;
        auto win_coord = glm::vec3(x, height - 1 - y,
                                   glm::project(c.get(i), cam->view, cam->projection, viewport).z);
        c.set(i, glm::unProject(win_coord, cam->view, cam->projection, viewport));
        bezier->upload();
    }
}

void Window::scrollCallback(double xoffset, double yoffset) {
}

void Window::loop() {
    // Enable depth buffering.
    glEnable(GL_DEPTH_TEST);
    // Related to shaders and z value comparisons for the depth buffer.
    glDepthFunc(GL_LEQUAL);
    // Enable face culling.
    glEnable(GL_CULL_FACE);
    // Set polygon drawing mode to fill front and back of each polygon.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // Set clear color to black.
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // Enable stencil test
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    Time::reset();
    while (!glfwWindowShouldClose(window)) {
        Time::tick();
        glfwPollEvents();
        update();
        draw();
    }
}
