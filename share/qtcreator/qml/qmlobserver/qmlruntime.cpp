/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <qdeclarativeview.h>

#ifdef hz
#undef hz
#endif
#ifdef Q_WS_MAEMO_5
#  include <QMaemo5ValueButton>
#  include <QMaemo5ListPickSelector>
#  include <QWidgetAction>
#  include <QStringListModel>
#  include "ui_recopts_maemo5.h"
#else
#  include "ui_recopts.h"
#endif

#include "qmlruntime.h"
#include <qdeclarativecontext.h>
#include <qdeclarativeengine.h>
#include <qdeclarativenetworkaccessmanagerfactory.h>
#include "qdeclarative.h"
#include <QAbstractAnimation>
#ifndef NO_PRIVATE_HEADERS
#include <private/qabstractanimation_p.h>
#endif

#include <qdeclarativeviewinspector.h>
#include <qdeclarativeinspectorservice.h>

#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkDiskCache>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>

#include <QDeclarativeComponent>

#include <QWidget>
#include <QApplication>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QProgressDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QGraphicsObject>
#include <QKeyEvent>

#include <QSignalMapper>
#include <QSettings>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QTranslator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

#include "proxysettings.h"
#include "deviceorientation.h"

#ifdef GL_SUPPORTED
#include <QGLWidget>
#endif

#if defined(Q_WS_S60)
#include <aknappui.h> // For locking app orientation
#endif

#include <qdeclarativetester.h>

#include "jsdebuggeragent.h"

QT_BEGIN_NAMESPACE

class DragAndDropView : public QDeclarativeView
{
    Q_OBJECT
public:
    DragAndDropView(QDeclarativeViewer *parent = 0)
    : QDeclarativeView(parent)
    {
        setAcceptDrops(true);
    }

    void dragEnterEvent(QDragEnterEvent *event)
    {
        const QMimeData *mimeData = event->mimeData();
        if (mimeData->hasUrls())
            event->acceptProposedAction();
    }

    void dragMoveEvent(QDragMoveEvent *event)
    {
        event->acceptProposedAction();
    }

    void dragLeaveEvent(QDragLeaveEvent *event)
    {
        event->accept();
    }

    void dropEvent(QDropEvent *event)
    {
        const QMimeData *mimeData = event->mimeData();
        if (!mimeData->hasUrls())
            return;
        const QList<QUrl> urlList = mimeData->urls();
        foreach (const QUrl &url, urlList) {
            if (url.scheme() == QLatin1String("file")) {
                static_cast<QDeclarativeViewer *>(parent())->open(url.toLocalFile());
                event->accept();
                return;
            }
        }
    }
};

class Runtime : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isActiveWindow READ isActiveWindow NOTIFY isActiveWindowChanged)
    Q_PROPERTY(DeviceOrientation::Orientation orientation READ orientation NOTIFY orientationChanged)

public:
    static Runtime* instance()
    {
        static Runtime *instance = 0;
        if (!instance)
            instance = new Runtime;
        return instance;
    }

    bool isActiveWindow() const { return activeWindow; }
    void setActiveWindow(bool active)
    {
        if (active == activeWindow)
            return;
        activeWindow = active;
        emit isActiveWindowChanged();
    }

    DeviceOrientation::Orientation orientation() const { return DeviceOrientation::instance()->orientation(); }

Q_SIGNALS:
    void isActiveWindowChanged();
    void orientationChanged();

private:
    Runtime(QObject *parent=0) : QObject(parent), activeWindow(false)
    {
        connect(DeviceOrientation::instance(), SIGNAL(orientationChanged()),
                this, SIGNAL(orientationChanged()));
    }

    bool activeWindow;
};



#if defined(Q_WS_MAEMO_5)

class Maemo5PickerAction : public QWidgetAction {
    Q_OBJECT
public:
    Maemo5PickerAction(const QString &text, QActionGroup *actions, QObject *parent)
        : QWidgetAction(parent), m_text(text), m_actions(actions)
    { }

    QWidget *createWidget(QWidget *parent)
    {
        QMaemo5ValueButton *button = new QMaemo5ValueButton(m_text, parent);
        button->setValueLayout(QMaemo5ValueButton::ValueUnderTextCentered);
        QMaemo5ListPickSelector *pick = new QMaemo5ListPickSelector(button);
        button->setPickSelector(pick);
        if (m_actions) {
            QStringList sl;
            int curIdx = -1, idx = 0;
            foreach (QAction *a, m_actions->actions()) {
                sl << a->text();
                if (a->isChecked())
                    curIdx = idx;
                idx++;
            }
            pick->setModel(new QStringListModel(sl));
            pick->setCurrentIndex(curIdx);
        } else {
            button->setEnabled(false);
        }
        connect(pick, SIGNAL(selected(QString)), this, SLOT(emitTriggered()));
        return button;
    }

private slots:
    void emitTriggered()
    {
        QMaemo5ListPickSelector *pick = qobject_cast<QMaemo5ListPickSelector *>(sender());
        if (!pick)
            return;
        int idx = pick->currentIndex();

        if (m_actions && idx >= 0 && idx < m_actions->actions().count())
            m_actions->actions().at(idx)->trigger();
    }

private:
    QString m_text;
    QPointer<QActionGroup> m_actions;
};

#endif // Q_WS_MAEMO_5

static struct { const char *name, *args; } ffmpegprofiles[] = {
    {"Maximum Quality", "-sameq"},
    {"High Quality", "-qmax 2"},
    {"Medium Quality", "-qmax 6"},
    {"Low Quality", "-qmax 16"},
    {"Custom ffmpeg arguments", ""},
    {0,0}
};

class RecordingDialog : public QDialog, public Ui::RecordingOptions {
    Q_OBJECT

public:
    RecordingDialog(QWidget *parent) : QDialog(parent)
    {
        setupUi(this);
#ifndef Q_WS_MAEMO_5
        hz->setValidator(new QDoubleValidator(hz));
#endif
        for (int i=0; ffmpegprofiles[i].name; ++i) {
            profile->addItem(ffmpegprofiles[i].name);
        }
    }

    void setArguments(QString a)
    {
        int i;
        for (i=0; ffmpegprofiles[i].args[0]; ++i) {
            if (ffmpegprofiles[i].args == a) {
                profile->setCurrentIndex(i);
                args->setText(QLatin1String(ffmpegprofiles[i].args));
                return;
            }
        }
        customargs = a;
        args->setText(a);
        profile->setCurrentIndex(i);
    }

