#pragma once

#include <cstdio>
#include <iostream>
#include <cstring>

#include "learning.h"

namespace LearningStatics
{
    static inline void GLDrawLine(const glm::vec3& P1, const glm::vec3& P2, const int LineWidth, const glm::vec4& LineColor)
    {
        glColor4f(LineColor.r, LineColor.g, LineColor.b, LineColor.a);
        glDisable(GL_LIGHTING);
        glLineWidth(LineWidth);
        glShadeModel(GL_FLAT);
        glBegin(GL_LINES);
        glVertex3f(P1.x, P1.y, P1.z);
        glVertex3f(P2.x, P2.y, P2.z);
        glEnd();
    }

    static inline void GLDrawWorldGrid(const float LineGap, const int LineNum, const int LineWidth, const glm::vec4& LineColor)
    {
        float LineLength = LineGap * LineNum;

        for (int i = 0; i < LineNum; i++)
        {
            glm::vec3 StartPt(i * LineGap - LineLength / 2, 0.f, LineLength / 2);
            glm::vec3 StopPt = StartPt + glm::vec3(0.f, 0.f, 1.f) * LineLength;
            
            GLDrawLine(StartPt, StopPt, LineWidth, LineColor);
        }

        for (int i = 0; i < LineNum; i++)
        {
            glm::vec3 StartPt(-LineLength / 2, 0.f, i * LineGap - LineLength / 2);
            glm::vec3 StopPt = StartPt + glm::vec3(1.f, 0.f, 0.f) * LineLength;
            
            GLDrawLine(StartPt, StopPt, LineWidth, LineColor);
        }
    }
}