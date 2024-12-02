// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// COPY OF QMOVIE BECAUSE QTBUG-131448

#include "movie.h"

#include "private/qobject_p.h"
#include "private/qproperty_p.h"
#include "qbuffer.h"
#include "qdir.h"
#include "qelapsedtimer.h"
#include "qimage.h"
#include "qimagereader.h"
#include "qlist.h"
#include "qloggingcategory.h"
#include "qmap.h"
#include "qpixmap.h"
#include "qrect.h"
#include "qtimer.h"

#define QTC_MOVIE_INVALID_DELAY -1

class QtcFrameInfo
{
public:
    QImage image;
    int delay;
    bool endMark;
    inline QtcFrameInfo(bool endMark)
        : image(QImage())
        , delay(QTC_MOVIE_INVALID_DELAY)
        , endMark(endMark)
    {}

    inline QtcFrameInfo()
        : image(QImage())
        , delay(QTC_MOVIE_INVALID_DELAY)
        , endMark(false)
    {}

    inline QtcFrameInfo(QImage &&image, int delay)
        : image(std::move(image))
        , delay(delay)
        , endMark(false)
    {}

    inline bool isValid()
    {
        return endMark || !(image.isNull() && (delay == QTC_MOVIE_INVALID_DELAY));
    }

    inline bool isEndMarker() { return endMark; }

    static inline QtcFrameInfo endMarker() { return QtcFrameInfo(true); }
};
Q_DECLARE_TYPEINFO(QtcFrameInfo, Q_RELOCATABLE_TYPE);

class QtcMoviePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QtcMovie)

public:
    QtcMoviePrivate(QtcMovie *qq);
    bool isDone();
    bool next();
    int speedAdjustedDelay(int delay) const;
    bool isValid() const;
    bool jumpToFrame(int frameNumber);
    int frameCount() const;
    bool jumpToNextFrame();
    QtcFrameInfo infoForFrame(int frameNumber);
    void reset();

    inline void enterState(QtcMovie::MovieState newState)
    {
        movieState = newState;
        emit q_func() -> stateChanged(newState);
    }

    // private slots
    void _q_loadNextFrame();
    void _q_loadNextFrame(bool starting);

    QImageReader *reader = nullptr;

    void setSpeed(int percentSpeed) { q_func()->setSpeed(percentSpeed); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QtcMoviePrivate, int, speed, &QtcMoviePrivate::setSpeed, 100)

    QtcMovie::MovieState movieState = QtcMovie::NotRunning;
    QRect frameRect;
    QImage currentImage;
    mutable std::optional<QPixmap> currentPixmap;
    int currentFrameNumber = -1;
    int nextFrameNumber = 0;
    int greatestFrameNumber = -1;
    int nextDelay = 0;
    int playCounter = -1;
    qint64 initialDevicePos = 0;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(
        QtcMoviePrivate, QtcMovie::CacheMode, cacheMode, QtcMovie::CacheNone)
    bool haveReadAll = false;
    bool isFirstIteration = true;
    QMap<int, QtcFrameInfo> frameMap;
    QString absoluteFilePath;

    QTimer *nextImageTimer;
};

/*! \internal
 */
QtcMoviePrivate::QtcMoviePrivate(QtcMovie *qq)
{
    q_ptr = qq;
}

/*! \internal
 */
void QtcMoviePrivate::reset()
{
    nextImageTimer->stop();
    if (reader->device())
        initialDevicePos = reader->device()->pos();
    currentFrameNumber = -1;
    nextFrameNumber = 0;
    greatestFrameNumber = -1;
    nextDelay = 0;
    playCounter = -1;
    haveReadAll = false;
    isFirstIteration = true;
    frameMap.clear();
}

/*! \internal
 */
bool QtcMoviePrivate::isDone()
{
    return (playCounter == 0);
}

/*!
    \internal

    Given the original \a delay, this function returns the
    actual number of milliseconds to delay according to
    the current speed. E.g. if the speed is 200%, the
    result will be half of the original delay.
*/
int QtcMoviePrivate::speedAdjustedDelay(int delay) const
{
    return int((qint64(delay) * qint64(100)) / qint64(speed));
}

