#include "cinder/app/AppBasic.h"
#include "Resources.h"
#include "OpenAL.h"
#include <list>

using namespace ci;
using namespace ci::app;
using namespace std;

// We'll create a new Cinder Application by deriving from the AppBasic class
class BasicApp : public AppBasic {
  public:
    void setup();
    void shutdown();
    void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );
	void draw();

	// This will maintain a list of points which we will draw line segments between
	list<Vec2f>		mPoints;

    // The sound effect source to be played
    ALuint          m_sfx1;
};

void BasicApp::setup()
{
    OpenAL::InitOpenAL();
    m_sfx1 = OpenAL::CreateSource(OpenAL::CreateBuffer(ci::app::loadResource(RES_SFX1_SOUND)));
}
void BasicApp::shutdown()
{
    alDeleteSources(1, &m_sfx1);
    OpenAL::DestroyOpenAL();
}

void BasicApp::mouseDown( MouseEvent event )
{
    alSourcePlay(m_sfx1);
}

void BasicApp::mouseDrag( MouseEvent event )
{
	mPoints.push_back( event.getPos() );
}

void BasicApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
}

void BasicApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.15f ) );

	gl::color( 1.0f, 0.5f, 0.25f );	
	gl::begin( GL_LINE_STRIP );
	for( auto pointIter = mPoints.begin(); pointIter != mPoints.end(); ++pointIter ) {
		gl::vertex( *pointIter );
	}
	gl::end();
}

// This line tells Cinder to actually create the application
CINDER_APP_BASIC( BasicApp, RendererGl )