    QString arguments() const
    {
        int i = profile->currentIndex();
        return ffmpegprofiles[i].args[0] ? QLatin1String(ffmpegprofiles[i].args) : customargs;
    }

    void setOriginalSize(const QSize &s)
    {
        QString str = tr("Original (%1x%2)").arg(s.width()).arg(s.height());

#ifdef Q_WS_MAEMO_5
        sizeCombo->setItemText(0, str);
#else
        sizeOriginal->setText(str);
        if (sizeWidth->value()<=1) {
            sizeWidth->setValue(s.width());
            sizeHeight->setValue(s.height());
        }
#endif
    }

    void showffmpegOptions(bool b)
    {
#ifdef Q_WS_MAEMO_5
        profileLabel->setVisible(b);
        profile->setVisible(b);
        ffmpegHelp->setVisible(b);
        args->setVisible(b);
#else
        ffmpegOptions->setVisible(b);
#endif
    }

    void showRateOptions(bool b)
    {
#ifdef Q_WS_MAEMO_5
        rateLabel->setVisible(b);
        rateCombo->setVisible(b);
#else
        rateOptions->setVisible(b);
#endif
    }

    void setVideoRate(int rate)
    {
#ifdef Q_WS_MAEMO_5
        int idx;
        if (rate >= 60)
            idx = 0;
        else if (rate >= 50)
            idx = 2;
        else if (rate >= 25)
            idx = 3;
        else if (rate >= 24)
            idx = 4;
        else if (rate >= 20)
            idx = 5;
        else if (rate >= 15)
            idx = 6;
        else
            idx = 7;
        rateCombo->setCurrentIndex(idx);
#else
        if (rate == 24)
            hz24->setChecked(true);
        else if (rate == 25)
            hz25->setChecked(true);
        else if (rate == 50)
            hz50->setChecked(true);
        else if (rate == 60)
            hz60->setChecked(true);
        else {
            hzCustom->setChecked(true);
            hz->setText(QString::number(rate));
        }
#endif
    }

    int videoRate() const
    {
#ifdef Q_WS_MAEMO_5
        switch (rateCombo->currentIndex()) {
            case 0: return 60;
            case 1: return 50;
            case 2: return 25;
            case 3: return 24;
            case 4: return 20;
            case 5: return 15;
            case 7: return 10;
            default: return 60;
        }
#else
        if (hz24->isChecked())
            return 24;
        else if (hz25->isChecked())
            return 25;
        else if (hz50->isChecked())
            return 50;
        else if (hz60->isChecked())
            return 60;
        else {
            return hz->text().toInt();
        }
#endif
    }

    QSize videoSize() const
    {
#ifdef Q_WS_MAEMO_5
        switch (sizeCombo->currentIndex()) {
            case 0: return QSize();
            case 1: return QSize(640,480);
            case 2: return QSize(320,240);
            case 3: return QSize(1280,720);
            default: return QSize();
        }
#else
        if (sizeOriginal->isChecked())
            return QSize();
        else if (size720p->isChecked())
            return QSize(1280,720);
        else if (sizeVGA->isChecked())
            return QSize(640,480);
        else if (sizeQVGA->isChecked())
            return QSize(320,240);
        else
            return QSize(sizeWidth->value(), sizeHeight->value());
#endif
    }



private slots:
    void pickProfile(int i)
    {
        if (ffmpegprofiles[i].args[0]) {
            args->setText(QLatin1String(ffmpegprofiles[i].args));
        } else {
            args->setText(customargs);
        }
    }

    void storeCustomArgs(QString s)
    {
        setArguments(s);
    }

private:
    QString customargs;
};

class PersistentCookieJar : public QNetworkCookieJar {
public:
    PersistentCookieJar(QObject *parent) : QNetworkCookieJar(parent) { load(); }
    ~PersistentCookieJar() { save(); }

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::cookiesForUrl(url);
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
    {
        QMutexLocker lock(&mutex);
        return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    }

private:
    void save()
    {
        QMutexLocker lock(&mutex);
        QList<QNetworkCookie> list = allCookies();
        QByteArray data;
        foreach (const QNetworkCookie &cookie, list) {
            if (!cookie.isSessionCookie()) {
                data.append(cookie.toRawForm());
                data.append('\n');
            }
        }
        QSettings settings;
        settings.setValue("Cookies",data);
    }

    void load()
    {
        QMutexLocker lock(&mutex);
        QSettings settings;
        QByteArray data = settings.value("Cookies").toByteArray();
        setAllCookies(QNetworkCookie::parseCookies(data));
    }

    mutable QMutex mutex;
};

class SystemProxyFactory : public QNetworkProxyFactory
{
public:
    SystemProxyFactory() : proxyDirty(true), httpProxyInUse(false) {
    }

    virtual QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query)
    {
        if (proxyDirty)
            setupProxy();
        QString protocolTag = query.protocolTag();
        if (httpProxyInUse && (protocolTag == "http" || protocolTag == "https")) {
            QList<QNetworkProxy> ret;
            ret << httpProxy;
            return ret;
        }
#ifdef Q_OS_WIN
        // systemProxyForQuery can take insanely long on Windows (QTBUG-10106)
        return QNetworkProxyFactory::proxyForQuery(query);
#else
        return QNetworkProxyFactory::systemProxyForQuery(query);
#endif
    }

    void setupProxy() {
        // Don't bother locking because we know that the proxy only
        // changes in response to the settings dialog and that
        // the view will be reloaded.
        proxyDirty = false;
        httpProxyInUse = ProxySettings::httpProxyInUse();
        if (httpProxyInUse)
            httpProxy = ProxySettings::httpProxy();
    }

    void proxyChanged() {
        proxyDirty = true;
    }

private:
    volatile bool proxyDirty;
    bool httpProxyInUse;
    QNetworkProxy httpProxy;
};

class NetworkAccessManagerFactory : public QObject, public QDeclarativeNetworkAccessManagerFactory
{
    Q_OBJECT
public:
    NetworkAccessManagerFactory() : cacheSize(0) {}
    ~NetworkAccessManagerFactory() {}

    QNetworkAccessManager *create(QObject *parent);

    void setCacheSize(int size) {
        if (size != cacheSize) {
            cacheSize = size;
        }
    }

    void proxyChanged() {
        foreach (QNetworkAccessManager *nam, namList) {
            static_cast<SystemProxyFactory*>(nam->proxyFactory())->proxyChanged();
        }
    }