/*!
    \internal

    Returns the QtcFrameInfo for the given \a frameNumber.

    If the frame number is invalid, an invalid QtcFrameInfo is
    returned.

    If the end of the animation has been reached, a
    special end marker QtcFrameInfo is returned.

*/
QtcFrameInfo QtcMoviePrivate::infoForFrame(int frameNumber)
{
    Q_Q(QtcMovie);

    if (frameNumber < 0)
        return QtcFrameInfo(); // Invalid

    if (haveReadAll && (frameNumber > greatestFrameNumber)) {
        if (frameNumber == greatestFrameNumber + 1)
            return QtcFrameInfo::endMarker();
        return QtcFrameInfo(); // Invalid
    }

    // For an animated image format, the tradition is that QtcMovie calls read()
    // until canRead() == false, because the number of frames may not be known
    // in advance; but if we're abusing a multi-frame format as an animation,
    // canRead() may remain true, and we need to stop after reading the maximum
    // number of frames that the image provides.
    const bool supportsAnimation = reader->supportsOption(QImageIOHandler::Animation);
    const int stopAtFrame = supportsAnimation ? -1 : frameCount();

    // For an animated image format, QImageIOHandler::nextImageDelay() should
    // provide the time to wait until showing the next frame; but multi-frame
    // formats are not expected to provide this value, so use 1000 ms by default.
    const auto nextFrameDelay = [&]() {
        return supportsAnimation ? reader->nextImageDelay() : 1000;
    };

    if (cacheMode == QtcMovie::CacheNone) {
        if (frameNumber != currentFrameNumber + 1) {
            // Non-sequential frame access
            if (!reader->jumpToImage(frameNumber)) {
                if (frameNumber == 0) {
                    // Special case: Attempt to "rewind" so we can loop
                    // ### This could be implemented as QImageReader::rewind()
                    if (reader->device()->isSequential())
                        return QtcFrameInfo(); // Invalid
                    QString fileName = reader->fileName();
                    QByteArray format = reader->format();
                    QIODevice *device = reader->device();
                    QColor bgColor = reader->backgroundColor();
                    QSize scaledSize = reader->scaledSize();
                    delete reader;
                    if (fileName.isEmpty())
                        reader = new QImageReader(device, format);
                    else
                        reader = new QImageReader(absoluteFilePath, format);
                    if (!reader->canRead()) // Provoke a device->open() call
                        emit q->error(reader->error());
                    reader->device()->seek(initialDevicePos);
                    reader->setBackgroundColor(bgColor);
                    reader->setScaledSize(scaledSize);
                } else {
                    return QtcFrameInfo(); // Invalid
                }
            }
        }
        //qCDebug(lcImageIo, "CacheNone: read frame %d of %d", frameNumber, stopAtFrame);
        if (stopAtFrame > 0 ? (frameNumber < stopAtFrame) : reader->canRead()) {
            // reader says we can read. Attempt to actually read image
            // But if it's a non-animated multi-frame format and we know the frame count, stop there.
            if (stopAtFrame > 0)
                reader->jumpToImage(frameNumber);
            QImage anImage = reader->read();
            if (anImage.isNull()) {
                // Reading image failed.
                return QtcFrameInfo(); // Invalid
            }
            if (frameNumber > greatestFrameNumber)
                greatestFrameNumber = frameNumber;
            return QtcFrameInfo(std::move(anImage), nextFrameDelay());
        } else if (frameNumber != 0) {
            // We've read all frames now. Return an end marker
            haveReadAll = true;
            return QtcFrameInfo::endMarker();
        } else {
            // No readable frames
            haveReadAll = true;
            return QtcFrameInfo();
        }
    }

    // CacheMode == CacheAll
    if (frameNumber > greatestFrameNumber) {
        // Frame hasn't been read from file yet. Try to do it
        for (int i = greatestFrameNumber + 1; i <= frameNumber; ++i) {
            //qCDebug(lcImageIo, "CacheAll: read frame %d of %d", frameNumber, stopAtFrame);
            if (stopAtFrame > 0 ? (frameNumber < stopAtFrame) : reader->canRead()) {
                // reader says we can read. Attempt to actually read image
                // But if it's a non-animated multi-frame format and we know the frame count, stop there.
                if (stopAtFrame > 0)
                    reader->jumpToImage(frameNumber);
                QImage anImage = reader->read();
                if (anImage.isNull()) {
                    // Reading image failed.
                    return QtcFrameInfo(); // Invalid
                }
                greatestFrameNumber = i;
                QtcFrameInfo info(std::move(anImage), nextFrameDelay());
                // Cache it!
                frameMap.insert(i, info);
                if (i == frameNumber) {
                    return info;
                }
            } else {
                // We've read all frames now. Return an end marker
                haveReadAll = true;
                return frameNumber == greatestFrameNumber + 1 ? QtcFrameInfo::endMarker()
                                                              : QtcFrameInfo();
            }
        }
    }
    // Return info for requested (cached) frame
    return frameMap.value(frameNumber);
}

