#include "MyCamera.h"
#include "cinder/Camera.h"
#include "cinder/Function.h"

MyCamera::MyCamera(){
}

void MyCamera::setup(){
    
    mParams = params::InterfaceGl::create( "Camera Params", toPixels( Vec2i(250,250) ) );
    mParams->setOptions ( "", "position=`10 10` valueswidth=150" );
    mParams->addParam( "eyePoint", &mEyePoint).updateFn( bind( &MyCamera::updateCamera, this ) );
    mParams->addParam( "Fov degree", &mFov).updateFn( bind( &MyCamera::updateCamera, this ) );
    mParams->addParam( "Near plane", &mNearClip ).updateFn( bind( &MyCamera::updateCamera, this ) );
    mParams->addParam( "Far plane", &mFarClip ).updateFn( bind( &MyCamera::updateCamera, this ) );
    
    lookAt( Vec3f::zero() );
}

void MyCamera::drawParam(){
    mParams->draw();
}

void MyCamera::updateCamera(){
    mProjectionCached = false;
    mModelViewCached = false;
    lookAt( mEyePoint, Vec3f::zero() );
}