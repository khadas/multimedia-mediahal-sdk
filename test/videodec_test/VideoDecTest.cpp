/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#define LOG_TAG "VideoDecTest"
#include <utils/Log.h>

#include <atomic>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <utils/Log.h>
#include <sys/time.h>

#include <AmVideoDecBase.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <media/ICrypto.h>
#include <media/DataSource.h>
#include <media/IMediaHTTPService.h>
#include <media/MediaSource.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AUtils.h>
//#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSourceFactory.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaExtractorFactory.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <hardware/gralloc1.h>

#include <gui/GLConsumer.h>
#include <gui/IProducerListener.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <dlfcn.h>

#include <android-base/logging.h>

#define UNUSED(x) (void)x

using namespace android;
using namespace std::chrono_literals;

using ::android::hardware::graphics::allocator::V2_0::IAllocator;
using ::android::hardware::graphics::mapper::V2_0::BufferDescriptor;
using ::android::hardware::graphics::mapper::V2_0::IMapper;

using ::android::Fence;
using ::android::GraphicBuffer;
using ::android::IGraphicBufferProducer;
using ::android::sp;
using ::android::status_t;
using ::android::wp;

class VideoDecSession;

static uint32_t kDefaultQueueCount = 256;
static const int64_t kDequeueTimeoutNs = 5 * 1000 * 1000;

class playerCallback;

AmVideoDecBase* getAmVideoDec(AmVideoDecCallback* callback) {
    void *libHandle = dlopen("libmediahal_videodec.so", RTLD_NOW);

    if (libHandle == NULL) {
        libHandle = dlopen("libmediahal_videodec.system.so", RTLD_NOW);
        if (libHandle == NULL) {
                ALOGE("unable to dlopen libmediahal_videodec.so: %s", dlerror());
                printf("unable to dlopen libmediahal_videodec.so: %s", dlerror());
                return nullptr;
        }
    }

    typedef AmVideoDecBase *(*createAmVideoDecFunc)(AmVideoDecCallback* callback);

    createAmVideoDecFunc getAmVideoDec =
        (createAmVideoDecFunc)dlsym(libHandle, "AmVideoDec_create");

    if (getAmVideoDec == NULL) {
        dlclose(libHandle);
        libHandle = NULL;
        ALOGE("can not create AmVideoDec\n");
        printf("can not create AmVideoDec\n");
        return nullptr;
    }
    AmVideoDecBase* halHanle = (*getAmVideoDec)(callback);
    ALOGI("getAmVideoDec ok\n");
    printf("getAmVideoDec ok\n");
    return halHanle;
}

class VideoDecSession {
public:
    VideoDecSession();
    virtual ~VideoDecSession();

    void setOutputToSurface(int x, int y, int w, int h);
    void play(const char* iname);
    void playResetReplay(const char* iname, const char* iname2);
    void playFlushReplay(const char* iname, const char* iname2);
    void directFlush(const char* iname);
    void playCal(const char* iname, uint32_t expect_output_count);

    /* Implement callback */
    virtual void onOutputFormatChanged(uint32_t requestedNumOfBuffers,
                int32_t width, uint32_t height);
    virtual void onOutputBufferDone(int32_t pictureBufferId, int32_t bitstreamId,
                uint32_t width, uint32_t height);
    virtual void onInputBufferDone(int32_t bitstream_buffer_id);
    virtual void onUpdateDecInfo(const uint8_t* info, uint32_t isize);
    virtual void onFlushDone();
    virtual void onResetDone();
    virtual void onError(int32_t error);

    class playerCallback : public AmVideoDecCallback {
        public:
            playerCallback(VideoDecSession *thiz) : mThis(thiz) {}
            virtual ~playerCallback() = default;

