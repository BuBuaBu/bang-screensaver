/* bang, Copyright (c) 2014 Vivien HENRIET <bubuabu@bubuabu.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "screenhackI.h"
#include "config.h"

#include <stdarg.h>
#include <limits.h>

#include "xlockmore.h"
#include "bang.h"

#define DEFAULTS	"*delay:	00000       \n" \
			"*showFPS:      True       \n" \

# define refresh_bang 0

ENTRYPOINT ModeSpecOpt bang_opts = {0, NULL, 0, NULL, NULL};

static bangstruct *bang=NULL;

#include <math.h>

#define X .525731112119133606 
#define Z .850650808352039932

static GLfloat vdata[12][3] = {    
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},    
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},    
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0} 
};
static GLuint tindices[20][3] = { 
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},    
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},    
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, 
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

void normalize(GLfloat *a) {
    GLfloat d=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    a[0]/=d; a[1]/=d; a[2]/=d;
}

void drawtri(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r) {
	int i;
    if (div<=0) {
        glNormal3fv(a); glVertex3f(a[0]*r, a[1]*r, a[2]*r);
        glNormal3fv(b); glVertex3f(b[0]*r, b[1]*r, b[2]*r);
        glNormal3fv(c); glVertex3f(c[0]*r, c[1]*r, c[2]*r);
    } else {
        GLfloat ab[3], ac[3], bc[3];
        for (i=0;i<3;i++) {
            ab[i]=(a[i]+b[i])/2;
            ac[i]=(a[i]+c[i])/2;
            bc[i]=(b[i]+c[i])/2;
        }
        normalize(ab); normalize(ac); normalize(bc);
        drawtri(a, ab, ac, div-1, r);
        drawtri(b, bc, ab, div-1, r);
        drawtri(c, ac, bc, div-1, r);
        drawtri(ab, bc, ac, div-1, r); /*   <--Comment this line and sphere looks really cool!*/
    }  
}

void drawsphere(float radius, float x, float y, float z) {
	int i;
	int ndiv = 3;
	glPushMatrix ();
	glTranslated(x, y, z);
    glBegin(GL_TRIANGLES);
    for (i=0;i<20;i++)
        drawtri(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], ndiv, radius);
    glEnd();
    glPopMatrix ();
}

ENTRYPOINT void reshape_bang (ModeInfo *mi, int w, int h){
	float ratio;

	/* Prevent a divide by zero, when window is too short
	 (you cant make a window of zero width).*/
	if(h == 0)
		h = 1;

	ratio = 1.0* w / h;

	/* Reset the coordinate system before modifying*/
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* Set the viewport to be the entire window*/
	glViewport(0, 0, w, h);

	/* Set the correct perspective.*/
	gluPerspective(45.0f,ratio,2.0f,100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0.0f, 0.0f, 0.0f);
	
}

ENTRYPOINT void init_bang (ModeInfo *mi)
{
	bangstruct *env;

	if (!bang)
	{
		bang=calloc (MI_NUM_SCREENS(mi), sizeof(bangstruct));
	}
	env = &bang[MI_SCREEN(mi)];
	env->alpha = 0;
	env->glx_context = init_GL(mi);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	reshape_bang(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
	
	glEnable(GL_DEPTH_TEST);

	env->game_time=time(NULL);
}

ENTRYPOINT void release_bang (ModeInfo * mi) 
{
	if (bang)
	{
		free((void *) bang);
		bang = (bangstruct *) NULL;
	}
	FreeAllGL(mi);
}

ENTRYPOINT void draw_bang (ModeInfo *mi)
{
	Display *dpy = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	GLfloat camx, camz;
	float r = 10.0;
	int LightPos[4] = {-10,0,0,1};
	int MatSpec [4] = {1,1,1,1};

	glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *(bang->glx_context));

	glMaterialiv(GL_FRONT_AND_BACK,GL_SPECULAR,MatSpec);
	glMateriali(GL_FRONT_AND_BACK,GL_SHININESS,100);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* set camera position */

	camx = r*sin(bang->alpha) + 0.5;
	camz = r*cos(bang->alpha) + 0.5;
	bang->alpha += 0.01;
	if (bang->alpha == 360)
	{
		bang->alpha = 0;
	}
	
	gluLookAt(camx, 0.0, camz, 0.0, 0.0, 0.0, 0, 1, 0);
	glLightiv(GL_LIGHT0, GL_POSITION, LightPos);
/*	gluLookAt(10, 0.0, 10, 0, 0, 0, 0, 1, 0);*/
	
	glPushMatrix ();

	drawsphere(0.1, 0.0, 0.0, 0.0);
	drawsphere(0.5, 3.0, 0.0, 0.0);
	drawsphere(1.0, 6.0, 0.0, 0.0);

	glPopMatrix ();

	if (MI_IS_FPS(mi)) do_fps (mi);

	glXSwapBuffers(dpy, window);
}

ENTRYPOINT Bool bang_handle_event (ModeInfo *mi, XEvent *event)
{
	return False;
}

XSCREENSAVER_MODULE ("Bang", bang)

