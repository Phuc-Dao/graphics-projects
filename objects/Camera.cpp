#include "Camera.h"

Camera::Camera(const glm::mat4 &p, const glm::mat4 &t)
        : Node(t), projection(p) {
}

Camera::Camera(const glm::mat4 &p, std::shared_ptr<Transform> t)
        : Node(t), projection(p) {
}

void Camera::setup() {
    auto cam = glm::mat4(1.0f);
    for (Node *p = this; p != nullptr; p = p->parent()) {
        cam = p->transform->model * cam;
    }
    view = glm::inverse(cam);
    eye = glm::vec3(cam[3]);
}