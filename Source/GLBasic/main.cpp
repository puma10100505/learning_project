#include <cstdio>
#include <iostream>
#include <functional>

#include "learning.h"
#include "CommonDefines.h"

int main() {
    // init window
    if (gl_init_window() < 0) {
        std::cout << "init window failed\n";
        return -1;
    }

    // create a new window
    if (gl_create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "sample window", true) < 0) {
        std::cout << "create window failed\n"; 
        return -2;
    }

    // 1. prepare content to be rendered
    GLCubic q;
    GLCamera c(glm::vec3(0.0f, 6.0f, 0.0f));

    // 2. prepare textures
    GLTexture tex(solution_base_path + "/resources/container.jpg", GL_TEXTURE0);
    GLTexture tex2(solution_base_path + "/resources/awesomeface.png", GL_TEXTURE1);

    // 3. prepare shader
    Shader shader((solution_base_path + "assets/shaders/main.shader.vs").c_str(), 
        (solution_base_path + "assets/shaders/main.shader.fs").c_str());

    shader.use();
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);
    
    glm::vec4 color(.5f, .5f, .5f, .8f);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    const char* glsl_version = "#version 130";
    ImGui_ImplGlfw_InitForOpenGL(GetGlobalWindow(), true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // main loop
    gl_window_loop([&](){
         // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        tex.Active();
        tex2.Active();
        shader.use();

        // create transformation
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 scale = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        
        model = glm::rotate(model, (float)glfwGetTime() * glm::radians(-55.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        scale = glm::scale(scale, glm::vec3(1.5f));
        view = c.GetViewMatrix(); //glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
        projection = glm::perspective(glm::radians(45.0f), WINDOW_RATIO, 0.1f, 100.0f);

        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("scale", scale);
        shader.setMat4("projection", projection);
        //

        q.Draw();

        // Rendering
        ImGui::Render();
        ImGui::EndFrame();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }, [&](){}); 

   q.Destroy();

   // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // release resources
    gl_destroy_window();

    return 0;
}