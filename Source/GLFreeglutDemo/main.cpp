#include <stdio.h>
#include <stdlib.h>
#include "GL/freeglut.h"

int gMainWin = 0;

void SampleIdle( void )
{
    /* One could do all this just as well in SampleGameModeKeyboard... */    
    glutPostWindowRedisplay(gMainWin);
}

void SampleDisplay( void )
{
    const GLfloat time = glutGet(GLUT_ELAPSED_TIME) / 1000.f * 40;

    /*
        * Clear the screen
        */
    glClearColor( 0, 0.5, 1, 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /*
        * Have the cube rotated
        */
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();

    glRotatef( time, 0, 0, 1 );
    glRotatef( time, 0, 1, 0 );
    glRotatef( time, 1, 0, 0 );

    /*
        * And then drawn...
        */
    glColor3f( 1, 1, 0 );
    /* glutWireCube( 20.0 ); */
    //glutWireTeapot( 20.0 );
    glutSolidCube(5.f);
    /* glutWireSphere( 15.0, 15, 15 ); */
    /* glColor3f( 0, 1, 0 ); */
    /* glutWireCube( 30.0 ); */
    /* glutSolidCone( 10, 20, 10, 2 ); */

    /*
        * Don't forget about the model-view matrix
        */
    glPopMatrix( );

    /*
     * And swap this context's buffers
     */
    glutSwapBuffers( );
    glutPostWindowRedisplay(gMainWin);
}

void SampleReshape( int nWidth, int nHeight )
{
    GLfloat fAspect = (GLfloat) nHeight / (GLfloat) nWidth;
    GLfloat fPos[ 4 ] = { 0.0f, 0.0f, 10.0f, 0.0f };
    GLfloat fCol[ 4 ] = { 0.5f, 1.0f,  0.0f, 1.0f };

    /*
     * Update the viewport first
     */
    glViewport( 0, 0, nWidth, nHeight );

    /*
     * Then the projection matrix
     */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -fAspect, fAspect, 1.0, 80.0 );

    /*
     * Move back the camera a bit
     */
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
    glTranslatef( 0.0, 0.0, -40.0f );

    /*
     * Enable some features...
     */
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_NORMALIZE );

    /*
     * Set up some lighting
     */
    glLightfv( GL_LIGHT0, GL_POSITION, fPos );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    /*
     * Set up a sample material
     */
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fCol );
}

void SampleEntry(int state)
{
    int window = glutGetWindow () ;
    printf ( "Window %d Entry Callback: %d\n", window, state ) ;
}

void SampleKeyboard( unsigned char cChar, int nMouseX, int nMouseY )
{
    printf( "SampleKeyboard(): keypress '%c' at (%i,%i)\n",
            cChar, nMouseX, nMouseY );
}

int main(int argc, char** argv)
{
    glutInitDisplayString("first freeglut demo window");
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);

    glutInit(&argc, argv);

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    gMainWin = glutCreateWindow( "Hello world!" );

    glutIdleFunc( SampleIdle );
    glutDisplayFunc( SampleDisplay );
    glutReshapeFunc( SampleReshape );
    glutEntryFunc( SampleEntry );
    glutKeyboardFunc( SampleKeyboard );

    glutMainLoop();

    return EXIT_SUCCESS;
}