            virtual void onOutputFormatChanged(uint32_t requestedNumOfBuffers,
                        int32_t width, uint32_t height) override {
                mThis->onOutputFormatChanged(requestedNumOfBuffers, width, height);
            }
            virtual void onOutputBufferDone(int32_t pictureBufferId, int32_t bitstreamId,
                        uint32_t width, uint32_t height) override {
                mThis->onOutputBufferDone(pictureBufferId, bitstreamId, width, height);
            }
            virtual void onInputBufferDone(int32_t bitstream_buffer_id) override {
                mThis->onInputBufferDone(bitstream_buffer_id);
            }
            virtual void onUserdataReady(const uint8_t* userdata, uint32_t usize) override {
                UNUSED(userdata);
                UNUSED(usize);
            }
            virtual void onUpdateDecInfo(const uint8_t* info, uint32_t isize) override {
                mThis->onUpdateDecInfo(info, isize);
            }
            virtual void onFlushDone() override {
                mThis->onFlushDone();
            }
            virtual void onResetDone() override {
                mThis->onResetDone();
            }
            virtual void onError(int32_t error) override {
                mThis->onError(error);
            }
            virtual void onEvent(uint32_t event, void* param, uint32_t paramsize) override {
                UNUSED(event);
                UNUSED(param);
                UNUSED(paramsize);
            }
        private:
            VideoDecSession * const mThis;
    };

    playerCallback* mCallback;
    uint32_t mOutputBufferNum;

    /* Display */
    sp<IProducerListener> mProducerListener;
    sp<Surface> mSurface;
    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mControl;

    std::mutex mFlushedLock;
    std::condition_variable mFlushedCondition;
    std::mutex mResettedLock;
    std::condition_variable mResettedCondition;

struct dispWork {
    int32_t bitstreamId;
    int64_t timestamp;
    int32_t pictureBufferId;
    uint32_t width;
    uint32_t height;
};

private:

    status_t getSourceFromFile(const char* name,
                sp<IMediaSource>* source, std::string* mime);
    status_t sourceRead(sp<IMediaSource> source,
                    sp<ABuffer>* buf, int64_t* timestamp);
    void displayOnce(void);

    AmVideoDecBase* mMedia;
    /* Lock for mInputWork & mDisplayWork */
    std::mutex mInputLock;
    std::mutex mOutputLock;
    /* Map: bitstreamId -> timestamp */
    std::map<int32_t, uint64_t> mInputWork;
    /* Map: picturebufferId -> slot */
    std::map<int32_t, int32_t> mOutputBufferSlot;
    /* Vector: queued slot */
    std::vector<int32_t> mQueuedSlot;
    /* Queue: bistreamId -> struct dispwork */
    std::queue<dispWork> mDisplayWork;

    /* store input buffer */
    std::map<int32_t, android::sp<ABuffer>> mInputBuffer;
    /* store output buffer */
    std::map<int32_t, android::sp<GraphicBuffer>> mSlotGraphicBuffers;
    /* output buffer mmap addr */
    std::vector<void*>mMmapAddr;

    bool mAllocFromIGBP;
    sp<IGraphicBufferProducer> mIGBP;
    uint32_t mDqWidth;
    uint32_t mDqHeight;

    uint32_t mInputDoneCount;
    uint32_t mOutputDoneCount;
};

uint64_t getTimeUs(void){
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_sec * 1e6 + time.tv_usec;
}

VideoDecSession::VideoDecSession() {
    mInputDoneCount = 0;
    mOutputDoneCount = 0;
}

VideoDecSession::~VideoDecSession() {
    if (mAllocFromIGBP) {
        ALOGI("~VideoDecPlayer free graphic buffer\n");
        mSlotGraphicBuffers.clear();
        mComposerClient->dispose();
    } else {
        ALOGI("~VideoDecPlayer: free ion buffer\n");
    }

    for (const auto& vaddr : mMmapAddr)
        mMedia->freeIonBuffer(vaddr);
}

