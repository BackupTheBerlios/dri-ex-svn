/* Test GL_EXT_stencil_wrap extension.
 * This is by no means complete, just a quick check.
 *
 * Brian Paul  30 October 2002
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static void RunTest(void)
{
   const GLenum prim = GL_QUAD_STRIP;
   GLubyte val;
   int bits, max, i;
   GLboolean failed;

   glGetIntegerv(GL_STENCIL_BITS, &bits);
   max = (1 << bits) - 1;

   glClearStencil(0);
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS, 0, ~0);

   /* test GL_INCR (saturation) */
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
   failed = GL_FALSE;
   printf("Testing GL_INCR...\n");
   for (i = 1; i < max+10; i++) {
      int expected = (i > max) ? max : i;
      glBegin(prim);
      glVertex2f(0, 0);      glVertex2f(10, 0);
      glVertex2f(0, 10);      glVertex2f(10, 10);
      glEnd();

      glReadPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &val);
      if (val != expected) {
	 printf( "Failed GL_INCR test on iteration #%u "
		 "(got %u, expected %u)\n", i, val, expected );
	 failed = GL_TRUE;
      }	   
   }

   if ( !failed ) printf("OK!\n");


   /* test GL_INCR_WRAP_EXT (wrap around) */
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
   failed = GL_FALSE;
   printf("Testing GL_INCR_WRAP_EXT...\n");
   for (i = 1; i < max+10; i++) {
      int expected = i % (max + 1);
      glBegin(prim);
      glVertex2f(0, 0);      glVertex2f(10, 0);
      glVertex2f(0, 10);      glVertex2f(10, 10);
      glEnd();
      glReadPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &val);
      if (val != expected) {
	 printf( "Failed GL_INCR_WRAP test on iteration #%u "
		 "(got %u, expected %u)\n", i, val, expected );
	 failed = GL_TRUE;
      }	   
   }
   if ( !failed ) printf("OK!\n");

   glClearStencil(max);

   /* test GL_INCR (saturation) */
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
   failed = GL_FALSE;
   printf("Testing GL_DECR...\n");
   for (i = max-1; i > -10; i--) {
      int expected = (i < 0) ? 0 : i;
      glBegin(prim);
      glVertex2f(0, 0);      glVertex2f(10, 0);
      glVertex2f(0, 10);      glVertex2f(10, 10);
      glEnd();
      glReadPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &val);
      if (val != expected) {
	 printf( "Failed GL_DECR test on iteration #%u "
		 "(got %u, expected %u)\n", max - i, val, expected );
	 failed = GL_TRUE;
      }	   
   }
   if ( !failed ) printf("OK!\n");

   /* test GL_INCR_WRAP_EXT (wrap-around) */
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);
   failed = GL_FALSE;
   printf("Testing GL_DECR_WRAP_EXT...\n");
   for (i = max-1; i > -10; i--) {
      int expected = (i < 0) ? max + i + 1: i;
      glBegin(prim);
      glVertex2f(0, 0);      glVertex2f(10, 0);
      glVertex2f(0, 10);      glVertex2f(10, 10);
      glEnd();
      glReadPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &val);
      if (val != expected) {
	 printf( "Failed GL_DECR_WRAP test on iteration #%u "
		 "(got %u, expected %u)\n", max - i, val, expected );
	 failed = GL_TRUE;
      }	   
   }
   if ( !failed ) printf("OK!\n");

   glDisable(GL_STENCIL_TEST);
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   RunTest();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   const char * ver_str;
   float        version;

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));


   /* Check for both the extension string and GL version 1.4 on the
    * outside chance that some silly vendor exports version 1.4 but doesn't
    * export the extension string.  The stencil-wrap modes are a required
    * part of GL 1.4.
    */

   ver_str = glGetString( GL_VERSION );
   version = (ver_str == NULL) ? 1.0 : atof( ver_str );

   if ( !glutExtensionSupported("GL_EXT_stencil_wrap")
	&& (version < 1.4) ) {
      printf("Sorry, GL_EXT_stencil_wrap not supported.\n");
      exit(1);
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
