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
	void keyDown( KeyEvent event );
	void draw();

    // The sound effect source to be played
    OpenAL::Sound*  m_pSfx;
    OpenAL::Sound*  m_pSfxUp;
    OpenAL::Sound*  m_pSfxDown;
    OpenAL::Sound*  m_pSfxLeft;
    OpenAL::Sound*  m_pSfxRight;

    ALuint m_monoBuffer;
};

void BasicApp::setup()
{
    OpenAL::InitOpenAL();

    // Create a Sound with a buffer automatically
    // OpenAL block will handle buffer cleanup
    m_pSfx = new OpenAL::Sound(ci::app::loadResource(RES_SFX_STEREO_SOUND));

    // Create multiple sounds that share the same buffer
    // Your application is responsible for buffer cleanup
    // Note that 3D audio only works with MONO sounds
    m_monoBuffer = OpenAL::CreateBuffer(ci::app::loadResource(RES_SFX_MONO_SOUND));
    m_pSfxUp    = new OpenAL::Sound(m_monoBuffer);
    m_pSfxDown  = new OpenAL::Sound(m_monoBuffer);
    m_pSfxLeft  = new OpenAL::Sound(m_monoBuffer);
    m_pSfxRight = new OpenAL::Sound(m_monoBuffer);
    m_pSfxUp->m_position    = Vec3f( 0.f,  1.f,  0.f);
    m_pSfxDown->m_position  = Vec3f( 0.f, -1.f,  0.f);
    m_pSfxLeft->m_position  = Vec3f(-1.f,  0.f,  0.f);
    m_pSfxRight->m_position = Vec3f( 1.f,  0.f,  0.f);
}
void BasicApp::shutdown()
{
    delete m_pSfx;
    delete m_pSfxUp;
    delete m_pSfxDown;
    delete m_pSfxLeft;
    delete m_pSfxRight;

    OpenAL::DestroyBuffer(m_monoBuffer);

    OpenAL::DestroyOpenAL();
}

void BasicApp::mouseDown( MouseEvent event )
{
    m_pSfx->Play();
}

void BasicApp::keyDown( KeyEvent event )
{
	auto key = event.getCode();

    switch (key)
    {
        case ci::app::KeyEvent::KEY_UP:
            m_pSfxUp->Play();
            break;
        case ci::app::KeyEvent::KEY_DOWN:
            m_pSfxDown->Play();
            break;
        case ci::app::KeyEvent::KEY_LEFT:
            m_pSfxLeft->Play();
            break;
        case ci::app::KeyEvent::KEY_RIGHT:
            m_pSfxRight->Play();
            break;
        case ci::app::KeyEvent::KEY_ESCAPE:
            quit();
            break;
        default:
            break;
    }
}

void BasicApp::draw()
{
	gl::clear( Color( 0.1f, 0.1f, 0.15f ) );
}

// This line tells Cinder to actually create the application
CINDER_APP_BASIC( BasicApp, RendererGl )