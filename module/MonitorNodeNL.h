/*
 
        This is almost same code with cinder/audio/dsp/MonitorNode.h

        But NL (None Realtime) version. ( actually the name must be NR, I notice it now... )
        You can manualy set buffer witch you want to calculate FFT.
 */

#pragma once

#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Dsp.h"
#include "cinder/audio/dsp/RingBuffer.h"

#include "cinder/Thread.h"

namespace cinder { namespace audio {
    
    namespace dsp {
        class Fft;
    }
    
    typedef std::shared_ptr<class MonitorNodeNL>			MonitorNodeNLRef;
    typedef std::shared_ptr<class MonitorSpectralNodeNL>	MonitorSpectralNodeNLRef;
    
    //!	\brief Node for retrieving time-domain audio PCM samples.
    //!
    //!	MonitorNodeNL provides a way to copy PCM samples from the audio thread and safely use them on the user (normally main) thread.
    //! Also provides RMS volume analysis.
    //!
    //! This Node does not modify the incoming Buffer in its process() function and does not need to be connected to a OutputNode.
    //!
    //! \note Internally, a dsp::RingBuffer is used, which has a limited size. Once it fills up, more samples will not be written
    //! until space is made by calling getBuffer(). In practice, this isn't a problem since this method is normally called from within the update() or draw() loop.
    class MonitorNodeNL : public NodeAutoPullable {
    public:
        struct Format : public Node::Format {
            Format() : mWindowSize( 0 ) {}
            
            //! Sets the window size, the number of samples that are recorded for one 'window' into the audio signal. Default is the Context's frames-per-block.
            //! \note will be rounded up to the nearest power of two.
            Format&		windowSize( size_t size )		{ mWindowSize = size; return *this; }
            //! Returns the window size.
            size_t		getWindowSize() const			{ return mWindowSize; }
            
            // reimpl Node::Format
            Format&		channels( size_t ch )					{ Node::Format::channels( ch ); return *this; }
            Format&		channelMode( ChannelMode mode )			{ Node::Format::channelMode( mode ); return *this; }
            Format&		autoEnable( bool autoEnable = true )	{ Node::Format::autoEnable( autoEnable ); return *this; }
            
        protected:
            size_t mWindowSize;
        };
        
        MonitorNodeNL( const Format &format = Format() );
        virtual ~MonitorNodeNL();
        
        //! Returns a filled Buffer of the sampled audio stream, suitable for consuming on the main UI thread.
        //! \note samples will only be copied if there is enough available in the internal dsp::RingBuffer.
        const Buffer& getBuffer();
        //! Returns the window size, which is the number of samples that are copied from the audio stream. Equivalent to: \code getBuffer().size() \endcode.
        size_t getWindowSize() const	{ return mWindowSize; }
        //! Compute the average (RMS) volume across all channels
        float getVolume();
        //! Compute the average (RMS) volume across \a channel
        float getVolume( size_t channel );
        
    protected:
        void initialize()				override;
        void process( Buffer *buffer )	override;
        
        //! Copies audio frames from the RingBuffer into mCopiedBuffer, which is suitable for operation on the main thread.
        void fillCopiedBuffer();
        
        std::vector<dsp::RingBuffer>	mRingBuffers;	// one per channel
        Buffer							mCopiedBuffer;	// used to safely read audio frames on a non-audio thread
        size_t							mWindowSize;
        size_t							mRingBufferPaddingFactor;
    };
    
    //! A Scope that performs spectral (Fourier) analysis.
    class MonitorSpectralNodeNL : public MonitorNodeNL {
    public:
        struct Format : public MonitorNodeNL::Format {
            Format() : MonitorNodeNL::Format(), mFftSize( 0 ), mWindowType( dsp::WindowType::BLACKMAN ) {}
            
            //! Sets the FFT size, rounded up to the nearest power of 2 greater or equal to \a windowSize. Setting this larger than \a windowSize causes the FFT transform to be 'zero-padded'. Default is the same as windowSize.
            //! \note resulting number of output spectral bins is equal to (\a size / 2)
            Format&     fftSize( size_t size )              { mFftSize = size; return *this; }
            //! defaults to WindowType::BLACKMAN
            Format&		windowType( dsp::WindowType type )	{ mWindowType = type; return *this; }
            //! \see Scope::windowSize()
            Format&		windowSize( size_t size )			{ MonitorNodeNL::Format::windowSize( size ); return *this; }
            
            size_t			getFftSize() const				{ return mFftSize; }
            dsp::WindowType	getWindowType() const			{ return mWindowType; }
            
            // reimpl Node::Format
            Format&		channels( size_t ch )					{ Node::Format::channels( ch ); return *this; }
            Format&		channelMode( ChannelMode mode )			{ Node::Format::channelMode( mode ); return *this; }
            Format&		autoEnable( bool autoEnable = true )	{ Node::Format::autoEnable( autoEnable ); return *this; }
            
        protected:
            size_t			mFftSize;
            dsp::WindowType	mWindowType;
        };
        
        MonitorSpectralNodeNL( const Format &format = Format() );
        virtual ~MonitorSpectralNodeNL();
        
        //! Returns the magnitude spectrum of the currently sampled audio stream, suitable for consuming on the main UI thread.
        const	std::vector<float>& getMagSpectrum( const Buffer & buffer );
        //! Returns the number of frequency bins in the analyzed magnitude spectrum. Equivilant to fftSize / 2.
        size_t	getNumBins() const				{ return mFftSize / 2; }
        //! Returns the size of the FFT used for spectral analysis.
        size_t	getFftSize() const				{ return mFftSize; }
        //! Returns the corresponding frequency for \a bin. Computed as \code bin * getSampleRate() / getFftSize() \endcode
        float	getFreqForBin( size_t bin );
        //! Returns the factor (0 - 1, default = 0.5) used when smoothing the magnitude spectrum between sequential calls to getMagSpectrum().
        float	getSmoothingFactor() const		{ return mSmoothingFactor; }
        //! Sets the factor (0 - 1, default = 0.5) used when smoothing the magnitude spectrum between sequential calls to getMagSpectrum()
        void	setSmoothingFactor( float factor );
        
    protected:
        void initialize() override;
        
    private:
        std::unique_ptr<dsp::Fft>	mFft;
        Buffer						mFftBuffer;			// windowed samples before transform
        BufferSpectral				mBufferSpectral;	// transformed samples
        std::vector<float>			mMagSpectrum;		// computed magnitude spectrum from frequency-domain samples
        AlignedArrayPtr				mWindowingTable;
        size_t						mFftSize;
        dsp::WindowType				mWindowType;
        float						mSmoothingFactor;
    };
    
} } // namespace cinder::audio