/*!
    \internal

    Attempts to advance the animation to the next frame.
    If successful, currentFrameNumber, currentImage and
    nextDelay are updated accordingly, and true is returned.
    Otherwise, false is returned.
    When false is returned, isDone() can be called to
    determine whether the animation ended gracefully or
    an error occurred when reading the frame.
*/
bool QtcMoviePrivate::next()
{
    QElapsedTimer time;
    time.start();
    QtcFrameInfo info = infoForFrame(nextFrameNumber);
    if (!info.isValid())
        return false;
    if (info.isEndMarker()) {
        // We reached the end of the animation.
        if (isFirstIteration) {
            if (nextFrameNumber == 0) {
                // No frames could be read at all (error).
                return false;
            }
            // End of first iteration. Initialize play counter
            playCounter = reader->loopCount();
            isFirstIteration = false;
        }
        // Loop as appropriate
        if (playCounter != 0) {
            if (playCounter != -1) // Infinite?
                playCounter--;     // Nope
            nextFrameNumber = 0;
            return next();
        }
        // Loop no more. Done
        return false;
    }
    // Image and delay OK, update internal state
    currentFrameNumber = nextFrameNumber++;
    currentImage = info.image;
    currentPixmap.reset();

    if (!speed)
        return true;

    nextDelay = speedAdjustedDelay(info.delay);
    // Adjust delay according to the time it took to read the frame
    int processingTime = time.elapsed();
    if (processingTime > nextDelay)
        nextDelay = 0;
    else
        nextDelay = nextDelay - processingTime;
    return true;
}

/*! \internal
 */
void QtcMoviePrivate::_q_loadNextFrame()
{
    _q_loadNextFrame(false);
}

void QtcMoviePrivate::_q_loadNextFrame(bool starting)
{
    Q_Q(QtcMovie);
    if (next()) {
        if (starting && movieState == QtcMovie::NotRunning) {
            enterState(QtcMovie::Running);
            emit q->started();
        }

        if (frameRect.size() != currentImage.rect().size()) {
            frameRect = currentImage.rect();
            emit q->resized(frameRect.size());
        }

        emit q->updated(frameRect);
        emit q->frameChanged(currentFrameNumber);

        if (speed && movieState == QtcMovie::Running)
            nextImageTimer->start(nextDelay);
    } else {
        // Could not read another frame
        if (!isDone()) {
            emit q->error(reader->error());
        }

        // Graceful finish
        if (movieState != QtcMovie::Paused) {
            nextFrameNumber = 0;
            isFirstIteration = true;
            playCounter = -1;
            enterState(QtcMovie::NotRunning);
            emit q->finished();
        }
    }
}

/*!
    \internal
*/
bool QtcMoviePrivate::isValid() const
{
    Q_Q(const QtcMovie);

    if (greatestFrameNumber >= 0)
        return true; // have we seen valid data
    bool canRead = reader->canRead();
    if (!canRead) {
        // let the consumer know it's broken
        //
        // ### the const_cast here is ugly, but 'const' of this method is
        // technically wrong right now, since it may cause the underlying device
        // to open.
        emit const_cast<QtcMovie *>(q)->error(reader->error());
    }
    return canRead;
}

/*!
    \internal
*/
bool QtcMoviePrivate::jumpToFrame(int frameNumber)
{
    if (frameNumber < 0)
        return false;
    if (currentFrameNumber == frameNumber)
        return true;
    nextFrameNumber = frameNumber;
    if (movieState == QtcMovie::Running)
        nextImageTimer->stop();
    _q_loadNextFrame();
    return (nextFrameNumber == currentFrameNumber + 1);
}

/*!
    \internal
*/
int QtcMoviePrivate::frameCount() const
{
    int result;
    if ((result = reader->imageCount()) != 0)
        return result;
    if (haveReadAll)
        return greatestFrameNumber + 1;
    return 0; // Don't know
}

/*!
    \internal
*/
bool QtcMoviePrivate::jumpToNextFrame()
{
    return jumpToFrame(currentFrameNumber + 1);
}

