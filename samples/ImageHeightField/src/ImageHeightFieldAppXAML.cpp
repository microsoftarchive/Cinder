// ImageHeightFieldAppXAML

#include "cinder/app/AppBasic.h"
#include "cinder/ArcBall.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/Surface.h"
// zv
// #include "cinder/gl/Vbo.h"
#include "cinder/ImageIo.h"

// zv
#include "cinder/dx/dx.h"
#include "cinder/dx/DxVbo.h"
#include "cinder/app/RendererDx.h"
#include "cinder/dx/DxTexture.h"


using namespace ci;
using namespace ci::app;

class ImageHFApp : public AppBasic {
 public:
	void    setup();
	void    resize();
	void    mouseDown( MouseEvent event );
	void    mouseDrag( MouseEvent event );
	void    keyDown( KeyEvent event );
	void    draw();
	void	openFile();

 private:
 	enum ColorSwitch {
		kColor, kRed, kGreen, kBlue
	};
	void updateData( ColorSwitch whichColor );
 
	CameraPersp mCam;
	Arcball     mArcball;

	uint32_t    mWidth, mHeight;

	Surface32f		mImage;
	dx::VboMesh		mVboMesh;

    // zv
    dx::TextureRef  mTexture;
};

void ImageHFApp::setup()
{
	dx::enableAlphaBlending();
	dx::enableDepthRead();
	dx::enableDepthWrite();    

	// initialize the arcball with a semi-arbitrary rotation just to give an interesting angle
	mArcball.setQuat( Quatf( Vec3f( 0.0577576f, -0.956794f, 0.284971f ), 3.68f ) );

	openFile();
}

void ImageHFApp::openFile()
{
    try {
    /*  On WinRT it is required that use use the async version of getOpenFilePath() 
		since Windows 8 Store Apps only have an async version of the Open File dialog.
		You will need to provide a callback function or lamba that will receive the fs::path to
		the selected file.
	*/
    // zv added
	std::vector<std::string> extensions;
	extensions.push_back(".png");
	extensions.push_back(".jpg");

    // zv
    // fs::path path = getOpenFilePath( "", ImageIo::getLoadExtensions() );
    getOpenFilePath("", extensions, [this](fs::path path) {
        // converted to async lambda fn for WinRT
        if (!path.empty()) {
            // zv - need a Surface, not a Texture
            // 	static void loadImageAsync(const fs::path path, SurfaceT &surface, const SurfaceConstraints &constraints = SurfaceConstraintsDefault(), boost::tribool alpha = boost::logic::indeterminate );
            // loadImageAsync(path, [this](ImageSourceRef imageRef){
            loadImageAsync(path, [this](ImageSourceRef imageRef){
                // this->mTexture = dx::Texture( imageRef );
                // this->mTexture = dx::Texture::create(imageRef);
                // mImage
                // Surface32f::operator cinder::ImageSourceRef
            });
        }
    });

    /*
		mImage = loadImage( path );
	 
		mWidth = mImage.getWidth();
		mHeight = mImage.getHeight();

		dx::VboMesh::Layout layout;
		layout.setDynamicColorsRGB();
		layout.setDynamicPositions();
		mVboMesh = dx::VboMesh( mWidth * mHeight, 0, layout, GL_POINTS );

		updateData( kColor );		
	}
    */
	}
	catch( ... ) {
		console() << "unable to load the texture file!" << std::endl;
	}
}

void ImageHFApp::resize()
{
	mArcball.setWindowSize( getWindowSize() );
	mArcball.setCenter( Vec2f( getWindowWidth() / 2.0f, getWindowHeight() / 2.0f ) );
	mArcball.setRadius( getWindowHeight() / 2.0f );

	mCam.lookAt( Vec3f( 0.0f, 0.0f, -150 ), Vec3f::zero() );
	mCam.setPerspective( 60.0f, getWindowAspectRatio(), 0.1f, 1000.0f );
	dx::setMatrices( mCam );
}

void ImageHFApp::mouseDown( MouseEvent event )
{
    mArcball.mouseDown( event.getPos() );
}

void ImageHFApp::mouseDrag( MouseEvent event )
{
    mArcball.mouseDrag( event.getPos() );
}

void ImageHFApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'r':
			updateData( kRed );
		break;
		case 'g':
			updateData( kGreen );
		break;
		case 'b':
			updateData( kBlue );
		break;
		case 'c':
			updateData( kColor );
		break;
		case 'o':
			openFile();
		break;
	}
}

void ImageHFApp::draw()
{
    // zv  s/b ColorAT ?
   	dx::clear( Color( 0.0f, 0.0f, 0.0f ) );
    // glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    // glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    dx::pushModelView();
		dx::translate( Vec3f( 0.0f, 0.0f, mHeight / 2.0f ) );
		dx::rotate( mArcball.getQuat() );
		if( mVboMesh )
			dx::draw( mVboMesh );
    dx::popModelView();
}

void ImageHFApp::updateData( ImageHFApp::ColorSwitch whichColor )
{
	Surface32f::Iter pixelIter = mImage.getIter();
	dx::VboMesh::VertexIter vertexIter( mVboMesh );

	while( pixelIter.line() ) {
		while( pixelIter.pixel() ) {
			Color color( pixelIter.r(), pixelIter.g(), pixelIter.b() );
			float height;
			const float muteColor = 0.2f;

			// calculate the height based on a weighted average of the RGB, and emphasize either the red green or blue color in each of those modes
			switch( whichColor ) {
				case kColor:
					height = color.dot( Color( 0.3333f, 0.3333f, 0.3333f ) );
				break;
				case kRed:
					height = color.dot( Color( 1, 0, 0 ) );
					color *= Color( 1, muteColor, muteColor );
				break;
				case kGreen:
					height = color.dot( Color( 0, 1, 0 ) );
					color *= Color( muteColor, 1, muteColor );
				break;
				case kBlue:
					height = color.dot( Color( 0, 0, 1 ) );
					color *= Color( muteColor, muteColor, 1 );					
				break;            
			}

			// the x and the z coordinates correspond to the pixel's x & y
			float x = pixelIter.x() - mWidth / 2.0f;
			float z = pixelIter.y() - mHeight / 2.0f;

            vertexIter.setPosition( x, height * 30.0f, z );
			vertexIter.setColorRGB( color );
			++vertexIter;
		}
	}
}

CINDER_APP_BASIC( ImageHFApp, RendererDx );