    static PersistentCookieJar *cookieJar;

private slots:
    void managerDestroyed(QObject *obj) {
        namList.removeOne(static_cast<QNetworkAccessManager*>(obj));
    }

private:
    QMutex mutex;
    int cacheSize;
    QList<QNetworkAccessManager*> namList;
};

PersistentCookieJar *NetworkAccessManagerFactory::cookieJar = 0;

static void cleanup_cookieJar()
{
    delete NetworkAccessManagerFactory::cookieJar;
    NetworkAccessManagerFactory::cookieJar = 0;
}

QNetworkAccessManager *NetworkAccessManagerFactory::create(QObject *parent)
{
    QMutexLocker lock(&mutex);
    QNetworkAccessManager *manager = new QNetworkAccessManager(parent);
    if (!cookieJar) {
        qAddPostRoutine(cleanup_cookieJar);
        cookieJar = new PersistentCookieJar(0);
    }
    manager->setCookieJar(cookieJar);
    cookieJar->setParent(0);
    manager->setProxyFactory(new SystemProxyFactory);
    if (cacheSize > 0) {
        QNetworkDiskCache *cache = new QNetworkDiskCache;
        cache->setCacheDirectory(QDir::tempPath()+QLatin1String("/qml-viewer-network-cache"));
        cache->setMaximumCacheSize(cacheSize);
        manager->setCache(cache);
    } else {
        manager->setCache(0);
    }
    connect(manager, SIGNAL(destroyed(QObject*)), this, SLOT(managerDestroyed(QObject*)));
    namList.append(manager);
    qDebug() << "created new network access manager for" << parent;
    return manager;
}

QString QDeclarativeViewer::getVideoFileName()
{
    QString title = convertAvailable || ffmpegAvailable ? tr("Save Video File") : tr("Save PNG Frames");
    QStringList types;
    if (ffmpegAvailable) types += tr("Common Video files")+QLatin1String(" (*.avi *.mpeg *.mov)");
    if (convertAvailable) types += tr("GIF Animation")+QLatin1String(" (*.gif)");
    types += tr("Individual PNG frames")+QLatin1String(" (*.png)");
    if (ffmpegAvailable) types += tr("All ffmpeg formats (*.*)");
    return QFileDialog::getSaveFileName(this, title, "", types.join(";; "));
}

QDeclarativeViewer::QDeclarativeViewer(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
      , loggerWindow(new LoggerWidget(this))
      , frame_stream(0)
      , rotateAction(0)
      , orientation(0)
      , showWarningsWindow(0)
      , designModeBehaviorAction(0)
      , m_scriptOptions(0)
      , tester(0)
      , useQmlFileBrowser(true)
      , translator(0)
{
    QDeclarativeViewer::registerTypes();
    setWindowTitle(tr("Qt QML Viewer"));
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
//    setPalette(QApplication::palette("QLabel"));
#endif

    devicemode = false;
    canvas = 0;
    record_autotime = 0;
    record_rate = 50;
    record_args += QLatin1String("-sameq");

    recdlg = new RecordingDialog(this);
    connect(recdlg->pickfile, SIGNAL(clicked()), this, SLOT(pickRecordingFile()));
    senseFfmpeg();
    senseImageMagick();
    if (!ffmpegAvailable)
        recdlg->showffmpegOptions(false);
    if (!ffmpegAvailable && !convertAvailable)
        recdlg->showRateOptions(false);
    QString warn;
    if (!ffmpegAvailable) {
        if (!convertAvailable)
            warn = tr("ffmpeg and ImageMagick not available - no video output");
        else
            warn = tr("ffmpeg not available - GIF and PNG outputs only");
        recdlg->warning->setText(warn);
    } else {
        recdlg->warning->hide();
    }

    canvas = new DragAndDropView(this);
    inspector = new QmlJSDebugger::QDeclarativeViewInspector(canvas, this);
    new QmlJSDebugger::JSDebuggerAgent(canvas->engine());

    canvas->setAttribute(Qt::WA_OpaquePaintEvent);
    canvas->setAttribute(Qt::WA_NoSystemBackground);

    canvas->setFocus();

    QObject::connect(canvas, SIGNAL(sceneResized(QSize)), this, SLOT(sceneResized(QSize)));
    QObject::connect(canvas, SIGNAL(statusChanged(QDeclarativeView::Status)), this, SLOT(statusChanged()));
    QObject::connect(canvas->engine(), SIGNAL(quit()), this, SLOT(close()));

    QObject::connect(warningsWidget(), SIGNAL(opened()), this, SLOT(warningsWidgetOpened()));
    QObject::connect(warningsWidget(), SIGNAL(closed()), this, SLOT(warningsWidgetClosed()));

    if (!(flags & Qt::FramelessWindowHint)) {
        createMenu();
        changeOrientation(orientation->actions().value(0));
    } else {
        setMenuBar(0);
    }

    setCentralWidget(canvas);

    namFactory = new NetworkAccessManagerFactory;
    canvas->engine()->setNetworkAccessManagerFactory(namFactory);

    connect(&autoStartTimer, SIGNAL(timeout()), this, SLOT(autoStartRecording()));
    connect(&autoStopTimer, SIGNAL(timeout()), this, SLOT(autoStopRecording()));
    connect(&recordTimer, SIGNAL(timeout()), this, SLOT(recordFrame()));
    connect(DeviceOrientation::instance(), SIGNAL(orientationChanged()),
            this, SLOT(orientationChanged()), Qt::QueuedConnection);
    autoStartTimer.setSingleShot(true);
    autoStopTimer.setSingleShot(true);
    recordTimer.setSingleShot(false);

    QObject::connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(appAboutToQuit()));
}

QDeclarativeViewer::~QDeclarativeViewer()
{
    delete loggerWindow;
    canvas->engine()->setNetworkAccessManagerFactory(0);
    delete namFactory;
}

void QDeclarativeViewer::setDesignModeBehavior(bool value)
{
    if (designModeBehaviorAction)
        designModeBehaviorAction->setChecked(value);
    inspector->setDesignModeBehavior(value);
}

