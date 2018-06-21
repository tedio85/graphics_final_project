#ifndef depth_fbo_h
#define depth_fbo_h

#include "../Externals/Include/Include.h"

extern GLuint depth_fbo, depth_tex;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;


void configure_depth_fbo();

#endif	/*depth_fbo_h*/
