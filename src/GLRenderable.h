#ifndef GL_REANDERABLE_H
#define GL_REANDERABLE_H

class GLRenderableI {
public:
    virtual void update() = 0; 
    virtual ~GLRenderableI()
    {

    }
};

#endif //GL_REANDERABLE_H