void VideoDecSession::onOutputFormatChanged(uint32_t requestedNumOfBuffers,
                int32_t width, uint32_t height) {

    mOutputBufferNum = requestedNumOfBuffers;
    mDqWidth = width;
    mDqHeight = height;
    uint32_t imagesize = (width * height * 3) / 2;
    mMedia->setupOutputBufferNum(mOutputBufferNum);
    if (mAllocFromIGBP) {
        sp<IMapper> mMapper = IMapper::getService();
        sp<IAllocator> mAllocator2 = IAllocator::getService();
        mIGBP->setMaxDequeuedBufferCount(mOutputBufferNum);
        ALOGI("setMaxDequeuedBufferCount %d\n", mOutputBufferNum);
        mIGBP->setDequeueTimeout(kDequeueTimeoutNs);
        uint32_t format = HAL_PIXEL_FORMAT_YCrCb_420_SP;//HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
        uint64_t usage = (uint64_t)GRALLOC_USAGE_SW_READ_OFTEN |
                                      GRALLOC_USAGE_SW_WRITE_OFTEN |
                                      GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER |
                                      GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
        /*uint64_t usage = 0x400933;*/
        mIGBP->allowAllocation(true);
        for (uint32_t i = 0; i < mOutputBufferNum; i++) {
            int slot = i;
            sp<Fence> fence = new Fence();
            status_t status;
            status = mIGBP->dequeueBuffer(&slot, &fence, width, height,
                        format, usage, nullptr, nullptr);
            EXPECT_EQ(status, android::BnGraphicBufferProducer::BUFFER_NEEDS_REALLOCATION);

            sp<GraphicBuffer> slotBuffer = new GraphicBuffer();
            status = mIGBP->requestBuffer(slot, &slotBuffer);
            EXPECT_EQ(status, android::NO_ERROR);

            mOutputBufferSlot[i] = slot;
            mSlotGraphicBuffers[slot] = slotBuffer;
            uint32_t dmabufFd = slotBuffer->handle->data[0];
            ALOGI("slotBuffer->handle %p, fd %d\n", slotBuffer->handle, slotBuffer->handle->data[0]);

            mMedia->createOutputBuffer(i, dmabufFd);
        }
        mIGBP->allowAllocation(false);
    } else {
        uint8_t* vaddr;
        for (uint32_t i = 0; i < mOutputBufferNum; i++) {
            mMedia->allocIonBuffer(imagesize, (void**)&vaddr);
            mMmapAddr.push_back((void*)vaddr);
            ALOGI("allocIonBuffer vaddr %p, size %d\n", vaddr, imagesize);
            mMedia->createOutputBuffer(i, vaddr, imagesize);
        }
    }
    ALOGI("onOutputFormatChanged ok this %p>>>>>\n", this);
}

void VideoDecSession::onOutputBufferDone(int32_t pictureBufferId, int32_t bitstreamId,
                uint32_t width, uint32_t height) {
    ALOGI("onOutputBufferDone this %p, pictureBufferId %d, bitstreamId %d, mDisplayWork.size %d, mInputWork.size %d\n",
                this, pictureBufferId, bitstreamId, mDisplayWork.size(), mInputWork.size());
    std::lock_guard<std::mutex> lock(mOutputLock);
    int64_t timestamp = mInputWork[bitstreamId];
    mDisplayWork.push({bitstreamId, timestamp, pictureBufferId, width, height});
    mInputWork.erase(bitstreamId);
    mOutputDoneCount++;
}

void VideoDecSession::onInputBufferDone(int32_t bitstreamId) {
    ALOGI("onInputBufferDone this %p bitstream_buffer_id %d, mInputBuffer.size() %d\n", this, bitstreamId, mInputBuffer.size());
    std::lock_guard<std::mutex> lock(mInputLock);
    mInputBuffer.erase(bitstreamId);
    mInputDoneCount++;
}

void VideoDecSession::onUpdateDecInfo(const uint8_t* info, uint32_t isize) {
    ALOGI("onUpdateDecInfo info %p size %d\n", info, isize);
}

void VideoDecSession::onFlushDone() {
    ALOGI("onFlushDone\n");
    std::unique_lock <std::mutex> l(mFlushedLock);
    mFlushedCondition.notify_all();
}

void VideoDecSession::onResetDone() {
    ALOGI("onResetDone\n");
    std::unique_lock <std::mutex> l(mResettedLock);
    mResettedCondition.notify_all();
}

void VideoDecSession::onError(int32_t error) {
    ALOGE("onError error %d\n", error);
    printf("onError error %d\n", error);
}

