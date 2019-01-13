/*

 MIT License

 Copyright (c) 2017 Leonid Keselman

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 https://github.com/leonidk/imshow/blob/master/LICENSE

 */

#include "imshow/imshow.h"
#include <GLFW/glfw3.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <vector>

#pragma comment( lib, "opengl32" )

// FIX ide auto-indent
#define BEGIN_NAMESPACE_GLFW namespace glfw {
#define END_NAMESPACE_GLFW }

BEGIN_NAMESPACE_GLFW

struct winState
{
    GLFWwindow* win;
    GLuint tex;
    Image image;
    int x, y; // screen position
};

typedef std::unordered_map<std::string, winState>::const_iterator winStateIter;


struct glfwState
{
    glfwState() : isInitialized(false) {}
    ~glfwState()
    {
        if(isInitialized)
        {
            isInitialized = false;
            destroyAllWindows();
            glfwTerminate();
        }
    }
    void init()
    {
        if(!isInitialized)
        {
            isInitialized = true;
            glfwInit();
        }
    }
    void quit()
    {
        if(isInitialized)
        {
            destroyAllWindows();  
            isInitialized = false;
            glfwTerminate();
        }
    }
    
    winStateIter erase(winStateIter iter)
    {
        // Always invalidate window with existing name
        glfwMakeContextCurrent(iter->second.win);
        glfwDestroyWindow(iter->second.win);
        glDeleteTextures(1,&iter->second.tex);
        iter = windows.erase(iter);
        
        if(windows.empty())
        {
            quit();
        }
        return iter;
    }
    
    void destroyAllWindows()
    {
        if(isInitialized)
        {
            for (winStateIter i = windows.begin(); i != windows.end(); i = erase(i));
        }
    }
    
    std::unordered_map<std::string, winState> windows;
    std::mutex mutex;
    bool isInitialized = false;
    char key;
    bool key_update = false;
    bool close = true;
    
} g_state;

static winStateIter eraseWindow(winStateIter iter)
{
    return g_state.erase(iter);
}

static void eraseWindow(const char *name)
{
    auto iter = g_state.windows.find(name);
    if(iter != g_state.windows.end())
    {
        eraseWindow(iter);
    }
}

static winState * getWindowState(GLFWwindow *window)
{
    for (auto & win : g_state.windows)
    {
        if (win.second.win == window)
        {
            return &win.second;
        }
    }
    return nullptr;
}

static void render_callback(GLFWwindow * window)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA);

    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    {
        const auto win = getWindowState(window);
        glBindTexture(GL_TEXTURE_2D, win->tex);

        glBegin(GL_QUADS);
        glTexCoord2f(0, 1);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(1, 1);
        glTexCoord2f(0, 0);
        glVertex2f(-1, 1);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA);

    glPopMatrix();
    glPopAttrib();
}

static void position_callback(GLFWwindow *window, int x, int y)
{
    if(auto win = getWindowState(window))
    {
        win->x = x;
        win->y = y;
    }
}

static void focus_callback(GLFWwindow *window, int focus)
{
    if(focus)
    {
        glfwMakeContextCurrent(window);
        render_callback(window);
        glfwSwapBuffers(window);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        g_state.key = tolower(key);
        g_state.key_update = true;
        glfwPostEmptyEvent(); // be sure to notify event loop
    }
}
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    const auto win = getWindowState(window);
    const float wScale = ((float)width) / ((float)win->image.width);
    const float hScale = ((float)height) / ((float)win->image.height);
    const float minScale = (wScale < hScale) ? wScale : hScale;
    const int wShift = (int)nearbyint((width - minScale*win->image.width) / 2.0f);
    const int hShift = (int)nearbyint((height - minScale*win->image.height) / 2.0f);

    glfwMakeContextCurrent(window);
    glViewport(wShift, hShift, (int)nearbyint(win->image.width*minScale), (int)nearbyint(win->image.height*minScale));
    render_callback(window);
    glfwSwapBuffers(window);
}

