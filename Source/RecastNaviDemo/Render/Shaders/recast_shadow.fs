#version 330 core
// =============================================================================
// recast_shadow.fs
//   仅深度写入；颜色 buffer 在 shadow FBO 里被禁用 (glDrawBuffer(GL_NONE))。
//   GL 仍要求有片元着色器存在，故这里保留一个空 main()。
// =============================================================================

void main()
{
    // 留空：硬件自动写入 gl_FragCoord.z 到 GL_DEPTH_ATTACHMENT
}