status_t VideoDecSession::getSourceFromFile(const char* name,
                sp<IMediaSource>* source, std::string* mime) {

    source->clear();

    sp<DataSource> dataSource = DataSourceFactory::CreateFromURI(nullptr , name);
    CHECK(dataSource);

    sp<IMediaExtractor> extractor = MediaExtractorFactory::Create(dataSource);
    CHECK(extractor);

    sp<MetaData> meta = extractor->getMetaData();

    const char* mimetype;
    if (meta != nullptr) {
        if (!meta->findCString(kKeyMIMEType, &mimetype)) {
            printf("extractor did not provide MIME type.\n");
            return BAD_VALUE;
        }
    }

    size_t numTracks = extractor->countTracks();

    size_t i;
    for (i = 0; i < numTracks; ++i) {
        meta = extractor->getTrackMetaData(i, 0);
        CHECK(meta);

        meta->findCString(kKeyMIMEType, &mimetype);
        if (!strncasecmp(mimetype, "video/", 6)) {
            ALOGI("video mime %s\n", mimetype);
            break;
        }
        meta = nullptr;
    }

    if (meta == nullptr) {
        printf("No video track found.\n");
        return BAD_VALUE;
    }

    *source = extractor->getTrack(i);
    if (*source == nullptr) {
        printf("skip NULL track %zu, total tracks %zu.\n", i, numTracks);
        return BAD_VALUE;
    }
    mime->append(mimetype);
    return NO_ERROR;
}

void VideoDecSession::displayOnce() {
    int32_t bitstreamId = 0;
    int64_t timestamp = 0;
    int32_t pictureBufferId = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t slot = -1;
    status_t err = 0;
    static uint64_t starttime = 0;
    static uint64_t firstpts = 0;

    if (mAllocFromIGBP && mQueuedSlot.size() > 0) {
        sp<Fence> dequeuedFence = new Fence();
        uint64_t usage = (uint64_t)GRALLOC_USAGE_SW_READ_OFTEN |
                              GRALLOC_USAGE_SW_WRITE_OFTEN |
                              GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER |
                              GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
        int queuesize = mQueuedSlot.size();
        int dqslot;
        for (int i = 2; i < queuesize; i++) {
            err = mIGBP->dequeueBuffer(&dqslot,
                            &dequeuedFence,
                            mDqWidth,
                            mDqHeight,
                            HAL_PIXEL_FORMAT_YCrCb_420_SP/*HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED*/,
                            usage,
                            nullptr,
                            nullptr);
            if (!err) {
                for (auto it = mQueuedSlot.begin(); it < mQueuedSlot.end(); it++) {
                    if (dqslot == *it) {
                        mQueuedSlot.erase(it);
                        break;
                    }
                }
                printf("queueOutputBuffer dqslot %d\n", dqslot);
                mMedia->queueOutputBuffer(dqslot);
            } else
                break;
        }
    }
    {
    if (mDisplayWork.empty()) {
        usleep(10000);
        return;
    }
    std::lock_guard<std::mutex> lock(mOutputLock);
    dispWork* work = &mDisplayWork.front();
    bitstreamId = work->bitstreamId;
    timestamp = work->timestamp;
    pictureBufferId = work->pictureBufferId;
    width = work->width;
    height = work->height;
    slot = mOutputBufferSlot[pictureBufferId];
    }
    if (mAllocFromIGBP) {
        if (!starttime) /* render first frame */
            starttime = android::ALooper::GetNowUs() + 40000;
        if (!firstpts) /* render first frame */
            firstpts = timestamp;
        //uint64_t rendertime = ALooper::GetNowUs()*1000LL;
        uint64_t rendertime = (timestamp - firstpts + starttime)*1000LL;
        //printf("this [%p] render frame [%d] timestampMs %0.3f S, playtimeMs %0.3f S\n",
        //        this, bitstreamId, (timestamp - firstpts)/1E6,
        //        (ALooper::GetNowUs() + 40000 - starttime)/1E6);
        IGraphicBufferProducer::QueueBufferInput qbi(rendertime,
                false,
                HAL_DATASPACE_UNKNOWN,
                Rect((uint32_t)width, (uint32_t)height),
                NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW,
                0,
                Fence::NO_FENCE,
                0);
        IGraphicBufferProducer::QueueBufferOutput qbo;

        err = mIGBP->queueBuffer(slot, qbi, &qbo);
        if (err) {
            ALOGI("igpb queueBuffer err %d, [%d x %d] passthrough\n", err, width, height);
            printf("igpb queueBuffer err %d, [%d x %d] passthrough\n", err, width, height);
            //continue;
        }
        ALOGI("displayThread pictureBufferId %d, bitstreamId %d,"
                "width %d, height %d, timestamp %lld, slot %d\n",
                pictureBufferId, bitstreamId, width, height, timestamp, slot);
    } else {
        //printf("ION DROP FRAME [%d] playtimeMs %0.3f S\n", bitstreamId, (ALooper::GetNowUs() + 40000 - starttime)/1E6);
        mMedia->queueOutputBuffer(pictureBufferId);
    }
    mDisplayWork.pop();
    mQueuedSlot.push_back(slot);
}

