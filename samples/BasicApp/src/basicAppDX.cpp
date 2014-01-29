#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererDx.h"
#include "cinder/dx/dx.h"

#include <list>

using namespace ci;
using namespace ci::app;
using namespace std;

// We'll create a new Cinder Application by deriving from the CinderMain class
// zv
// class BasicApp : public CinderMain {
class BasicApp : public AppBasic {
  public:
    void mouseDrag( MouseEvent event );
    void keyDown( KeyEvent event );
    void draw();

	// This will maintain a list of points which we will draw line segments between
	list<Vec2f>		mPoints;
};

void BasicApp::mouseDrag( MouseEvent event )
{
    mPoints.push_back( event.getPos() );
}

void BasicApp::keyDown( KeyEvent event )
{
    char key = event.getChar();
    if (key == 'e' || key == 'E')
    {
        mPoints.clear();
    }
}

void BasicApp::draw()
{
	dx::clear(Color(0.1f, 0.1f, 0.15f));

    dx::color( 1.0f, 0.5f, 0.25f );	
	dx::begin( GL_LINE_STRIP );
	for( auto pointIter = mPoints.begin(); pointIter != mPoints.end(); ++pointIter ) {
		dx::vertex( *pointIter );
	}
	dx::end();
}

// This line tells Cinder to actually create the application
CINDER_APP_BASIC( BasicApp, RendererDx )

/*

zv debug

int mainXAML() {
    cinder::app::AppBasic::prepareLaunch();
    cinder::app::AppBasic *app = new BasicApp;
    cinder::app::RendererRef ren(new RendererDx);
    cinder::app::AppBasic::executeLaunch(app, ren, "BasicApp");
    cinder::app::AppBasic::cleanupLaunch();
    return 0;
}

*/
