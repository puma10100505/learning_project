#pragma once
# pragma warning (disable:4819)

#include <chrono>
#include <thread>
#include <cstdio>
#include <iostream>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"

#include "LearningShader.h"
#include "stb_image.h"

#include "utility.h"
#include "defines.h"
#include "GL/freeglut.h"

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 900;