status_t VideoDecSession::sourceRead(sp<IMediaSource> source,
                sp<ABuffer>* buf, int64_t* timestamp) {
    MediaBufferBase *buffer = nullptr;

    status_t err = source->read(&buffer);
    if (err != OK) {
        assert(buffer == nullptr);
        if (err == INFO_FORMAT_CHANGED) {
            ALOGI("source read INFO_FORMAT_CHANGED\n");
            return err;
        }
        ALOGI("source read error %d\n", err);
        return err;
    }

    MetaDataBase& meta = buffer->meta_data();
    int64_t time;
    meta.findInt64(kKeyTime, &time);

    sp<ABuffer> newbuf = ABuffer::CreateAsCopy(buffer->data(), buffer->size());
    newbuf->setRange(0, buffer->size());

    *buf = newbuf;
    *timestamp = time;
    ALOGI("data size %d, data %p, timestamp %lld\n", buffer->size(), buffer->data(), time);
    buffer->release();
    return 0;
}

void VideoDecSession::setOutputToSurface(int x, int y, int w, int h) {
    mComposerClient = new SurfaceComposerClient;
    CHECK_EQ(mComposerClient->initCheck(), OK);

    mProducerListener = new DummyProducerListener;

    mControl = mComposerClient->createSurface(
            String8("testSurface"),
            w,
            h,
            HAL_PIXEL_FORMAT_YCrCb_420_SP/*HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED*/);

    CHECK(mControl);
    CHECK(mControl->isValid());

    SurfaceComposerClient::Transaction{}
            .setLayer(mControl, 0)
            .show(mControl)
            .setPosition(mControl, x, y)
            .apply();

    mSurface = mControl->getSurface();
    CHECK(mSurface);
    mSurface->connect(NATIVE_WINDOW_API_CPU, mProducerListener);

    mAllocFromIGBP = true;
    mIGBP = mSurface->getIGraphicBufferProducer();
    CHECK(mIGBP);
}

