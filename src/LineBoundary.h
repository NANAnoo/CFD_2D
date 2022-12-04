//
// Created by ZhangHao on 2022/12/4.
//

#ifndef CFD_2D_LINE_BOUNDARY_H
#define CFD_2D_LINE_BOUNDARY_H

#include "Fluid2D.h"
#include "GLRenderable.h"

// a line boundary between start and end
class LineBoundary final : public BoundaryI, public GLRenderableI {
private:
    vec2 u_start;
    vec2 u_end;
    vec2 start;
    vec2 end;
    vec2 normal;
    vec2 direction;
    float damp;
public:
    // in a uniform CS [0, 1] x [0, 1], with damp d
    explicit LineBoundary(float start_x, float start_y, float end_x, float end_y, float d = 0)
    : u_start(vec2(start_x, start_y)), u_end(vec2(end_x, end_y)), damp(d) {
        updateCS(0, 1, 0, 1);
    }

    void updateCS(float top, float bottom, float right, float left) override {
        vec2 scale(top - bottom, right - left);
        start = u_start * scale;
        end = u_end * scale;
        normal = vec2(start.y() - end.y(), end.x() - start.x());
        normal = normal.normalize();
        direction = vec2(start.x() - end.x(), start.y() - end.y());
        direction = direction.normalize();
    }

    bool updateAt(int index, Vec<D2> next_pos,
                  std::vector<Vec<D2>> &all_pos,
                  std::vector<Vec<D2>> &all_vel) override {
        // check if the line (start, end) and (next pos, prev pos) is collided
        vec2 prev_pos = all_pos[index];
        vec2 vel = all_vel[index];
        vec2 prev_next = prev_pos - next_pos;
        vec2 start_prev = prev_pos - start;
        vec2 start_next = next_pos - start;
        vec2 end_prev = prev_pos - end;
        if (std::abs(start_prev.Mul(normal)) < 0.0001 &&
                (direction.Mul(end_prev) * direction.Mul(start_prev) <= 0)) {
            float mod = (2 - damp) * vel.Mul(normal);
            vec2 dv = normal * mod;
            vec2 d_pos = dv.normalize() * 0.1;
            all_vel[index] = vel - dv;
            all_pos[index] = prev_pos - d_pos;
            return true;
        }

        if (normal.Mul(start_prev) * normal.Mul(start_next) < 0) {
            // prev and next are on two side of this line
            vec2 n(prev_next.y(), -prev_next.x());
            if (n.Mul(start_prev) * n.Mul(end_prev) < 0) {
                // start and end are on two side of particle path,
                // has intersection, reject position update,
                // and then update velocity, bounce back!
                float mod =(2 - damp) * vel.Mul(normal);
                vec2 dv = normal * mod;
                vec2 d_pos = dv.normalize() * 0.01;
                all_vel[index] = vel - dv;
                all_pos[index] = prev_pos - d_pos;
                return true;
            }
        }
        return false;
    }

    bool isSeperated(vec2 a, vec2 b) override
    {
        vec2 start_a = a - start;
        vec2 start_b = b - start;
        vec2 a_b = b - a;
        if (normal.Mul(start_a) * normal.Mul(start_b) < 0) {
            vec2 end_b = b - end;
            vec2 n(a_b.y(), a_b.x());
            // are on two side
            return n.Mul(start_b) * n.Mul(end_b) < 0;
        }
    }

    // draw the line
    void update() override {
        glColor3f(0.8, 0.5, 0.1);
        glLineWidth(5);
        glBegin(GL_LINES);
        glVertex3f(u_start.x() * 2 - 1, u_start.y() * 2 - 1, 0);
        glVertex3f(u_end.x() * 2 - 1, u_end.y() * 2 - 1, 0);
        glEnd();
    }
};


#endif //CFD_2D_LINE_BOUNDARY_H