void QDeclarativeViewer::enableExperimentalGestures()
{
#ifndef QT_NO_GESTURES
    canvas->viewport()->grabGesture(Qt::TapGesture,Qt::DontStartGestureOnChildren|Qt::ReceivePartialGestures|Qt::IgnoredGesturesPropagateToParent);
    canvas->viewport()->grabGesture(Qt::TapAndHoldGesture,Qt::DontStartGestureOnChildren|Qt::ReceivePartialGestures|Qt::IgnoredGesturesPropagateToParent);
    canvas->viewport()->grabGesture(Qt::PanGesture,Qt::DontStartGestureOnChildren|Qt::ReceivePartialGestures|Qt::IgnoredGesturesPropagateToParent);
    canvas->viewport()->grabGesture(Qt::PinchGesture,Qt::DontStartGestureOnChildren|Qt::ReceivePartialGestures|Qt::IgnoredGesturesPropagateToParent);
    canvas->viewport()->grabGesture(Qt::SwipeGesture,Qt::DontStartGestureOnChildren|Qt::ReceivePartialGestures|Qt::IgnoredGesturesPropagateToParent);
    canvas->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
#endif
}

QDeclarativeView *QDeclarativeViewer::view() const
{
    return canvas;
}

LoggerWidget *QDeclarativeViewer::warningsWidget() const
{
    return loggerWindow;
}