static QTimer *createTimer(QObject *parent)
{
    auto timer = new QTimer(parent);
    timer->setSingleShot(true);
    return timer;
}

/*!
    Constructs a QtcMovie object, passing the \a parent object to QObject's
    constructor.

    \sa setFileName(), setDevice(), setFormat()
 */
QtcMovie::QtcMovie(QObject *parent)
    : QObject(*new QtcMoviePrivate(this), parent)
{
    Q_D(QtcMovie);
    d->nextImageTimer = createTimer(this);
    d->reader = new QImageReader;
    connect(d->nextImageTimer, SIGNAL(timeout()), this, SLOT(_q_loadNextFrame()));
}

/*!
    Constructs a QtcMovie object. QtcMovie will use read image data from \a
    device, which it assumes is open and readable. If \a format is not empty,
    QtcMovie will use the image format \a format for decoding the image
    data. Otherwise, QtcMovie will attempt to guess the format.

    The \a parent object is passed to QObject's constructor.
 */
QtcMovie::QtcMovie(QIODevice *device, const QByteArray &format, QObject *parent)
    : QObject(*new QtcMoviePrivate(this), parent)
{
    Q_D(QtcMovie);
    d->nextImageTimer = createTimer(this);
    d->reader = new QImageReader(device, format);
    d->initialDevicePos = device->pos();
    connect(d->nextImageTimer, SIGNAL(timeout()), this, SLOT(_q_loadNextFrame()));
}

/*!
    Constructs a QtcMovie object. QtcMovie will use read image data from \a
    fileName. If \a format is not empty, QtcMovie will use the image format \a
    format for decoding the image data. Otherwise, QtcMovie will attempt to
    guess the format.

    The \a parent object is passed to QObject's constructor.
 */
QtcMovie::QtcMovie(const QString &fileName, const QByteArray &format, QObject *parent)
    : QObject(*new QtcMoviePrivate(this), parent)
{
    Q_D(QtcMovie);
    d->nextImageTimer = createTimer(this);
    d->absoluteFilePath = QDir(fileName).absolutePath();
    d->reader = new QImageReader(fileName, format);
    if (d->reader->device())
        d->initialDevicePos = d->reader->device()->pos();
    connect(d->nextImageTimer, SIGNAL(timeout()), this, SLOT(_q_loadNextFrame()));
}

/*!
    Destructs the QtcMovie object.
*/
QtcMovie::~QtcMovie()
{
    Q_D(QtcMovie);
    delete d->reader;
}

/*!
    Sets the current device to \a device. QtcMovie will read image data from
    this device when the movie is running.

    \sa device(), setFormat()
*/
void QtcMovie::setDevice(QIODevice *device)
{
    Q_D(QtcMovie);
    d->reader->setDevice(device);
    d->reset();
}

/*!
    Returns the device QtcMovie reads image data from. If no device has
    currently been assigned, \nullptr is returned.

    \sa setDevice(), fileName()
*/
QIODevice *QtcMovie::device() const
{
    Q_D(const QtcMovie);
    return d->reader->device();
}

/*!
    Sets the name of the file that QtcMovie reads image data from, to \a
    fileName.

    \sa fileName(), setDevice(), setFormat()
*/
void QtcMovie::setFileName(const QString &fileName)
{
    Q_D(QtcMovie);
    d->absoluteFilePath = QDir(fileName).absolutePath();
    d->reader->setFileName(fileName);
    d->reset();
}

/*!
    Returns the name of the file that QtcMovie reads image data from. If no file
    name has been assigned, or if the assigned device is not a file, an empty
    QString is returned.

    \sa setFileName(), device()
*/
QString QtcMovie::fileName() const
{
    Q_D(const QtcMovie);
    return d->reader->fileName();
}

/*!
    Sets the format that QtcMovie will use when decoding image data, to \a
    format. By default, QtcMovie will attempt to guess the format of the image
    data.

    You can call supportedFormats() for the full list of formats
    QtcMovie supports.

    \sa QImageReader::supportedImageFormats()
*/
void QtcMovie::setFormat(const QByteArray &format)
{
    Q_D(QtcMovie);
    d->reader->setFormat(format);
}

/*!
    Returns the format that QtcMovie uses when decoding image data. If no format
    has been assigned, an empty QByteArray() is returned.

    \sa setFormat()
*/
QByteArray QtcMovie::format() const
{
    Q_D(const QtcMovie);
    return d->reader->format();
}

