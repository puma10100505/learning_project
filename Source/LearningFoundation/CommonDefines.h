#pragma once

#include <string>

#define LearningOpenGL_VERSION_MAJOR 1
#define LearningOpenGL_VERSION_MINOR 0

// Runtime: first ancestor of the executable (or of cwd) that contains `Assets/`
const std::string& GetSolutionBasePath();
const std::wstring& GetWideSolutionBasePath();

#define solution_base_path GetSolutionBasePath()
#define wide_solution_base_path GetWideSolutionBasePath()

const std::string DefaultAssetsDirectory = GetSolutionBasePath() + "Assets/";
const std::string DefaultShaderDirectory = GetSolutionBasePath() + "Assets/Shaders/";
const std::string DefaultTextureDirectory = GetSolutionBasePath() + "Assets/Textures/";
const std::string DefaultFontDirectory = GetSolutionBasePath() + "Assets/Fonts/";
const std::string DefaultMeshDirectory = GetSolutionBasePath() + "Assets/Meshes/";
const std::string DefaultDataDirectory = GetSolutionBasePath() + "Assets/Data/";
const std::string DefaultLogDirectory = GetSolutionBasePath() + "Logs/";


const std::wstring WideDefaultAssetsDirectory = GetWideSolutionBasePath() + L"Assets/";
const std::wstring WideDefaultShaderDirectory = GetWideSolutionBasePath() + L"Assets/Shaders/";
const std::wstring WideDefaultTextureDirectory = GetWideSolutionBasePath() + L"Assets/Textures/";
const std::wstring WideDefaultFontDirectory = GetWideSolutionBasePath() + L"Assets/Fonts/";
const std::wstring WideDefaultMeshDirectory = GetWideSolutionBasePath() + L"Assets/Meshes/";
const std::wstring WideDefaultDataDirectory = GetWideSolutionBasePath() + L"Assets/Data/";
const std::wstring WideDefaultLogDirectory = GetWideSolutionBasePath() + L"Logs/";