void QDeclarativeViewer::createMenu()
{
    QAction *openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcuts(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    QAction *openUrlAction = new QAction(tr("Open &URL..."), this);
    connect(openUrlAction, SIGNAL(triggered()), this, SLOT(openUrl()));

    QAction *reloadAction = new QAction(tr("&Reload"), this);
    reloadAction->setShortcuts(QKeySequence::Refresh);
    connect(reloadAction, SIGNAL(triggered()), this, SLOT(reload()));

    QAction *snapshotAction = new QAction(tr("&Take Snapshot"), this);
    snapshotAction->setShortcut(QKeySequence("F3"));
    connect(snapshotAction, SIGNAL(triggered()), this, SLOT(takeSnapShot()));

    recordAction = new QAction(tr("Start Recording &Video"), this);
    recordAction->setShortcut(QKeySequence("F9"));
    connect(recordAction, SIGNAL(triggered()), this, SLOT(toggleRecordingWithSelection()));

    QAction *recordOptions = new QAction(tr("Video &Options..."), this);
    connect(recordOptions, SIGNAL(triggered()), this, SLOT(chooseRecordingOptions()));

    QMenu *playSpeedMenu = new QMenu(tr("Animation Speed"), this);
    playSpeedMenuActions = new QActionGroup(this);
    playSpeedMenuActions->setExclusive(true);

    QAction *speedAction = playSpeedMenu->addAction(tr("1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setChecked(true);
    speedAction->setData(1.0f);
    playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.5x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(2.0f);
    playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.25x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(4.0f);
    playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.125x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(8.0f);
    playSpeedMenuActions->addAction(speedAction);

    speedAction = playSpeedMenu->addAction(tr("0.1x"), this, SLOT(changeAnimationSpeed()));
    speedAction->setCheckable(true);
    speedAction->setData(10.0f);
    playSpeedMenuActions->addAction(speedAction);

    pauseAnimationsAction = playSpeedMenu->addAction(tr("Pause"), inspector, SLOT(setAnimationPaused(bool)));
    pauseAnimationsAction->setCheckable(true);
    pauseAnimationsAction->setShortcut(QKeySequence("Ctrl+."));

    animationStepAction = playSpeedMenu->addAction(tr("Pause and step"), this, SLOT(stepAnimations()));
    animationStepAction->setShortcut(QKeySequence("Ctrl+,"));

    animationSetStepAction = playSpeedMenu->addAction(tr("Set step"), this, SLOT(setAnimationStep()));
    m_stepSize = 40;

    QAction *playSpeedAction = new QAction(tr("Animations"), this);
    playSpeedAction->setMenu(playSpeedMenu);

    connect(inspector, SIGNAL(animationSpeedChanged(qreal)), SLOT(animationSpeedChanged(qreal)));
    connect(inspector, SIGNAL(animationPausedChanged(bool)), pauseAnimationsAction, SLOT(setChecked(bool)));

    showWarningsWindow = new QAction(tr("Show Warnings"), this);
    showWarningsWindow->setCheckable((true));
    showWarningsWindow->setChecked(loggerWindow->isVisible());
    connect(showWarningsWindow, SIGNAL(triggered(bool)), this, SLOT(showWarnings(bool)));

    designModeBehaviorAction = new QAction(tr("&Inspector Mode"), this);
    designModeBehaviorAction->setShortcut(QKeySequence("Ctrl+D"));
    designModeBehaviorAction->setCheckable(true);
    designModeBehaviorAction->setChecked(inspector->designModeBehavior());
    designModeBehaviorAction->setEnabled(QmlJSDebugger::QDeclarativeInspectorService::hasDebuggingClient());
    connect(designModeBehaviorAction, SIGNAL(triggered(bool)), this, SLOT(setDesignModeBehavior(bool)));
    connect(inspector, SIGNAL(designModeBehaviorChanged(bool)), designModeBehaviorAction, SLOT(setChecked(bool)));
    connect(QmlJSDebugger::QDeclarativeInspectorService::instance(), SIGNAL(debuggingClientChanged(bool)),
            designModeBehaviorAction, SLOT(setEnabled(bool)));

    appOnTopAction = new QAction(tr("Keep Window on Top"), this);
    appOnTopAction->setCheckable(true);
    appOnTopAction->setChecked(inspector->showAppOnTop());

    connect(appOnTopAction, SIGNAL(triggered(bool)), inspector, SLOT(setShowAppOnTop(bool)));
    connect(inspector, SIGNAL(showAppOnTopChanged(bool)), appOnTopAction, SLOT(setChecked(bool)));

    QAction *proxyAction = new QAction(tr("HTTP &Proxy..."), this);
    connect(proxyAction, SIGNAL(triggered()), this, SLOT(showProxySettings()));

    QAction *fullscreenAction = new QAction(tr("Full Screen"), this);
    fullscreenAction->setCheckable(true);
    connect(fullscreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

    rotateAction = new QAction(tr("Rotate orientation"), this);
    rotateAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(rotateAction, SIGNAL(triggered()), this, SLOT(rotateOrientation()));

    orientation = new QActionGroup(this);
    orientation->setExclusive(true);
    connect(orientation, SIGNAL(triggered(QAction*)), this, SLOT(changeOrientation(QAction*)));

    QAction *portraitAction = new QAction(tr("Portrait"), this);
    portraitAction->setCheckable(true);
    QAction *landscapeAction = new QAction(tr("Landscape"), this);
    landscapeAction->setCheckable(true);
    QAction *portraitInvAction = new QAction(tr("Portrait (inverted)"), this);
    portraitInvAction->setCheckable(true);
    QAction *landscapeInvAction = new QAction(tr("Landscape (inverted)"), this);
    landscapeInvAction->setCheckable(true);

    QAction *aboutAction = new QAction(tr("&About Qt..."), this);
    aboutAction->setMenuRole(QAction::AboutQtRole);
    connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    QAction *closeAction = new QAction(tr("&Close"), this);
    closeAction->setShortcuts(QKeySequence::Close);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(close()));

    QAction *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcuts(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    QMenuBar *menu = menuBar();
    if (!menu)
        return;

#if defined(Q_WS_MAEMO_5)
    menu->addAction(openAction);
    menu->addAction(openUrlAction);
    menu->addAction(reloadAction);

    menu->addAction(snapshotAction);
    menu->addAction(recordAction);

    menu->addAction(recordOptions);
    menu->addAction(proxyAction);

    menu->addAction(playSpeedMenu);
    menu->addAction(showWarningsWindow);

    orientation->addAction(landscapeAction);
    orientation->addAction(portraitAction);
    menu->addAction(new Maemo5PickerAction(tr("Set orientation"), orientation, this));
    menu->addAction(fullscreenAction);
    return;
#endif // Q_WS_MAEMO_5

    QMenu *fileMenu = menu->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(openUrlAction);
    fileMenu->addAction(reloadAction);
    fileMenu->addSeparator();
    fileMenu->addAction(closeAction);
    fileMenu->addAction(quitAction);

    QMenu *recordMenu = menu->addMenu(tr("&Recording"));
    recordMenu->addAction(snapshotAction);
    recordMenu->addAction(recordAction);

    QMenu *debugMenu = menu->addMenu(tr("&Debugging"));
    debugMenu->addMenu(playSpeedMenu);
    debugMenu->addAction(showWarningsWindow);
    debugMenu->addAction(designModeBehaviorAction);
    debugMenu->addAction(appOnTopAction);

    QMenu *settingsMenu = menu->addMenu(tr("&Settings"));
    settingsMenu->addAction(proxyAction);
    settingsMenu->addAction(recordOptions);
    settingsMenu->addMenu(loggerWindow->preferencesMenu());
    settingsMenu->addAction(rotateAction);

    QMenu *propertiesMenu = settingsMenu->addMenu(tr("Properties"));

    orientation->addAction(portraitAction);
    orientation->addAction(landscapeAction);
    orientation->addAction(portraitInvAction);
    orientation->addAction(landscapeInvAction);
    propertiesMenu->addActions(orientation->actions());

    QMenu *helpMenu = menu->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

void QDeclarativeViewer::showProxySettings()
{
    ProxySettings settingsDlg (this);

    connect(&settingsDlg, SIGNAL(accepted()), this, SLOT(proxySettingsChanged()));

    settingsDlg.exec();
}

void QDeclarativeViewer::proxySettingsChanged()
{
    namFactory->proxyChanged();
    reload ();
}

void QDeclarativeViewer::rotateOrientation()
{
#if defined(Q_WS_S60)
    CAknAppUi *appUi = static_cast<CAknAppUi *>(CEikonEnv::Static()->AppUi());
    if (appUi) {
        CAknAppUi::TAppUiOrientation oldOrientation = appUi->Orientation();
        QString newOrientation;
        if (oldOrientation == CAknAppUi::EAppUiOrientationPortrait) {
            newOrientation = QLatin1String("Landscape");
        } else {
            newOrientation = QLatin1String("Portrait");
        }
        foreach (QAction *action, orientation->actions()) {
            if (action->text() == newOrientation) {
                changeOrientation(action);
            }
        }
    }
#else
    QAction *current = orientation->checkedAction();
    QList<QAction *> actions = orientation->actions();
    int index = actions.indexOf(current);
    if (index < 0)
        return;

    QAction *newOrientation = actions[(index + 1) % actions.count()];
    changeOrientation(newOrientation);
#endif
}

void QDeclarativeViewer::toggleFullScreen()
{
    if (isFullScreen())
        showMaximized();
    else
        showFullScreen();
}

void QDeclarativeViewer::showWarnings(bool show)
{
    loggerWindow->setVisible(show);
}

void QDeclarativeViewer::warningsWidgetOpened()
{
    showWarningsWindow->setChecked(true);
}

void QDeclarativeViewer::warningsWidgetClosed()
{
    showWarningsWindow->setChecked(false);
}

void QDeclarativeViewer::takeSnapShot()
{
    static int snapshotcount = 1;
    QString snapFileName = QString(QLatin1String("snapshot%1.png")).arg(snapshotcount);
    QPixmap::grabWidget(canvas).save(snapFileName);
    qDebug() << "Wrote" << snapFileName;
    ++snapshotcount;
}

void QDeclarativeViewer::pickRecordingFile()
{
    QString fileName = getVideoFileName();
    if (!fileName.isEmpty())
        recdlg->file->setText(fileName);
}

void QDeclarativeViewer::chooseRecordingOptions()
{
    // File
    recdlg->file->setText(record_file);

    // Size
    recdlg->setOriginalSize(canvas->size());

    // Rate
    recdlg->setVideoRate(record_rate);


    // Profile
    recdlg->setArguments(record_args.join(" "));
    if (recdlg->exec()) {
        // File
        record_file = recdlg->file->text();
        // Size
        record_outsize = recdlg->videoSize();
        // Rate
        record_rate = recdlg->videoRate();
        // Profile
        record_args = recdlg->arguments().split(QLatin1Char(' '),QString::SkipEmptyParts);
    }
}

void QDeclarativeViewer::toggleRecordingWithSelection()
{
    if (!recordTimer.isActive()) {
        if (record_file.isEmpty()) {
            QString fileName = getVideoFileName();
            if (fileName.isEmpty())
                return;
            if (!fileName.contains(QRegExp(".[^\\/]*$")))
                fileName += ".avi";
            setRecordFile(fileName);
        }
    }
    toggleRecording();
}

void QDeclarativeViewer::toggleRecording()
{
#ifndef NO_PRIVATE_HEADERS
    if (record_file.isEmpty()) {
        toggleRecordingWithSelection();
        return;
    }
    bool recording = !recordTimer.isActive();
    recordAction->setText(recording ? tr("&Stop Recording Video\tF9") : tr("&Start Recording Video\tF9"));
    setRecording(recording);
#endif
}

void QDeclarativeViewer::pauseAnimations()
{
    inspector->setAnimationPaused(true);
}

void QDeclarativeViewer::stepAnimations()
{
    inspector->setAnimationPaused(false);
    QTimer::singleShot(m_stepSize, this, SLOT(pauseAnimations()));
 }

void QDeclarativeViewer::setAnimationStep()
{
   bool ok;
   int stepSize = QInputDialog::getInt(this, tr("Set animation step duration"),
                                             tr("Step duration (ms):"), m_stepSize, 20, 10000, 1, &ok);
   if (ok) m_stepSize = stepSize;
}

void QDeclarativeViewer::changeAnimationSpeed()
{
   if (QAction *action = qobject_cast<QAction*>(sender()))
       inspector->setAnimationSpeed(action->data().toFloat());
}

void QDeclarativeViewer::addLibraryPath(const QString& lib)
{
    canvas->engine()->addImportPath(lib);
}

void QDeclarativeViewer::addPluginPath(const QString& plugin)
{
    canvas->engine()->addPluginPath(plugin);
}

void QDeclarativeViewer::reload()
{
    launch(currentFileOrUrl);
}

void QDeclarativeViewer::openFile()
{
    QString cur = canvas->source().toLocalFile();
    if (useQmlFileBrowser) {
        open("qrc:/browser/Browser.qml");
    } else {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open QML file"), cur, tr("QML Files (*.qml)"));
        if (!fileName.isEmpty()) {
            QFileInfo fi(fileName);
            open(fi.absoluteFilePath());
        }
    }
}

void QDeclarativeViewer::openUrl()
{
    QString cur = canvas->source().toLocalFile();
    QString url= QInputDialog::getText(this, tr("Open QML file"), tr("URL of main QML file:"), QLineEdit::Normal, cur);
    if (!url.isEmpty())
        open(url);
}

void QDeclarativeViewer::statusChanged()
{
    if (canvas->status() == QDeclarativeView::Error && tester)
        tester->executefailure();

    if (canvas->status() == QDeclarativeView::Ready) {
        initialSize = canvas->initialSize();
        updateSizeHints(true);
    }
}

void QDeclarativeViewer::launch(const QString& file_or_url)
{
    QMetaObject::invokeMethod(this, "open", Qt::QueuedConnection, Q_ARG(QString, file_or_url));
}

void QDeclarativeViewer::loadTranslationFile(const QString& directory)
{
    if (!translator) {
        translator = new QTranslator(this);
        QApplication::installTranslator(translator);
    }

    translator->load(QLatin1String("qml_" )+QLocale::system().name(), directory + QLatin1String("/i18n"));
}

void QDeclarativeViewer::loadDummyDataFiles(const QString& directory)
{
    QDir dir(directory+"/dummydata", "*.qml");
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QString qml = list.at(i);
        QDeclarativeComponent comp(canvas->engine(), dir.filePath(qml));
        QObject *dummyData = comp.create();

        if(comp.isError()) {
            QList<QDeclarativeError> errors = comp.errors();
            foreach (const QDeclarativeError &error, errors) {
                qWarning() << error;
            }
            if (tester) tester->executefailure();
        }

        if (dummyData) {
            qWarning() << "Loaded dummy data:" << dir.filePath(qml);
            qml.truncate(qml.length()-4);
            canvas->rootContext()->setContextProperty(qml, dummyData);
            dummyData->setParent(this);
        }
    }
}

bool QDeclarativeViewer::open(const QString& file_or_url)
{
    currentFileOrUrl = file_or_url;

    QUrl url;
    QFileInfo fi(file_or_url);
    if (fi.exists())
        url = QUrl::fromLocalFile(fi.absoluteFilePath());
    else
        url = QUrl(file_or_url);
    setWindowTitle(tr("%1 - Qt QML Viewer").arg(file_or_url));

#ifndef NO_PRIVATE_HEADERS
    if (!m_script.isEmpty())
        tester = new QDeclarativeTester(m_script, m_scriptOptions, canvas);
#endif

    delete canvas->rootObject();
    canvas->engine()->clearComponentCache();
    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("qmlViewer", this);
    ctxt->setContextProperty("qmlViewerFolder", QDir::currentPath());
    ctxt->setContextProperty("runtime", Runtime::instance());

    QString fileName = url.toLocalFile();
    if (!fileName.isEmpty()) {
        fi.setFile(fileName);
        if (fi.exists()) {
            if (fi.suffix().toLower() != QLatin1String("qml")) {
                qWarning() << "qml cannot open non-QML file" << fileName;
                return false;
            }

            QFileInfo fi(fileName);
            loadTranslationFile(fi.path());
            loadDummyDataFiles(fi.path());
        } else {
            qWarning() << "qml cannot find file:" << fileName;
            return false;
        }
    }

    QTime t;
    t.start();

    canvas->setSource(url);

    return true;
}

void QDeclarativeViewer::setAutoRecord(int from, int to)
{
    if (from==0) from=1; // ensure resized
    record_autotime = to-from;
    autoStartTimer.setInterval(from);
    autoStartTimer.start();
}

void QDeclarativeViewer::setRecordArgs(const QStringList& a)
{
    record_args = a;
}

void QDeclarativeViewer::setRecordFile(const QString& f)
{
    record_file = f;
}

void QDeclarativeViewer::setRecordRate(int fps)
{
    record_rate = fps;
}

void QDeclarativeViewer::sceneResized(QSize)
{
    updateSizeHints();
}

void QDeclarativeViewer::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_0 && devicemode)
        exit(0);
    else if (event->key() == Qt::Key_F1 || (event->key() == Qt::Key_1 && devicemode)) {
        qDebug() << "F1 - help\n"
                 << "F2 - save test script\n"
                 << "F3 - take PNG snapshot\n"
                 << "F4 - show items and state\n"
                 << "F5 - reload QML\n"
                 << "F6 - show object tree\n"
                 << "F7 - show timing\n"
                 << "F9 - toggle video recording\n"
                 << "F10 - toggle orientation\n"
                 << "device keys: 0=quit, 1..8=F1..F8"
                ;
    } else if (event->key() == Qt::Key_F2 || (event->key() == Qt::Key_2 && devicemode)) {
        if (tester && m_scriptOptions & Record)
            tester->save();
    } else if (event->key() == Qt::Key_F3 || (event->key() == Qt::Key_3 && devicemode)) {
        takeSnapShot();
    } else if (event->key() == Qt::Key_F5 || (event->key() == Qt::Key_5 && devicemode)) {
        reload();
    } else if (event->key() == Qt::Key_F9 || (event->key() == Qt::Key_9 && devicemode)) {
        toggleRecording();
    } else if (event->key() == Qt::Key_F10) {
        rotateOrientation();
    }

    QWidget::keyPressEvent(event);
}

bool QDeclarativeViewer::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        Runtime::instance()->setActiveWindow(true);
        DeviceOrientation::instance()->resumeListening();
    } else if (event->type() == QEvent::WindowDeactivate) {
        Runtime::instance()->setActiveWindow(false);
        DeviceOrientation::instance()->pauseListening();
    }
    return QWidget::event(event);
}

