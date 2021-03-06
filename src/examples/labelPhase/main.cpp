#include "CImg.h"
using namespace cimg_library;

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "getTime.h"
#include "phase.h"
#include"labelPhase.h"

#ifdef _RPI
#include "bcm_host.h"
#endif

struct ESContext
{
   /// Window width
   GLint       width;

   /// Window height
   GLint       height;

   /// Window handle
   EGLNativeWindowType  hWnd;

   /// EGL display
   EGLDisplay  eglDisplay;

   /// EGL context
   EGLContext  eglContext;

   /// EGL surface
   EGLSurface  eglSurface;
} esContext;


#define EGL_CHECK(stmt) do { \
        if(!stmt) { \
	   checkEGLError(#stmt, __FILE__, __LINE__); \
        }\
    } while(0)

void checkEGLError(const char *stmt, const char *fname, int line)
{
    GLenum err = eglGetError();
    if (err != EGL_SUCCESS)
    {
        printf("EGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}


int initEGL(int width, int height)
{
    EGLDisplay eglDisplay;
    EGLConfig eglConfig;

    esContext.width  = width;
    esContext.height = height;

// Step 1 - Get the default display.
   eglDisplay = eglGetDisplay((EGLNativeDisplayType)0);
   EGL_CHECK( (eglDisplay != EGL_NO_DISPLAY) );

// Step 2 - Initialize EGL.
   EGL_CHECK(eglInitialize(eglDisplay, NULL, NULL) );

   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                                          EGL_NONE };

// Step 3 - Make OpenGL ES the current API.
   EGL_CHECK( eglBindAPI(EGL_OPENGL_ES_API) );

// Step 4 - Specify the required configuration attributes.
   EGLint attribList[] = { EGL_SURFACE_TYPE   , EGL_PBUFFER_BIT,
                           EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                           EGL_NONE
                         };

// Step 5 - Find a config that matches all requirements.
   EGLint iConfigs = 0;
   EGL_CHECK( eglChooseConfig(eglDisplay, attribList, &eglConfig, 1, &iConfigs) );

   if (iConfigs != 1) {
       printf("Error: eglChooseConfig(): config not found.\n");
       return EGL_FALSE;
   }

// Step 6 - Create a surface to draw to.

   // Necessary to set EGL_WIDTH and EGL_HEIGHT to at least 1
   // if left default (0) the rpi gets into troubles
   const EGLint srfPbufferAttr[] =
   {
       EGL_WIDTH, 1,
       EGL_HEIGHT, 1,
       EGL_NONE
   };
   EGLSurface eglSurface;
   eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, srfPbufferAttr);
   EGL_CHECK( (eglSurface != EGL_NO_SURFACE) );

// Step 7 - Create a context.
   EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, contextAttribs);
   EGL_CHECK( (eglContext != EGL_NO_CONTEXT) );

// Step 8 - Bind the context to the current thread
   EGL_CHECK( eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) );

   esContext.eglDisplay = eglDisplay;
   esContext.eglSurface = eglSurface;
   esContext.eglContext = eglContext;

   return EGL_TRUE;
}

int main()
{
#ifdef _RPI
    bcm_host_init();
#endif

    // Initialize esContext to 0
    esContext = {};

    GLuint fboId[2] ;
    LabelPhase labelPhase;
    labelPhase.mVertFilename = "quad.vert";
    labelPhase.mFragFilename = "labelPhase.frag";


    // Read PNG-file
    labelPhase.mImage.assign("test.png");
    int width  = labelPhase.mImage.width();
    int height = labelPhase.mImage.height();
    labelPhase.mWidth  = width;
    labelPhase.mHeight = height;
    labelPhase.mImage.mirror("y");
    labelPhase.mImage.permute_axes("cxyz");

    // initialize EGL-context
    initEGL(width, height);

    // initialize the 2 frambuffers for ping-pong method
    GL_CHECK( glGenFramebuffers(2, fboId) );

    unsigned usedTexUnits = 0;
    if(!labelPhase.initIndependent(fboId, usedTexUnits) )
        exit(1);

    double labelTime;


    labelPhase.setupGeometry();
    labelTime = labelPhase.run();

    cout << "Label time: " << labelTime << endl;


#ifdef _RPI
    bcm_host_deinit();
#endif

    return 0;
}

