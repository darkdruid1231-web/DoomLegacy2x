#ifndef gl_bsp_h
#define gl_bsp_h

class Map;

// Use BspCompiler (ZDBSP library) for in-process GL node generation
bool BuildGLDataWithBspCompiler(Map *map);

void BuildGLData(Map *map);

#endif