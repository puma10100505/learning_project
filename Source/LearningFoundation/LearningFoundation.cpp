#include "LearningFoundation.h"




// // @Old Version
// void on_glfw_error_default(int code, const char* desc) 
// {
//     // TODO:
// }

// void on_frame_buffer_size_changed_default(GLFWwindow* window, int width, int height) {
//     // TODO:
//     glViewport(0, 0, width, height);
// }

// void on_key_event_default(GLFWwindow* window, int key, int scancode, int action, int mods) {
//     // TODO:
//     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
//         glfwSetWindowShouldClose(GetGlobalWindow(), true);
//     }
// }

// int gl_init_window() {
//     if (!glfwInit()) {
//         return -1;
//     }

//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//     return 0;
// }

// int gl_init_gui() {
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGuiIO& io = ImGui::GetIO(); (void)io;

//     // Setup Dear ImGui style
//     ImGui::StyleColorsDark();
//     // ImGui::StyleColorsLight();
//     // ImGui::StyleColorsClassic();
//     // io.Fonts->AddFontFromFileTTF("../resources/fonts/Roboto-Medium.ttf", 18);

//     // Setup Platform/Renderer bindings
//     ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
//     ImGui_ImplOpenGL3_Init("#version 330");

//     return 0;
// }

// void gl_on_gui_default() {
    
// }

// // create glfw window
// int gl_create_window(int32_t width, int32_t height, 
//     const std::string& title, bool hide_cursor, int32_t frameInterval,
//     GLFWerrorfun err_handler, GLFWframebuffersizefun fbs_handler, GLFWkeyfun key_handler) {

// #ifdef __APPLE__
//     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
// #endif

//     __GlobalWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
//     if (!__GlobalWindow) {
//         glfwTerminate();
//         std::cout << "gl_create_window create window failed\n";
//         return -1;
//     }

//     glfwMakeContextCurrent(__GlobalWindow);
//     gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//     glfwSwapInterval(1);
    
//     // set some callback func
//     glfwSetErrorCallback(err_handler);
//     glfwSetFramebufferSizeCallback(GetGlobalWindow(), fbs_handler);
//     glfwSetKeyCallback(GetGlobalWindow(), key_handler);

//     // Enable depth test
//     glEnable(GL_DEPTH_TEST);

//     if (hide_cursor) {
//         // Diable the mouse cursor when startup
//         glfwSetInputMode(GetGlobalWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//     }

//     FPS = 1000 / frameInterval;

//     return 0;    
// }

// void gl_destroy_window() {
//     glfwTerminate();
// }

// void gl_destroy_gui() {
//     ImGui_ImplOpenGL3_Shutdown();
//     ImGui_ImplGlfw_Shutdown();
//     ImGui::DestroyContext();
// }

// int gl_window_loop(std::function<void (void)> on_update, std::function<void (void)> on_gui) {
    
//     while (!glfwWindowShouldClose(__GlobalWindow)) {
//         glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

//         float currentFrameTime = glfwGetTime();
//         DeltaTime = currentFrameTime - LastFrameTime;
//         LastFrameTime = currentFrameTime;

//             // Start the Dear ImGui frame
//         ImGui_ImplOpenGL3_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();

//         on_gui();

//         on_update();

//         // Rendering
//         ImGui::Render();

//         ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

//         glfwSwapBuffers(__GlobalWindow);
//         glfwPollEvents();

//         std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
//     }

//     return 0;
// }
