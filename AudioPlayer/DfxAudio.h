#pragma once
#include "SampleUtil.h"
#include <thread>
#include <mutex>
#include <vector>

namespace dfx
{
    enum class IoMode
    {
        Out,
        In,
        Duplex
    };

#if 0

    enum class StreamMode
    {
        OUTPUT,
        INPUT,
        DUPLEX,
        UNINITIALIZED = -75
    };

#endif

    enum class StreamState
    {
        STOPPED,
        STOPPING,
        RUNNING,
        CLOSED = -50
    };

    // We do the old-fashioned enum this way to get around
    // guideline warnings (talked about messed up):

    using StreamIOStatus = unsigned;

    static constexpr StreamIOStatus StreamIO_Good = 0x0;
    static constexpr StreamIOStatus StreamIO_Input_Overflow = 0x1;
    static constexpr StreamIOStatus StreamIO_Output_Underflow = 0x2;

    // ///////////////////////////////////////////////////////////////////
    // Set these for your desired system-wide format for working samples

    using system_t = double;
    static constexpr auto system_fmt = SampleFormat::FLOAT64;
   
    // ///////////////////////////////////////////////////////////////////

    typedef int (*CallbackPtr)(void* outBuff, void* inBuff, unsigned nFrames, double streamTime, StreamIOStatus ioStatus, void* userData);

    // For buffer conversion

    struct ConvertInfo
    {
        int nChannels{};
        int inJump{};
        int outJump{};
        SampleFormat inFormat{};
        SampleFormat outFormat{};
        std::vector<int> inOffset{};
        std::vector<int> outOffset{};

        ConvertInfo() = default;
    };

    struct CallbackInfo
    {
        std::thread thread{};
        void* object{};    // Used as a "this" pointer.
        void* callback{};
        void* userData{};
        void* errorCallback{};
        void* apiInfo{};   // void pointer for API specific callback information
        bool isRunning{};
        bool doRealtime{};
        int priority{};

        // Default constructor.
        CallbackInfo() = default;
    };

    // /////////////////////////////////////////////////////////////////////////////

    static constexpr unsigned int MAX_SAMPLE_RATES = 7;

    static constexpr unsigned int SAMPLE_RATES[MAX_SAMPLE_RATES] = {
        // Why bother? 4000, 5512, 8000, 9600, 11025, 16000, 
        22050,
        // Why bother? 32000, 
        44100, 48000, 88200, 96000, 176400, 192000
    };

    // This information is shielded from the internals of any particular
    // audio platform (asio, jack, core). Some of it is duplicated from
    // the more specific driver information.

    class DeviceInfo {
    public:

        std::vector<unsigned> supportedSampleRates;
        std::vector<std::string> inputNames;
        std::vector<std::string> outputNames;
        std::string name;
        long devID;
        long nOutChannels;
        long nInChannels;
        long nDuplexChannels;
        bool isDefaultOutput;
        bool isDefaultInput;
        unsigned preferredSampleRate;
        SampleFormat format;
        bool little_endian; // meaning device is little_endian
        bool valid;

    public:

        DeviceInfo();

        // Just getting experience with move semantics.
        // Not necessarily needed.

        DeviceInfo(DeviceInfo&& other) noexcept;

        bool IsCompatibleChannelRange(IoMode mode, long nChannels, long startChannel = 0);
        bool IsCompatibleSampleRate(unsigned sampleRate);
        bool IsCompatibleFormat(SampleFormat fmt);

    };

    // /////////////////////////////////////////////////////////////////
    //
    // Platform independent
    //
    // /////////////////////////////////////////////////////////////////

    class DfxStream {
    public:

        CallbackInfo callbackInfo{};

        ConvertInfo convertPlayInfo{};
        ConvertInfo convertRecInfo{};

        void* apiHandle{};           // void pointer for API specific stream handle information
        //StreamMode mode{};           // OUTPUT, INPUT, or DUPLEX.
        StreamState state{};         // STOPPED, RUNNING, or CLOSED

        unsigned int devPlayID{};
        unsigned int devRecID{};

        char* userPlayBuffer{};
        char* userRecBuffer{};

        char* devPlayBuffer{};
        char* devRecBuffer{};

        bool doConvertPlayBuffer{};
        bool doConvertRecBuffer{};

        bool userInterleaved{};

        bool devPlayInterleaved{};
        bool devRecInterleaved{};

        bool swapPlayBytes{};
        bool swapRecBytes{};

        unsigned sampleRate{};
        unsigned bufferSize{};  // nominal
        unsigned nBuffers{};

        unsigned nUserPlayChannels{};
        unsigned nUserRecChannels{};

        unsigned nDevPlayChannels{};
        unsigned nDevRecChannels{};

        unsigned playChannelOffset{};
        unsigned recChannelOffset{};

        unsigned playLatency{};
        unsigned recLatency{};

        SampleFormat userFormat{};

        SampleFormat devPlayFormat{};
        SampleFormat devRecFormat{};

        std::mutex mutex{};

        double streamTime{};         // Number of elapsed seconds since the stream started.

#if defined(HAVE_GETTIMEOFDAY)
        struct timeval lastTickTimestamp {};
#endif

        DfxStream();
        virtual ~DfxStream();

        //////
        // Platform independent

        bool FinishBufferConfig(); 

        void cfgRecConvertInfo(unsigned int firstChannel);
        void cfgPlayConvertInfo(unsigned int firstChannel);

        // ////////////////////

    };

    extern void convertBuffer(DfxStream& stream, char* outBuffer, char* inBuffer, ConvertInfo& info);

    // This way does interleave/de-interleave
    extern void convertBufferX(DfxStream& stream, char* outBuffer, char* inBuffer, ConvertInfo& info);


    // /////////////////////////////////////////////////////////////////
    //
    // 
    //
    // /////////////////////////////////////////////////////////////////

    class DfxAudio {
    public:

        DfxStream stream;
        DeviceInfo devInfo;

    public:

        DfxAudio();
        virtual ~DfxAudio();

    public:

        virtual bool LoadDriver(const std::string& name) = 0;
        virtual bool InitDriver(bool verbose = true) = 0;
        virtual void UnloadDriver() = 0;

        virtual long NumDevices() = 0;
        virtual std::vector<std::string> DeviceNames() = 0;
        virtual std::string DeviceName(long devId) = 0;
        virtual long QueryDeviceID() = 0;

        virtual int LastError() = 0;

    public:

        virtual void verifyStream();

        virtual void closeStream(); // should be overidden
        virtual void startStream() = 0;
        virtual void stopStream() = 0;
        virtual void abortStream();

        double getStreamTime();
        void setStreamTime(double time);
        void tickStreamTime();

    public:

        virtual bool Start(); // should be overidden
        virtual bool Stop();  // should be overidden
        virtual bool Stopped() = 0;
        virtual bool Exit() = 0;

        virtual void ConfigureUserCallback(CallbackPtr userCallback) = 0;

        virtual bool Open(long nInputChannels_, long nOutputChannels_, long bufferSize_, unsigned sampleRate_, void* userData_, bool verbose) = 0;

    };


} // end of namespace
