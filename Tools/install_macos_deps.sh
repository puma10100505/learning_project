#!/usr/bin/env bash
# macOS 下构建 learning_project 常用依赖（需已安装 Xcode Command Line Tools 与 Homebrew）
set -euo pipefail

echo "==> 检查 Command Line Tools..."
if ! xcode-select -p &>/dev/null; then
  echo "请先在终端执行: xcode-select --install"
  exit 1
fi

if ! command -v brew &>/dev/null; then
  echo "未检测到 Homebrew。安装见: https://brew.sh"
  echo '  安装命令: /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
  exit 1
fi

echo "==> brew 安装: cmake, ninja, glfw, assimp"
brew update
brew install cmake ninja glfw assimp

# 若链接阶段报 -lIrrXML 找不到，可再执行: brew install irrlicht
# PhysX 无官方 Homebrew 包，需使用工程自带或自编译的静态库，见下方说明

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo ""
echo "已安装/更新以上包。请确认 PhysX 静态库已放在工程目录:"
echo "  ${ROOT}/Libraries/MacOS/PhysX/Debug"
echo "  （需 libPhysX*_static_64.a 等与 SharedDef.cmake 中名称一致；若仓库用 Git LFS，可尝试: git lfs pull）"
echo ""
echo "配置并编译示例:"
echo "  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug"
echo "  cmake --build build -j\$(sysctl -n hw.ncpu)"
echo "  可执行文件会复制到 Bin/（由 deploy_files 目标决定）。"