static void refresh_callback(GLFWwindow *window)
{
    glfwMakeContextCurrent(window);
    render_callback(window);
    glfwSwapBuffers(window);
}

static void window_close_callback(GLFWwindow *window)
{
    // For a key update for the main loop
    g_state.close = true;
    glfwMakeContextCurrent(window);
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    glfwPostEmptyEvent();
}

/*
 * Above = private API
 * --------------------
 * Below = public API
 */

void imshow(const char * name, const Image & img)
{
    std::unique_lock<std::mutex> lock(g_state.mutex);
    
    std::string s_name(name);
    
    if(g_state.windows.empty())
    {
        g_state.init();
    }

    // Terminate any windows with the same name, create anew
    auto iter = g_state.windows.find(s_name);
    if(iter == g_state.windows.end())
    {
        // OS X:
        // * must create window prior to glGenTextures on OS X
        // * glfw complains w/ the following error on >= 10.12
        auto window = glfwCreateWindow(img.width, img.height, s_name.c_str(), NULL, NULL);

        glfwMakeContextCurrent(window);

        glfwSetKeyCallback(window, key_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetWindowRefreshCallback(window, render_callback);
        glfwSetWindowFocusCallback(window, focus_callback);
        glfwSetWindowPosCallback(window, position_callback);
        glfwSetWindowRefreshCallback(window, refresh_callback);
        glfwSetWindowCloseCallback(window, window_close_callback);

        GLuint tex;
        glGenTextures(1, &tex);

        int x, y;
        glfwGetWindowPos(window, &x, &y);
        
        g_state.windows[s_name] = {window, tex, img, x, y};
    }

    GLuint format, type;
    switch (img.type)
    {
        case IM_8U: type = GL_UNSIGNED_BYTE; break;
        case IM_8I: type = GL_BYTE; break;
        case IM_16U: type = GL_UNSIGNED_SHORT; break;
        case IM_16I: type = GL_SHORT; break;
        case IM_32U: type = GL_UNSIGNED_INT; break;
        case IM_32I: type = GL_INT; break;
        case IM_32F: type = GL_FLOAT; break;
        case IM_64F: type = GL_DOUBLE; break;
        default: return;
    }
    switch (img.channels)
    {
        case 0:
        case 1: format = GL_LUMINANCE; break;
        case 2: format = GL_LUMINANCE_ALPHA; break;
        case 3: format = GL_BGR; break;
        case 4: format = GL_BGRA; break;
        default: return; 
    }

    auto window = g_state.windows[s_name];
    glfwMakeContextCurrent(window.win);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, window.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, format, type, img.data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Retrieve actual size of allocated window framebuffer:
    int width, height;
    glfwGetFramebufferSize(window.win, &width, &height);
    framebuffer_size_callback(window.win, width, height);
}

char getKey(bool wait)
{
    std::unique_lock<std::mutex> lock(g_state.mutex);
    g_state.close = false;

    std::vector<std::string> toDelete;
    if (wait)
    {
        do
        {
            glfwWaitEvents();
            for(const auto &win : g_state.windows)
            {
                if(glfwWindowShouldClose(win.second.win))
                {
                    g_state.close = true;
                    toDelete.push_back(win.first);
                }
            }
        }
        while (!g_state.key_update && !g_state.close);
    }
    else
    {
        glfwPollEvents();
    }
    
    g_state.key_update = false;
    for (const auto & name : toDelete)
    {
        eraseWindow(name.c_str());
    }
    return g_state.key;
}
    
void destroyWindow(const char *name)
{
    std::unique_lock<std::mutex> lock(g_state.mutex);
    eraseWindow(name);
}

void destroyAllWindows()
{
    std::unique_lock<std::mutex> lock(g_state.mutex);
    g_state.destroyAllWindows();
}

END_NAMESPACE_GLFW