/*!
    For image formats that support it, this function sets the background color
    to \a color.

    \sa backgroundColor()
*/
void QtcMovie::setBackgroundColor(const QColor &color)
{
    Q_D(QtcMovie);
    d->reader->setBackgroundColor(color);
}

/*!
    Returns the background color of the movie. If no background color has been
    assigned, an invalid QColor is returned.

    \sa setBackgroundColor()
*/
QColor QtcMovie::backgroundColor() const
{
    Q_D(const QtcMovie);
    return d->reader->backgroundColor();
}

/*!
    Returns the current state of QtcMovie.

    \sa MovieState, stateChanged()
*/
QtcMovie::MovieState QtcMovie::state() const
{
    Q_D(const QtcMovie);
    return d->movieState;
}

/*!
    Returns the rect of the last frame. If no frame has yet been updated, an
    invalid QRect is returned.

    \sa currentImage(), currentPixmap()
*/
QRect QtcMovie::frameRect() const
{
    Q_D(const QtcMovie);
    return d->frameRect;
}

/*!
    Returns the current frame as a QPixmap.

    \sa currentImage(), updated()
*/
QPixmap QtcMovie::currentPixmap() const
{
    Q_D(const QtcMovie);
    if (!d->currentPixmap)
        d->currentPixmap = QPixmap::fromImage(d->currentImage);
    return *d->currentPixmap;
}

/*!
    Returns the current frame as a QImage.

    \sa currentPixmap(), updated()
*/
QImage QtcMovie::currentImage() const
{
    Q_D(const QtcMovie);
    return d->currentImage;
}

/*!
    Returns \c true if the movie is valid (e.g., the image data is readable and
    the image format is supported); otherwise returns \c false.

    For information about why the movie is not valid, see lastError().
*/
bool QtcMovie::isValid() const
{
    Q_D(const QtcMovie);
    return d->isValid();
}

/*!
    Returns the most recent error that occurred while attempting to read image data.

    \sa lastErrorString()
*/
QImageReader::ImageReaderError QtcMovie::lastError() const
{
    Q_D(const QtcMovie);
    return d->reader->error();
}

/*!
     Returns a human-readable representation of the most recent error that occurred
     while attempting to read image data.

    \sa lastError()
*/
QString QtcMovie::lastErrorString() const
{
    Q_D(const QtcMovie);
    return d->reader->errorString();
}

/*!
    Returns the number of frames in the movie.

    Certain animation formats do not support this feature, in which
    case 0 is returned.
*/
int QtcMovie::frameCount() const
{
    Q_D(const QtcMovie);
    return d->frameCount();
}

/*!
    Returns the number of milliseconds QtcMovie will wait before updating the
    next frame in the animation.
*/
int QtcMovie::nextFrameDelay() const
{
    Q_D(const QtcMovie);
    return d->nextDelay;
}

/*!
    Returns the sequence number of the current frame. The number of the first
    frame in the movie is 0.
*/
int QtcMovie::currentFrameNumber() const
{
    Q_D(const QtcMovie);
    return d->currentFrameNumber;
}

/*!
    Jumps to the next frame. Returns \c true on success; otherwise returns \c false.
*/
bool QtcMovie::jumpToNextFrame()
{
    Q_D(QtcMovie);
    return d->jumpToNextFrame();
}

/*!
    Jumps to frame number \a frameNumber. Returns \c true on success; otherwise
    returns \c false.
*/
bool QtcMovie::jumpToFrame(int frameNumber)
{
    Q_D(QtcMovie);
    return d->jumpToFrame(frameNumber);
}

/*!
    Returns the number of times the movie will loop before it finishes.
    If the movie will only play once (no looping), loopCount returns 0.
    If the movie loops forever, loopCount returns -1.

    Note that, if the image data comes from a sequential device (e.g. a
    socket), QtcMovie can only loop the movie if the cacheMode is set to
    QtcMovie::CacheAll.
*/
int QtcMovie::loopCount() const
{
    Q_D(const QtcMovie);
    return d->reader->loopCount();
}

/*!
    If \a paused is true, QtcMovie will enter \l Paused state and emit
    stateChanged(Paused); otherwise it will enter \l Running state and emit
    stateChanged(Running).

    \sa state()
*/
void QtcMovie::setPaused(bool paused)
{
    Q_D(QtcMovie);
    if (paused) {
        if (d->movieState == NotRunning)
            return;
        d->enterState(Paused);
        d->nextImageTimer->stop();
    } else {
        if (d->movieState == Running)
            return;
        d->enterState(Running);
        d->nextImageTimer->start(nextFrameDelay());
    }
}

