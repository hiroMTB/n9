#include "MonitorNodeNL.h"
#include "cinder/audio/dsp/RingBuffer.h"
#include "cinder/audio/dsp/Fft.h"
#include "cinder/audio/Debug.h"
#include "cinder/CinderMath.h"

using namespace std;
using namespace ci;

namespace cinder { namespace audio {
    
    // ----------------------------------------------------------------------------------------------------
    // MARK: - MonitorNodeNL
    // ----------------------------------------------------------------------------------------------------
    
    MonitorNodeNL::MonitorNodeNL( const Format &format )
    : NodeAutoPullable( format ), mWindowSize( format.getWindowSize() ), mRingBufferPaddingFactor( 2 )
    {
    }
    
    MonitorNodeNL::~MonitorNodeNL()
    {
    }
    
    void MonitorNodeNL::initialize()
    {
        if( ! mWindowSize )
            mWindowSize = getFramesPerBlock();
        else if( ! isPowerOf2( mWindowSize ) )
            mWindowSize = nextPowerOf2( static_cast<uint32_t>( mWindowSize ) );
        
        for( size_t ch = 0; ch < getNumChannels(); ch++ )
            mRingBuffers.emplace_back( mWindowSize * mRingBufferPaddingFactor );
        
        mCopiedBuffer = Buffer( mWindowSize, getNumChannels() );
    }
    
    void MonitorNodeNL::process( Buffer *buffer )
    {
        size_t numFrames = min( buffer->getNumFrames(), mRingBuffers[0].getSize() );
        for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
            if( ! mRingBuffers[ch].write( buffer->getChannel( ch ), numFrames ) )
                return;
        }
    }
    
    const Buffer& MonitorNodeNL::getBuffer()
    {
        fillCopiedBuffer();
        return mCopiedBuffer;
    }
    
    float MonitorNodeNL::getVolume()
    {
        fillCopiedBuffer();
        return dsp::rms( mCopiedBuffer.getData(), mCopiedBuffer.getSize() );
    }
    
    float MonitorNodeNL::getVolume( size_t channel )
    {
        fillCopiedBuffer();
        return dsp::rms( mCopiedBuffer.getChannel( channel ), mCopiedBuffer.getNumFrames() );
    }
    
    //
    //  copy ringBuffer to mCopiedBuffer
    //
    void MonitorNodeNL::fillCopiedBuffer()
    {
        for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
            if( ! mRingBuffers[ch].read( mCopiedBuffer.getChannel( ch ), mCopiedBuffer.getNumFrames() ) )
                return;
        }
    }
    
    // ----------------------------------------------------------------------------------------------------
    // MARK: - MonitorSpectralNodeNL
    // ----------------------------------------------------------------------------------------------------
    
    MonitorSpectralNodeNL::MonitorSpectralNodeNL( const Format &format )
    : MonitorNodeNL( format ), mFftSize( format.getFftSize() ), mWindowType( format.getWindowType() ), mSmoothingFactor( 0.5f )
    {
    }
    
    MonitorSpectralNodeNL::~MonitorSpectralNodeNL()
    {
    }
    
    void MonitorSpectralNodeNL::initialize()
    {
        MonitorNodeNL::initialize();
        
        if( mFftSize < mWindowSize )
            mFftSize = mWindowSize;
        if( ! isPowerOf2( mFftSize ) )
            mFftSize = nextPowerOf2( static_cast<uint32_t>( mFftSize ) );
        
        mFft = unique_ptr<dsp::Fft>( new dsp::Fft( mFftSize ) );
        mFftBuffer = audio::Buffer( mFftSize );
        mBufferSpectral = audio::BufferSpectral( mFftSize );
        mMagSpectrum.resize( mFftSize / 2 );
        
        if( ! mWindowSize  )
            mWindowSize = mFftSize;
        else if( ! isPowerOf2( mWindowSize ) )
            mWindowSize = nextPowerOf2( static_cast<uint32_t>( mWindowSize ) );
        
        mWindowingTable = makeAlignedArray<float>( mWindowSize );
        generateWindow( mWindowType, mWindowingTable.get(), mWindowSize );
    }
    
    // TODO: When getNumChannels() > 1, use generic channel converter.
    // - alternatively, this tap can force mono output, which only works if it isn't a tap but is really a leaf node (no output).
    const std::vector<float>& MonitorSpectralNodeNL::getMagSpectrum( const Buffer & buffer )
    {
        //fillCopiedBuffer();
        mCopiedBuffer.copy( buffer );

        // window the copied buffer and compute forward FFT transform
        if( getNumChannels() > 1 ) {
            // naive average of all channels
            mFftBuffer.zero();
            float scale = 1.0f / getNumChannels();
            for( size_t ch = 0; ch < getNumChannels(); ch++ ) {
                for( size_t i = 0; i < mWindowSize; i++ )
                    mFftBuffer[i] += mCopiedBuffer.getChannel( ch )[i] * scale;
            }
            dsp::mul( mFftBuffer.getData(), mWindowingTable.get(), mFftBuffer.getData(), mWindowSize );
        }
        else
            dsp::mul( buffer.getData(), mWindowingTable.get(), mFftBuffer.getData(), mWindowSize );
        
        mFft->forward( &mFftBuffer, &mBufferSpectral );
        
        float *real = mBufferSpectral.getReal();
        float *imag = mBufferSpectral.getImag();
        
        // remove nyquist component
        imag[0] = 0.0f;
        
        // compute normalized magnitude spectrum
        // TODO: break this into vector cartisian -> polar and then vector lowpass. skip lowpass if smoothing factor is very small
        const float magScale = 1.0f / mFft->getSize();
        for( size_t i = 0; i < mMagSpectrum.size(); i++ ) {
            float re = real[i];
            float im = imag[i];
            mMagSpectrum[i] = mMagSpectrum[i] * mSmoothingFactor + sqrt( re * re + im * im ) * magScale * ( 1 - mSmoothingFactor );
        }
        
        return mMagSpectrum;
    }
    
    void MonitorSpectralNodeNL::setSmoothingFactor( float factor )
    {
        mSmoothingFactor = math<float>::clamp( factor );
    }
    
    float MonitorSpectralNodeNL::getFreqForBin( size_t bin )
    {
        return bin * getSampleRate() / (float)getFftSize();
    }
    
} } // namespace cinder::audio


