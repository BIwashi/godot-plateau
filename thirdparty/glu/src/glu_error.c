/*
 * gluErrorString() for the bundled libtess. Mesa GLU implements it in
 * libutil/error.c together with the rest of GLU; libtess only reports
 * tessellator errors, so only those are named here.
 */
#include <GL/glu.h>

const GLubyte *GLAPIENTRY gluErrorString(GLenum errorCode)
{
    switch (errorCode) {
    case GLU_TESS_MISSING_BEGIN_POLYGON: return (const GLubyte *)"missing gluTessBeginPolygon";
    case GLU_TESS_MISSING_BEGIN_CONTOUR: return (const GLubyte *)"missing gluTessBeginContour";
    case GLU_TESS_MISSING_END_POLYGON: return (const GLubyte *)"missing gluTessEndPolygon";
    case GLU_TESS_MISSING_END_CONTOUR: return (const GLubyte *)"missing gluTessEndContour";
    case GLU_TESS_COORD_TOO_LARGE: return (const GLubyte *)"tessellation coordinate too large";
    case GLU_TESS_NEED_COMBINE_CALLBACK: return (const GLubyte *)"need combine callback";
    case GLU_OUT_OF_MEMORY: return (const GLubyte *)"out of memory";
    case GLU_INVALID_ENUM: return (const GLubyte *)"invalid enumerant";
    case GLU_INVALID_VALUE: return (const GLubyte *)"invalid value";
    default: return (const GLubyte *)"unknown GLU error";
    }
}