void QDeclarativeViewer::senseImageMagick()
{
    QProcess proc;
    proc.start("convert", QStringList() << "-h");
    proc.waitForFinished(2000);
    QString help = proc.readAllStandardOutput();
    convertAvailable = help.contains("ImageMagick");
}

void QDeclarativeViewer::senseFfmpeg()
{
    QProcess proc;
    proc.start("ffmpeg", QStringList() << "-h");
    proc.waitForFinished(2000);
    QString ffmpegHelp = proc.readAllStandardOutput();
    ffmpegAvailable = ffmpegHelp.contains("-s ");
    ffmpegHelp = tr("Video recording uses ffmpeg:")+"\n\n"+ffmpegHelp;

    QDialog *d = new QDialog(recdlg);
    QVBoxLayout *l = new QVBoxLayout(d);
    QTextBrowser *b = new QTextBrowser(d);
    QFont f = b->font();
    f.setFamily("courier");
    b->setFont(f);
    b->setText(ffmpegHelp);
    l->addWidget(b);
    d->setLayout(l);
    ffmpegHelpWindow = d;
    connect(recdlg->ffmpegHelp,SIGNAL(clicked()), ffmpegHelpWindow, SLOT(show()));
}

void QDeclarativeViewer::setRecording(bool on)
{
    if (on == recordTimer.isActive())
        return;

    int period = int(1000/record_rate+0.5);
#ifndef NO_PRIVATE_HEADERS
    QUnifiedTimer::instance()->setTimingInterval(on ? period:16);
    QUnifiedTimer::instance()->setConsistentTiming(on);
#endif
    if (on) {
        canvas->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        recordTimer.setInterval(period);
        recordTimer.start();
        frame_fmt = record_file.right(4).toLower();
        frame = QImage(canvas->width(),canvas->height(),QImage::Format_RGB32);
        if (frame_fmt != ".png" && (!convertAvailable || frame_fmt != ".gif")) {
            // Stream video to ffmpeg

            QProcess *proc = new QProcess(this);
            connect(proc, SIGNAL(finished(int)), this, SLOT(ffmpegFinished(int)));
            frame_stream = proc;

            QStringList args;
            args << "-y";
            args << "-r" << QString::number(record_rate);
            args << "-f" << "rawvideo";
            args << "-pix_fmt" << (frame_fmt == ".gif" ? "rgb24" : "rgb32");
            args << "-s" << QString("%1x%2").arg(canvas->width()).arg(canvas->height());
            args << "-i" << "-";
            if (record_outsize.isValid()) {
                args << "-s" << QString("%1x%2").arg(record_outsize.width()).arg(record_outsize.height());
                args << "-aspect" << QString::number(double(canvas->width())/canvas->height());
            }
            args += record_args;
            args << record_file;
            proc->start("ffmpeg",args);

        } else {
            // Store frames, save to GIF/PNG
            frame_stream = 0;
        }
    } else {
        canvas->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
        recordTimer.stop();
        if (frame_stream) {
            qDebug() << "Saving video...";
            frame_stream->close();
            qDebug() << "Wrote" << record_file;
        } else {
            QProgressDialog progress(tr("Saving frames..."), tr("Cancel"), 0, frames.count()+10, this);
            progress.setWindowModality(Qt::WindowModal);

            int frame=0;
            QStringList inputs;
            qDebug() << "Saving frames...";

            QString framename;
            bool png_output = false;
            if (record_file.right(4).toLower()==".png") {
                if (record_file.contains('%'))
                    framename = record_file;
                else
                    framename = record_file.left(record_file.length()-4)+"%04d"+record_file.right(4);
                png_output = true;
            } else {
                framename = "tmp-frame%04d.png";
                png_output = false;
            }
            foreach (QImage* img, frames) {
                progress.setValue(progress.value()+1);
                if (progress.wasCanceled())
                    break;
                QString name;
                name.sprintf(framename.toLocal8Bit(),frame++);
                if (record_outsize.isValid())
                    *img = img->scaled(record_outsize,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
                if (record_dither=="ordered")
                    img->convertToFormat(QImage::Format_Indexed8,Qt::PreferDither|Qt::OrderedDither).save(name);
                else if (record_dither=="threshold")
                    img->convertToFormat(QImage::Format_Indexed8,Qt::PreferDither|Qt::ThresholdDither).save(name);
                else if (record_dither=="floyd")
                    img->convertToFormat(QImage::Format_Indexed8,Qt::PreferDither).save(name);
                else
                    img->save(name);
                inputs << name;
                delete img;
            }

            if (!progress.wasCanceled()) {
                if (png_output) {
                    framename.replace(QRegExp("%\\d*."),"*");
                    qDebug() << "Wrote frames" << framename;
                    inputs.clear(); // don't remove them
                } else {
                    // ImageMagick and gifsicle for GIF encoding
                    progress.setLabelText(tr("Converting frames to GIF file..."));
                    QStringList args;
                    args << "-delay" << QString::number(period/10);
                    args << inputs;
                    args << record_file;
                    qDebug() << "Converting..." << record_file << "(this may take a while)";
                    if (0!=QProcess::execute("convert", args)) {
                        qWarning() << "Cannot run ImageMagick 'convert' - recorded frames not converted";
                        inputs.clear(); // don't remove them
                        qDebug() << "Wrote frames tmp-frame*.png";
                    } else {
                        if (record_file.right(4).toLower() == ".gif") {
                            qDebug() << "Compressing..." << record_file;
                            if (0!=QProcess::execute("gifsicle", QStringList() << "-O2" << "-o" << record_file << record_file))
                                qWarning() << "Cannot run 'gifsicle' - not compressed";
                        }
                        qDebug() << "Wrote" << record_file;
                    }
                }
            }

            progress.setValue(progress.maximum()-1);
            foreach (const QString &name, inputs)
                QFile::remove(name);

            frames.clear();
        }
    }
    qDebug() << "Recording: " << (recordTimer.isActive()?"ON":"OFF");
}

void QDeclarativeViewer::ffmpegFinished(int code)
{
    qDebug() << "ffmpeg returned" << code << frame_stream->readAllStandardError();
}

void QDeclarativeViewer::appAboutToQuit()
{
    // avoid QGLContext errors about invalid contexts on exit
    canvas->setViewport(0);

    // avoid crashes if messages are received after app has closed
    delete loggerWindow;
    loggerWindow = 0;
    delete tester;
    tester = 0;
}

void QDeclarativeViewer::autoStartRecording()
{
    setRecording(true);
    autoStopTimer.setInterval(record_autotime);
    autoStopTimer.start();
}

void QDeclarativeViewer::autoStopRecording()
{
    setRecording(false);
}

void QDeclarativeViewer::recordFrame()
{
    canvas->QWidget::render(&frame);
    if (frame_stream) {
        if (frame_fmt == ".gif") {
            // ffmpeg can't do 32bpp with gif
            QImage rgb24 = frame.convertToFormat(QImage::Format_RGB888);
            frame_stream->write((char*)rgb24.bits(),rgb24.numBytes());
        } else {
            frame_stream->write((char*)frame.bits(),frame.numBytes());
        }
    } else {
        frames.append(new QImage(frame));
    }
}

void QDeclarativeViewer::changeOrientation(QAction *action)
{
    if (!action)
        return;
    QString o = action->text();
    action->setChecked(true);
#if defined(Q_WS_S60)
    CAknAppUi *appUi = static_cast<CAknAppUi *>(CEikonEnv::Static()->AppUi());
    if (appUi) {
        CAknAppUi::TAppUiOrientation orientation = appUi->Orientation();
        if (o == QLatin1String("Auto-orientation")) {
            appUi->SetOrientationL(CAknAppUi::EAppUiOrientationAutomatic);
            rotateAction->setVisible(false);
        } else if (o == QLatin1String("Portrait")) {
            appUi->SetOrientationL(CAknAppUi::EAppUiOrientationPortrait);
            rotateAction->setVisible(true);
        } else if (o == QLatin1String("Landscape")) {
            appUi->SetOrientationL(CAknAppUi::EAppUiOrientationLandscape);
            rotateAction->setVisible(true);
        }
    }
#else
    if (o == QLatin1String("Portrait"))
        DeviceOrientation::instance()->setOrientation(DeviceOrientation::Portrait);
    else if (o == QLatin1String("Landscape"))
        DeviceOrientation::instance()->setOrientation(DeviceOrientation::Landscape);
    else if (o == QLatin1String("Portrait (inverted)"))
        DeviceOrientation::instance()->setOrientation(DeviceOrientation::PortraitInverted);
    else if (o == QLatin1String("Landscape (inverted)"))
        DeviceOrientation::instance()->setOrientation(DeviceOrientation::LandscapeInverted);
#endif
}

void QDeclarativeViewer::orientationChanged()
{
    updateSizeHints();
}

void QDeclarativeViewer::animationSpeedChanged(qreal factor)
{
    foreach (QAction *action, playSpeedMenuActions->actions()) {
        if (action->data().toFloat() == factor) {
            action->setChecked(true);
            break;
        }
    }
}

void QDeclarativeViewer::setDeviceKeys(bool on)
{
    devicemode = on;
}

void QDeclarativeViewer::setNetworkCacheSize(int size)
{
    namFactory->setCacheSize(size);
}

void QDeclarativeViewer::setUseGL(bool useGL)
{
#ifdef GL_SUPPORTED
    if (useGL) {
        QGLFormat format = QGLFormat::defaultFormat();
#ifdef Q_OS_MAC
        format.setSampleBuffers(true);
#else
        format.setSampleBuffers(false);
#endif

        QGLWidget *glWidget = new QGLWidget(format);
        //### potentially faster, but causes junk to appear if top-level is Item, not Rectangle
        //glWidget->setAutoFillBackground(false);

        canvas->setViewport(glWidget);
    }
#else
    Q_UNUSED(useGL)
#endif
}

void QDeclarativeViewer::setUseNativeFileBrowser(bool use)
{
    useQmlFileBrowser = !use;
}

void QDeclarativeViewer::setSizeToView(bool sizeToView)
{
    QDeclarativeView::ResizeMode resizeMode = sizeToView ? QDeclarativeView::SizeRootObjectToView : QDeclarativeView::SizeViewToRootObject;
    if (resizeMode != canvas->resizeMode()) {
        canvas->setResizeMode(resizeMode);
        updateSizeHints();
    }
}

void QDeclarativeViewer::setStayOnTop(bool stayOnTop)
{
    appOnTopAction->setChecked(stayOnTop);
}

void QDeclarativeViewer::updateSizeHints(bool initial)
{
    static bool isRecursive = false;

    if (isRecursive)
        return;
    isRecursive = true;

    if (initial || (canvas->resizeMode() == QDeclarativeView::SizeViewToRootObject)) {
        QSize newWindowSize = initial ? initialSize : canvas->sizeHint();
        //qWarning() << "USH:" << (initial ? "INIT:" : "V2R:") << "setting fixed size " << newWindowSize;
        if (!isFullScreen() && !isMaximized()) {
            canvas->setFixedSize(newWindowSize);
            resize(1, 1);
            layout()->setSizeConstraint(QLayout::SetFixedSize);
            layout()->activate();
        }
    }
    //qWarning() << "USH: R2V: setting free size ";
    layout()->setSizeConstraint(QLayout::SetNoConstraint);
    layout()->activate();
    setMinimumSize(minimumSizeHint());
    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    canvas->setMinimumSize(QSize(0,0));
    canvas->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    isRecursive = false;
}

void QDeclarativeViewer::registerTypes()
{
    static bool registered = false;

    if (!registered) {
        // registering only for exposing the DeviceOrientation::Orientation enum
        qmlRegisterUncreatableType<DeviceOrientation>("Qt",4,7,"Orientation","");
        qmlRegisterUncreatableType<DeviceOrientation>("QtQuick",1,0,"Orientation","");
        registered = true;
    }
}

QT_END_NAMESPACE

#include "qmlruntime.moc"