/*!
    \property QtcMovie::speed
    \brief the movie's speed

    The speed is measured in percentage of the original movie speed.
    The default speed is 100%.
    Example:

    \snippet code/src_gui_image_Qtcmovie.cpp 1
*/
void QtcMovie::setSpeed(int percentSpeed)
{
    Q_D(QtcMovie);
    if (!d->speed && d->movieState == Running)
        d->nextImageTimer->start(nextFrameDelay());
    if (percentSpeed != d->speed) {
        d->speed = percentSpeed;
        d->speed.notify();
    } else {
        d->speed.removeBindingUnlessInWrapper();
    }
}

int QtcMovie::speed() const
{
    Q_D(const QtcMovie);
    return d->speed;
}

QBindable<int> QtcMovie::bindableSpeed()
{
    Q_D(QtcMovie);
    return &d->speed;
}

/*!
    Starts the movie. QtcMovie will enter \l Running state, and start emitting
    updated() and resized() as the movie progresses.

    If QtcMovie is in the \l Paused state, this function is equivalent
    to calling setPaused(false). If QtcMovie is already in the \l
    Running state, this function does nothing.

    \sa stop(), setPaused()
*/
void QtcMovie::start()
{
    Q_D(QtcMovie);
    if (d->movieState == NotRunning) {
        d->_q_loadNextFrame(true);
    } else if (d->movieState == Paused) {
        setPaused(false);
    }
}

/*!
    Stops the movie. QtcMovie enters \l NotRunning state, and stops emitting
    updated() and resized(). If start() is called again, the movie will
    restart from the beginning.

    If QtcMovie is already in the \l NotRunning state, this function
    does nothing.

    \sa start(), setPaused()
*/
void QtcMovie::stop()
{
    Q_D(QtcMovie);
    if (d->movieState == NotRunning)
        return;
    d->enterState(NotRunning);
    d->nextImageTimer->stop();
    d->nextFrameNumber = 0;
}

/*!
    Returns the scaled size of frames.

    \sa QImageReader::scaledSize()
*/
QSize QtcMovie::scaledSize()
{
    Q_D(QtcMovie);
    return d->reader->scaledSize();
}

/*!
    Sets the scaled frame size to \a size.

    \sa QImageReader::setScaledSize()
*/
void QtcMovie::setScaledSize(const QSize &size)
{
    Q_D(QtcMovie);
    d->reader->setScaledSize(size);
}

/*!
    Returns the list of image formats supported by QtcMovie.

    \sa QImageReader::supportedImageFormats()
*/
QList<QByteArray> QtcMovie::supportedFormats()
{
    QList<QByteArray> list = QImageReader::supportedImageFormats();

    QBuffer buffer;
    buffer.open(QIODevice::ReadOnly);

    const auto doesntSupportAnimation = [&buffer](const QByteArray &format) {
        return !QImageReader(&buffer, format).supportsOption(QImageIOHandler::Animation);
    };

    list.removeIf(doesntSupportAnimation);
    return list;
}

/*!
    \property QtcMovie::cacheMode
    \brief the movie's cache mode

    Caching frames can be useful when the underlying animation format handler
    that QtcMovie relies on to decode the animation data does not support
    jumping to particular frames in the animation, or even "rewinding" the
    animation to the beginning (for looping). Furthermore, if the image data
    comes from a sequential device, it is not possible for the underlying
    animation handler to seek back to frames whose data has already been read
    (making looping altogether impossible).

    To aid in such situations, a QtcMovie object can be instructed to cache the
    frames, at the added memory cost of keeping the frames in memory for the
    lifetime of the object.

    By default, this property is set to \l CacheNone.

    \sa QtcMovie::CacheMode
*/

QtcMovie::CacheMode QtcMovie::cacheMode() const
{
    Q_D(const QtcMovie);
    return d->cacheMode;
}

void QtcMovie::setCacheMode(CacheMode cacheMode)
{
    Q_D(QtcMovie);
    d->cacheMode = cacheMode;
}

QBindable<QtcMovie::CacheMode> QtcMovie::bindableCacheMode()
{
    Q_D(QtcMovie);
    return &d->cacheMode;
}

#include "moc_movie.cpp"