void VideoDecSession::play(const char* iname) {
    sp<IMediaSource> source;

    std::string mime;
    status_t err = getSourceFromFile(iname, &source, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format;
    convertMetaDataToMessage(source->getFormat(), &format);

    err = source->start();
    if (err != OK) {
        printf("source returned error %d (0x%08x)\n", err, err);
        return ;
    }

    mCallback = new playerCallback(this);
    mMedia = getAmVideoDec(mCallback);

    mMedia->initialize(mime.c_str(), NULL, 0, false, true);
    mMedia->setQueueCount(kDefaultQueueCount);

    std::atomic_bool running(true);
    std::thread displayThread([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    int32_t bitstreamId = 0;
    int64_t timestamp = 0u;
    sp<ABuffer> csd0, csd1;
    format->findBuffer("csd-0", &csd0);
    format->findBuffer("csd-1", &csd1);

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {
        sp<ABuffer> buf;
        status_t err = sourceRead(source, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }
        mInputBuffer[bitstreamId] = buf;
        ALOGI("queueInputBuffer bitstreamId %d, size %zd\n",
                        bitstreamId,  buf->size());

        int retry = 1000;
        do {
            err = mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
            if (err == -EAGAIN)
                usleep(20000);
            else
                break;
        } while(err || retry-- > 0);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }
    mMedia->flush();

    std::unique_lock <std::mutex> l(mFlushedLock);
    mFlushedCondition.wait_for(l, 2000ms);
    running.store(false);
    displayThread.join();
    mMedia->destroy();
    source->stop();
    delete mMedia;
    return;
}

void VideoDecSession::playResetReplay(const char* iname, const char* iname2) {
    sp<IMediaSource> source;

    std::string mime;
    status_t err = getSourceFromFile(iname, &source, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format;
    convertMetaDataToMessage(source->getFormat(), &format);

    err = source->start();
    if (err != OK) {
        printf("source returned error %d (0x%08x)\n", err, err);
        return ;
    }

    mCallback = new playerCallback(this);
    mMedia = getAmVideoDec(mCallback);

    mMedia->initialize(mime.c_str(), NULL, 0, false, true);

    std::atomic_bool running(true);
    std::thread displayThread([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    int32_t bitstreamId = 0;
    int64_t timestamp = 0u;
    sp<ABuffer> csd0, csd1;
    format->findBuffer("csd-0", &csd0);
    format->findBuffer("csd-1", &csd1);

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {
        sp<ABuffer> buf;
        status_t err = sourceRead(source, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }

        mInputBuffer[bitstreamId] = buf;

        //printf("queueInputBuffer bitstreamId %d\n", bitstreamId);
        mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }

    /* seek */
    mMedia->reset();

    std::unique_lock <std::mutex> l(mResettedLock);
    mResettedCondition.wait_for(l, 2000ms);

    running.store(false);
    displayThread.join();
    source->stop();

    /* replay from beginning without inject csd */
    sp<IMediaSource> source2;

    err = getSourceFromFile(iname2, &source2, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format2;
    convertMetaDataToMessage(source2->getFormat(), &format2);

    err = source2->start();
    if (err != OK) {
        printf("source2 returned error %d (0x%08x)\n", err, err);
        return ;
    }

    printf("start displayThread2\n");
    running.store(true);
    std::thread displayThread2([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {

        sp<ABuffer> buf;
        status_t err = sourceRead(source2, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }

        mInputBuffer[bitstreamId] = buf;

        mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }

    mMedia->flush();

    std::unique_lock <std::mutex> f(mFlushedLock);
    mFlushedCondition.wait_for(f, 2000ms);

    running.store(false);
    displayThread2.join();
    mMedia->destroy();
    source2->stop();
    delete mMedia;
    return;
}


void VideoDecSession::playFlushReplay(const char* iname, const char* iname2) {
    sp<IMediaSource> source;

    std::string mime;
    status_t err = getSourceFromFile(iname, &source, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format;
    convertMetaDataToMessage(source->getFormat(), &format);

    err = source->start();
    if (err != OK) {
        printf("source returned error %d (0x%08x)\n", err, err);
        return ;
    }

    mCallback = new playerCallback(this);
    mMedia = getAmVideoDec(mCallback);

    mMedia->initialize(mime.c_str(), NULL, 0, false, true);

    std::atomic_bool running(true);
    std::thread displayThread([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    int32_t bitstreamId = 0;
    int64_t timestamp = 0u;
    sp<ABuffer> csd0, csd1;
    format->findBuffer("csd-0", &csd0);
    format->findBuffer("csd-1", &csd1);

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {
        sp<ABuffer> buf;
        status_t err = sourceRead(source, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }

        mInputBuffer[bitstreamId] = buf;

        //printf("queueInputBuffer bitstreamId %d\n", bitstreamId);
        mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }

    mMedia->flush();

    {
    std::unique_lock <std::mutex> r(mFlushedLock);
    EXPECT_NE(mFlushedCondition.wait_for(r, 2000ms), std::cv_status::timeout);
    }
    EXPECT_GE(mOutputDoneCount, 1u);

    mMedia->reset();

    {
    std::unique_lock <std::mutex> r(mResettedLock);
    EXPECT_NE(mResettedCondition.wait_for(r, 2000ms), std::cv_status::timeout);
    }

    running.store(false);
    displayThread.join();
    source->stop();
    mInputDoneCount = 0;
    mOutputDoneCount = 0;

    /* replay from beginning without inject csd */
    sp<IMediaSource> source2;

    err = getSourceFromFile(iname2, &source2, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format2;
    convertMetaDataToMessage(source2->getFormat(), &format2);

    err = source2->start();
    if (err != OK) {
        printf("source2 returned error %d (0x%08x)\n", err, err);
        return ;
    }

    printf("start displayThread2\n");
    running.store(true);
    std::thread displayThread2([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {

        sp<ABuffer> buf;
        status_t err = sourceRead(source2, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }

        mInputBuffer[bitstreamId] = buf;

        mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }

    mMedia->flush();

    {
    std::unique_lock <std::mutex> f(mFlushedLock);
    EXPECT_NE(mFlushedCondition.wait_for(f, 2000ms), std::cv_status::timeout);
    }
    EXPECT_GE(mOutputDoneCount, 1u);
    running.store(false);
    displayThread2.join();
    mMedia->destroy();
    source2->stop();
    delete mMedia;
    return;
}

void VideoDecSession::directFlush(const char* iname) {
    sp<IMediaSource> source;

    std::string mime;
    status_t err = getSourceFromFile(iname, &source, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format;
    convertMetaDataToMessage(source->getFormat(), &format);

    err = source->start();
    if (err != OK) {
        printf("source returned error %d (0x%08x)\n", err, err);
        return ;
    }

    mCallback = new playerCallback(this);
    mMedia = getAmVideoDec(mCallback);

    mMedia->initialize(mime.c_str(), NULL, 0, false, true);
    mMedia->setQueueCount(kDefaultQueueCount);

    std::atomic_bool running(true);
    std::thread displayThread([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });
    mMedia->flush();

    std::unique_lock <std::mutex> l(mFlushedLock);
    EXPECT_NE(mFlushedCondition.wait_for(l, 2000ms), std::cv_status::timeout);
    running.store(false);
    displayThread.join();
    mMedia->destroy();
    source->stop();
    delete mMedia;
    return;
}

void VideoDecSession::playCal(const char* iname, uint32_t expect_output_count) {
    sp<IMediaSource> source;

    std::string mime;
    status_t err = getSourceFromFile(iname, &source, &mime);
    if (err != OK) {
        printf("can not get source from file error %d (0x%08x)\n", err, err);
        return ;
    }

    sp<AMessage> format;
    convertMetaDataToMessage(source->getFormat(), &format);

    err = source->start();
    if (err != OK) {
        printf("source returned error %d (0x%08x)\n", err, err);
        return ;
    }

    mCallback = new playerCallback(this);
    mMedia = getAmVideoDec(mCallback);

    mMedia->initialize(mime.c_str(), NULL, 0, false, true);
    mMedia->setQueueCount(kDefaultQueueCount);

    std::atomic_bool running(true);
    std::thread displayThread([this, &running]() {
        while (running) {
            displayOnce();
        }
        ALOGI("display loop finished");
    });

    int32_t bitstreamId = 0;
    int64_t timestamp = 0u;
    sp<ABuffer> csd0, csd1;
    format->findBuffer("csd-0", &csd0);
    format->findBuffer("csd-1", &csd1);

    if (csd0 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd0->data(), csd0->size());
        newbuf->setRange(0, csd0->size());
        ALOGI("csd0 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;

        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    if (csd1 != nullptr) {
        sp<ABuffer> newbuf = ABuffer::CreateAsCopy(csd1->data(), csd1->size());
        newbuf->setRange(0, csd1->size());
        ALOGI("csd1 size %d, data %p\n", newbuf->size(), (void*)newbuf->data());
        mInputBuffer[bitstreamId] = newbuf;
        mMedia->queueInputBuffer(bitstreamId, newbuf->data(), 0, newbuf->size(), timestamp);
        bitstreamId++;
    }

    while (1) {
        sp<ABuffer> buf;
        status_t err = sourceRead(source, &buf, &timestamp);
        if (err) {
            ALOGE("source read fail err %d\n", err);
            break;
        }
        mInputBuffer[bitstreamId] = buf;
        ALOGI("queueInputBuffer bitstreamId %d, size %zd\n",
                        bitstreamId,  buf->size());

        int retry = 1000;
        do {
            err = mMedia->queueInputBuffer(bitstreamId, buf->data(), 0, buf->size(), timestamp);
            if (err == -EAGAIN)
                usleep(20000);
            else
                break;
        } while(err || retry-- > 0);
        std::lock_guard<std::mutex> lock(mInputLock);
        mInputWork[bitstreamId] = timestamp;
        bitstreamId++;
    }
    mMedia->flush();

    std::unique_lock <std::mutex> l(mFlushedLock);
    mFlushedCondition.wait_for(l, 2000ms);
    running.store(false);
    displayThread.join();
    mMedia->destroy();
    source->stop();
    delete mMedia;
    EXPECT_EQ(mOutputBufferNum, expect_output_count);
    return;
}

class VideoDecTest : public testing::Test {
protected:
    VideoDecSession* s;
    AmVideoDecBase* h;

    virtual void SetUp(){
        s = new VideoDecSession();
        EXPECT_NE(nullptr, s);
        h = getAmVideoDec(new VideoDecSession::playerCallback(s));
        h->setQueueCount(kDefaultQueueCount);
        EXPECT_NE(nullptr, h);
    }
    virtual void TearDown(){
        h->destroy();
        delete h;
        delete s;
    }
};

class VideoDecPlaybackTest : public testing::Test {
protected:
    VideoDecSession* s;
    AmVideoDecBase* h;
    uint32_t screenWidth;
    uint32_t screenHeight;

    virtual void SetUp(){
        s = new VideoDecSession();
        EXPECT_NE(nullptr, s);
        h = getAmVideoDec(new VideoDecSession::playerCallback(s));
        h->setQueueCount(kDefaultQueueCount);
        EXPECT_NE(nullptr, h);
        DisplayInfo info;
#if ANDROID_PLATFORM_SDK_VERSION >= 29
        SurfaceComposerClient::getDisplayInfo(SurfaceComposerClient::getInternalDisplayToken(), &info);
#else
        sp<IBinder> display(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
        SurfaceComposerClient::getDisplayInfo(display, &info);
#endif
        screenWidth = info.w;
        screenHeight = info.h;
        s->setOutputToSurface(0, 0, this->screenWidth, this->screenHeight);

    }
    virtual void TearDown(){
        h->destroy();
        delete h;
        delete s;
    }
};

TEST_F(VideoDecPlaybackTest, h264_720p)
{
    s->play("/data/h264_720p.mp4");
}

TEST_F(VideoDecPlaybackTest, h264_4k)
{
    s->play("/data/4kcity20.mp4");
}

TEST_F(VideoDecPlaybackTest, h264_4k_oneframe)
{
    s->play("/data/4kcity_oneframe.es");
}

TEST_F(VideoDecPlaybackTest, h265_1080p)
{
    s->play("/data/h265_1080p.mp4");
}

TEST_F(VideoDecPlaybackTest, vp9_1080p)
{
    s->play("/data/vp9.webm");
}

TEST_F(VideoDecPlaybackTest, h264_b1)
{
    s->play("/data/b1.mp4");
}

TEST_F(VideoDecTest, createhal)
{
}

TEST_F(VideoDecTest, initialize_destory)
{
    h->initialize(NULL, NULL, 0, false, true);
    h->destroy();
}

TEST_F(VideoDecTest, alloc_ion_buffer)
{
    void* mapaddr;
    size_t size = 1048576;
    EXPECT_EQ(OK, h->allocIonBuffer(size, &mapaddr));
    EXPECT_EQ(OK, h->freeIonBuffer(mapaddr));
}

TEST_F(VideoDecPlaybackTest, playH264_4kcity10_seek)
{
    s->playResetReplay("/data/4kcity10.mp4", "/data/4kcity10.mp4");
}

TEST_F(VideoDecPlaybackTest, playH264_720p_seek)
{
    s->playResetReplay("/data/h264_720p.mp4", "/data/h264_720p.mp4");
}

TEST_F(VideoDecPlaybackTest, h264_frame1_flush_frame1)
{
    s->playFlushReplay("/data/b1.mp4", "/data/b1.mp4");
}

TEST_F(VideoDecPlaybackTest, h264_direct_flush)
{
    s->directFlush("/data/b1.mp4");
}

TEST_F(VideoDecPlaybackTest, h264_bbb_cal)
{
    s->playCal("/data/bbb_short.mp4", 240);
}


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  android::base::InitLogging(argv, android::base::StderrLogger);
  return RUN_ALL_TESTS();
}
