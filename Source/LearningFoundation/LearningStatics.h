#pragma once

#include <cstdio>
#include <iostream>
#include <cstring>

#include "learning.h"

namespace LearningStatics
{
    static inline void GLDrawLine(const glm::vec3& P1, const glm::vec3& P2, const int LineWidth, const glm::vec3& LineColor)
    {
        // glDisable(GL_LIGHTING);
        // glShadeModel(GL_FLAT);
        glLineWidth(LineWidth);
        glBegin(GL_LINES);
        glColor3f(LineColor.x, LineColor.y, LineColor.z);
        glVertex3f(P1.x, P1.y, P1.z);
        glVertex3f(P2.x, P2.y, P2.z);
        glEnd();
        // printf("Finished draw a line from (%f, %f, %f) to (%f, %f, %f)\n", 
        //     P1.x, P1.y, P1.z, P2.x, P2.y, P2.z);
    }

    static inline void GLDrawWorldGrid(const float LineGap, const int LineNum, const int LineWidth, const glm::vec3& LineColor)
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