//-----------------------------------------------------------------------------
//
// $Id: gl_bsp.h $
//
// Dynamic GL node generation for Doom Legacy
//
//-----------------------------------------------------------------------------

#ifndef gl_bsp_h
#define gl_bsp_h 1

// GL data
extern int numglvertexes;
extern vertex_t *glvertexes;
extern int numglsegs;
extern seg_t *glsegs;
extern int numglsubsectors;
extern subsector_t *glsubsectors;
extern int numglnodes;
extern node_t *glnodes;

// Function to build GL data
void BuildGLData();

#endif