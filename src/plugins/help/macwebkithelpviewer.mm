// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH
// Qt-GPL-exception-1.0

#include "macwebkithelpviewer.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QHelpEngine>
#include <QStyle>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

#include <QDebug>

#import <AppKit/NSMenuItem.h>
#import <WebKit/WebKit.h>

using namespace Utils;

// -- Helpers --

static int mainScreenHeight()
{
    NSRect screenFrame = [[[NSScreen screens] firstObject] frame];
    return screenFrame.size.height;
}

static QPoint flipPoint(const NSPoint &p)
{
    return QPoint(p.x, mainScreenHeight() - p.y);
}

// Synchronous JS evaluation via run-loop pumping. Only call from the main
// thread.
static id evaluateJSSynchronously(WKWebView *webView, NSString *js)
{
    __block id result = nil;
    __block BOOL done = NO;
    [webView evaluateJavaScript:js
              completionHandler:^(id value, NSError *) {
                  result = [value retain];
                  done = YES;
              }];
    while (!done)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    return [result autorelease];
}

static BOOL findStringSynchronously(WKWebView *webView, NSString *string, WKFindConfiguration *config)
{
    __block BOOL found = NO;
    __block BOOL done = NO;
    [webView findString:string
        withConfiguration:config
        completionHandler:^(WKFindResult *result) {
            found = result.matchFound;
            done = YES;
        }];
    while (!done) {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    }
    return found;
}

// -- QtHelpSchemeHandler --

@interface QtHelpSchemeHandler : NSObject <WKURLSchemeHandler>
@end

@implementation QtHelpSchemeHandler

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask
{
    Q_UNUSED(webView)
    const QUrl url = QUrl::fromNSURL(urlSchemeTask.request.URL);
    Help::Internal::LocalHelpManager::HelpData data;
    Help::Internal::LocalHelpManager *helpManager = Help::Internal::LocalHelpManager::instance();

    // WKURLSchemeHandler is called on the main thread; use DirectConnection if
    // already there.
    const Qt::ConnectionType connectionType = (QThread::currentThread() == helpManager->thread())
                                                  ? Qt::DirectConnection
                                                  : Qt::BlockingQueuedConnection;
    QMetaObject::invokeMethod(
        helpManager,
        "helpData",
        connectionType,
        Q_RETURN_ARG(Help::Internal::LocalHelpManager::HelpData, data),
        Q_ARG(QUrl, url));

    NSURL *resolvedURL = data.resolvedUrl.toNSURL();
    NSData *nsdata = data.data.toNSData();
    NSString *mimeType = data.mimeType.toNSString();
    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:resolvedURL
                                                        MIMEType:mimeType
                                           expectedContentLength:(NSInteger) nsdata.length
                                                textEncodingName:@"UTF8"];
    [urlSchemeTask didReceiveResponse:response];
    [urlSchemeTask didReceiveData:nsdata];
    [urlSchemeTask didFinish];
    [response release];
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask
{
    Q_UNUSED(webView)
    Q_UNUSED(urlSchemeTask)
}

@end

// -- NavigationDelegate --

@interface NavigationDelegate : NSObject <WKNavigationDelegate> {
    Help::Internal::MacWebKitHelpViewer *m_viewer;
    bool m_finished;
}
- (id)initWithViewer:(Help::Internal::MacWebKitHelpViewer *)viewer;
@end

@implementation NavigationDelegate

- (id)initWithViewer:(Help::Internal::MacWebKitHelpViewer *)viewer
{
    self = [super init];
    if (self) {
        m_viewer = viewer;
        m_finished = false;
    }
    return self;
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    Q_UNUSED(webView)
    Q_UNUSED(navigation)
    m_finished = false;
    m_viewer->slotLoadStarted();
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    Q_UNUSED(webView)
    Q_UNUSED(navigation)
    if (!m_finished) {
        m_finished = true;
        m_viewer->titleChanged();
        m_viewer->slotLoadFinished();
    }
}

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error
{
    Q_UNUSED(webView)
    Q_UNUSED(navigation)
    Q_UNUSED(error)
    if (!m_finished) {
        m_finished = true;
        m_viewer->slotLoadFinished();
    }
}

- (void)webView:(WKWebView *)webView
    didFailProvisionalNavigation:(WKNavigation *)navigation
                       withError:(NSError *)error
{
    Q_UNUSED(webView)
    Q_UNUSED(navigation)
    Q_UNUSED(error)
    if (!m_finished) {
        m_finished = true;
        m_viewer->slotLoadFinished();
    }
}

@end

// -- HelpWebView --
// WKWebView subclass that intercepts the context menu to inject custom link
// actions.

@interface HelpWebView : WKWebView
@property(nonatomic, copy) NSString *hoveredLinkURL;
@property(nonatomic, assign) Help::Internal::MacWebKitHelpWidget *helpWidget;
@end

@implementation HelpWebView

- (void)dealloc
{
    [_hoveredLinkURL release];
    [super dealloc];
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    [super willOpenMenu:menu withEvent:event];

    if (self.helpWidget) {
        self.helpWidget->hideToolTip();
        self.helpWidget->setContextMenuOpen(true);
    }

    NSString *urlString = self.hoveredLinkURL;
    if (!urlString || urlString.length == 0)
        return;
    NSURL *url = [NSURL URLWithString:urlString];
    if (!url)
        return;

    Help::Internal::MacWebKitHelpWidget *widget = self.helpWidget;
    if (!widget)
        return;

    if (widget->viewer()->isActionVisible(Help::Internal::HelpViewer::Action::NewPage)) {
        NSMenuItem *item = [[NSMenuItem alloc]
            initWithTitle:Help::Tr::tr(Help::Constants::TR_OPEN_LINK_AS_NEW_PAGE).toNSString()
                   action:@selector(openAsNewPage:)
            keyEquivalent:@""];
        item.representedObject = url;
        item.target = self;
        [menu insertItem:[item autorelease] atIndex:0];
    }
}

- (void)didCloseMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
    [super didCloseMenu:menu withEvent:event];
    if (self.helpWidget)
        self.helpWidget->setContextMenuOpen(false);
}

- (void)openAsNewPage:(NSMenuItem *)item
{
    if (self.helpWidget)
        self.helpWidget->viewer()->newPageRequested(QUrl::fromNSURL(item.representedObject));
}

@end

// -- UIDelegate (WKUIDelegate + WKScriptMessageHandler) --

@interface UIDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler> {
    Help::Internal::MacWebKitHelpWidget *m_widget;
}
- (id)initWithWidget:(Help::Internal::MacWebKitHelpWidget *)widget;
@end

@implementation UIDelegate

- (id)initWithWidget:(Help::Internal::MacWebKitHelpWidget *)widget
{
    self = [super init];
    if (self)
        m_widget = widget;
    return self;
}

// Intercept target="_blank" and similar new-window navigations.
- (WKWebView *)webView:(WKWebView *)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration
               forNavigationAction:(WKNavigationAction *)navigationAction
                    windowFeatures:(WKWindowFeatures *)windowFeatures
{
    Q_UNUSED(webView)
    Q_UNUSED(configuration)
    Q_UNUSED(windowFeatures)
    m_widget->viewer()->externalPageRequested(QUrl::fromNSURL(navigationAction.request.URL));
    return nil;
}

// Hover messages from the injected JS mousemove script.
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message
{
    Q_UNUSED(userContentController)
    if (![message.name isEqualToString:@"linkHover"])
        return;

    NSString *urlString = [message.body isKindOfClass:[NSString class]] ? message.body : @"";
    HelpWebView *helpWebView = (HelpWebView *) m_widget->webView();
    helpWebView.hoveredLinkURL = urlString;

    if (urlString.length == 0) {
        m_widget->hideToolTip();
        return;
    }
    if ((NSEvent.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask) != 0) {
        m_widget->hideToolTip();
        return;
    }
    m_widget->startToolTipTimer(flipPoint(NSEvent.mouseLocation), QString::fromNSString(urlString));
}

@end

namespace Help {
namespace Internal {

// -- MacWebKitHelpWidgetPrivate --

class MacWebKitHelpWidgetPrivate
{
public:
    explicit MacWebKitHelpWidgetPrivate(MacWebKitHelpViewer *parent)
        : m_viewer(parent)
    {}

    ~MacWebKitHelpWidgetPrivate()
    {
        [m_webView removeFromSuperview];
        [m_webView.configuration.userContentController
            removeScriptMessageHandlerForName:@"linkHover"];
        m_webView.navigationDelegate = nil;
        m_webView.UIDelegate = nil;
        [m_webView release];
        [m_navigationDelegate release];
        [m_uiDelegate release];
    }

    MacWebKitHelpViewer *m_viewer;
    HelpWebView *m_webView = nil;
    NavigationDelegate *m_navigationDelegate = nil;
    UIDelegate *m_uiDelegate = nil;
    QTimer m_toolTipTimer;
    QPoint m_toolTipPos;
    QString m_toolTipText;
    bool m_contextMenuOpen = false;
};

// -- MacWebKitHelpWidget --

MacWebKitHelpWidget::MacWebKitHelpWidget(MacWebKitHelpViewer *parent)
    : QWidget(parent)
    , d(new MacWebKitHelpWidgetPrivate(parent))
{
    // WA_NativeWindow gives this widget its own QNSView so the WKWebView can
    // be embedded as a direct subview rather than a child NSWindow (which would
    // always float above all other widgets regardless of Z-order).
    setAttribute(Qt::WA_NativeWindow);
    d->m_toolTipTimer.setSingleShot(true);
    connect(&d->m_toolTipTimer, &QTimer::timeout, this, &MacWebKitHelpWidget::showToolTip);

    @autoreleasepool {
        WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

        QtHelpSchemeHandler *schemeHandler = [[QtHelpSchemeHandler alloc] init];
        [config setURLSchemeHandler:schemeHandler forURLScheme:@"qthelp"];
        [schemeHandler release];

        // Inject a script that tracks mouse-hovered link URLs and reports them
        // via the "linkHover" message handler. Only posts when the hovered URL
        // changes.
        NSString *hoverScriptSource
            = @"(function(){"
               "var last='';"
               "document.addEventListener('mousemove',function(e){"
               "  var el=e.target,href='';"
               "  while (el&&el!==document.documentElement){"
               "    "
               "if (el.tagName&&el.tagName.toUpperCase()==='A'&&el.href){href=el."
               "href;break;}"
               "    el=el.parentElement;"
               "  }"
               "  if (href!==last){last=href;"
               "    window.webkit.messageHandlers.linkHover.postMessage(href);}"
               "},false);"
               "})();";
        WKUserScript *hoverScript = [[WKUserScript alloc]
              initWithSource:hoverScriptSource
               injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
            forMainFrameOnly:YES];
        [config.userContentController addUserScript:hoverScript];
        [hoverScript release];

        // UIDelegate must be created before the webview so we can add the script
        // message handler to the configuration prior to initialisation.
        d->m_uiDelegate = [[UIDelegate alloc] initWithWidget:this];
        [config.userContentController addScriptMessageHandler:d->m_uiDelegate name:@"linkHover"];

        d->m_webView = [[HelpWebView alloc] initWithFrame:CGRectZero configuration:config];
        [config release];

        d->m_webView.wantsLayer = YES;
        d->m_webView.helpWidget = this;

        d->m_navigationDelegate = [[NavigationDelegate alloc] initWithViewer:parent];
        d->m_webView.navigationDelegate = d->m_navigationDelegate;
        d->m_webView.UIDelegate = d->m_uiDelegate;
        // WKWebView is added as the bottommost subview of this widget's QNSView
        // in updateWebViewFrame(), called once the native handle is available.
    }
}

MacWebKitHelpWidget::~MacWebKitHelpWidget()
{
    delete d;
}

void MacWebKitHelpWidget::updateWebViewFrame()
{
    if (!d->m_webView)
        return;
    WId handle = internalWinId();
    if (!handle)
        return;
    @autoreleasepool {
        NSView *parentView = reinterpret_cast<NSView *>(handle);
        if (d->m_webView.superview != parentView) {
            // Add behind all existing subviews so Qt-native sibling widgets
            // rendered into this view's layer can appear on top.
            [parentView addSubview:d->m_webView positioned:NSWindowBelow relativeTo:nil];
        }
        // Qt's QNSView has isFlipped=YES (top-left origin), so (0,0,w,h) fills
        // the widget area correctly without coordinate-space conversion.
        d->m_webView.frame = CGRectMake(0, 0, width(), height());
    }
}

void MacWebKitHelpWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    updateWebViewFrame();
}

bool MacWebKitHelpWidget::event(QEvent *e)
{
    if (e->type() == QEvent::WinIdChange)
        updateWebViewFrame();
    return QWidget::event(e);
}

MacWebKitHelpViewer *MacWebKitHelpWidget::viewer() const
{
    return d->m_viewer;
}

WKWebView *MacWebKitHelpWidget::webView() const
{
    return d->m_webView;
}

void MacWebKitHelpWidget::startToolTipTimer(const QPoint &pos, const QString &text)
{
    if (d->m_contextMenuOpen)
        return;
    int delay = style()->styleHint(QStyle::SH_ToolTip_WakeUpDelay, nullptr, this, nullptr);
    d->m_toolTipPos = pos;
    d->m_toolTipText = text;
    d->m_toolTipTimer.start(delay);
}

void MacWebKitHelpWidget::setContextMenuOpen(bool open)
{
    d->m_contextMenuOpen = open;
}

void MacWebKitHelpWidget::hideToolTip()
{
    d->m_toolTipTimer.stop();
    QToolTip::showText(QPoint(), QString());
}

void MacWebKitHelpWidget::hideEvent(QHideEvent *)
{
    [d->m_webView setHidden:YES];
}

void MacWebKitHelpWidget::showEvent(QShowEvent *)
{
    [d->m_webView setHidden:NO];
    updateWebViewFrame();
}

void MacWebKitHelpWidget::showToolTip()
{
    QToolTip::showText(d->m_toolTipPos, d->m_toolTipText, this);
}

// -- MacWebKitHelpViewer --

static void responderHack(QWidget *old, QWidget *now)
{
    // On focus change Qt does not make the QNSView firstResponder. When the
    // native NSView is embedded into a Qt hierarchy and focus is set from code
    // (not mouse click), Cocoa needs a nudge.
    Q_UNUSED(old)
    if (!now)
        return;
    @autoreleasepool {
        NSView *view;
        if (auto viewContainer = qobject_cast<MacWebKitHelpWidget *>(now))
            view = viewContainer->webView();
        else
            view = reinterpret_cast<NSView *>(now->effectiveWinId());
        [view.window makeFirstResponder:view];
    }
}

MacWebKitHelpViewer::MacWebKitHelpViewer(QWidget *parent)
    : HelpViewer(parent)
    , m_widget(new MacWebKitHelpWidget(this))
{
    static bool responderHackInstalled = false;
    if (!responderHackInstalled) {
        responderHackInstalled = true;
        QObject::connect(qApp, &QApplication::focusChanged, &responderHack);
    }

    @autoreleasepool {
        QVBoxLayout *layout = new QVBoxLayout;
        setLayout(layout);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_widget, 10);
    }
}

MacWebKitHelpViewer::~MacWebKitHelpViewer() {}

void MacWebKitHelpViewer::setViewerFont(const QFont &font)
{
    m_viewerFont = font;
    m_fontSet = true;
    qWarning() << "MacWebKitHelpViewer: setViewerFont() is not supported on macOS";
}

void MacWebKitHelpViewer::setScale(qreal scale)
{
    m_widget->webView().pageZoom = (scale <= 0.0 ? 1.0 : scale);
}

QString MacWebKitHelpViewer::title() const
{
    @autoreleasepool {
        return QString::fromNSString(m_widget->webView().title);
    }
}

QUrl MacWebKitHelpViewer::source() const
{
    @autoreleasepool {
        NSURL *url = m_widget->webView().URL;
        if (url)
            return QUrl::fromNSURL(url);
        return m_pendingUrl;
    }
}

void MacWebKitHelpViewer::setSource(const QUrl &url)
{
    @autoreleasepool {
        m_pendingUrl = url;
        [m_widget->webView() loadRequest:[NSURLRequest requestWithURL:url.toNSURL()]];
    }
}

void MacWebKitHelpViewer::scrollToAnchor(const QString &anchor)
{
    QUrl url = source();
    url.setFragment(anchor);
    setSource(url);
}

void MacWebKitHelpViewer::setHtml(const QString &html)
{
    @autoreleasepool {
        [m_widget->webView() loadHTMLString:html.toNSString()
                                    baseURL:[NSURL URLWithString:@"about:blank"]];
    }
}

QString MacWebKitHelpViewer::selectedText() const
{
    @autoreleasepool {
        id result
            = evaluateJSSynchronously(m_widget->webView(), @"window.getSelection().toString()");
        return QString::fromNSString([result isKindOfClass:[NSString class]] ? result : @"");
    }
}

bool MacWebKitHelpViewer::isForwardAvailable() const
{
    @autoreleasepool {
        return m_widget->webView().canGoForward;
    }
}

bool MacWebKitHelpViewer::isBackwardAvailable() const
{
    @autoreleasepool {
        return m_widget->webView().canGoBack;
    }
}

void MacWebKitHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    @autoreleasepool {
        WKBackForwardList *list = m_widget->webView().backForwardList;
        int count = (int) list.backList.count;
        for (int i = 0; i < count; ++i) {
            int historyIndex = -(i + 1);
            WKBackForwardListItem *item = [list itemAtIndex:historyIndex];
            QAction *action = new QAction(backMenu);
            action->setText(QString::fromNSString(item.title));
            action->setData(historyIndex);
            connect(action, &QAction::triggered, this, &MacWebKitHelpViewer::goToHistoryItem);
            backMenu->addAction(action);
        }
    }
}

void MacWebKitHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    @autoreleasepool {
        WKBackForwardList *list = m_widget->webView().backForwardList;
        int count = (int) list.forwardList.count;
        for (int i = 0; i < count; ++i) {
            int historyIndex = i + 1;
            WKBackForwardListItem *item = [list itemAtIndex:historyIndex];
            QAction *action = new QAction(forwardMenu);
            action->setText(QString::fromNSString(item.title));
            action->setData(historyIndex);
            connect(action, &QAction::triggered, this, &MacWebKitHelpViewer::goToHistoryItem);
            forwardMenu->addAction(action);
        }
    }
}

bool MacWebKitHelpViewer::findText(
    const QString &text, FindFlags flags, bool incremental, bool fromSearch, bool *wrapped)
{
    Q_UNUSED(incremental)
    Q_UNUSED(fromSearch)
    @autoreleasepool {
        if (wrapped)
            *wrapped = false;
        if (text.isEmpty())
            return false;

        WKFindConfiguration *config = [[WKFindConfiguration alloc] init];
        config.backwards = (flags & FindBackward) ? YES : NO;
        config.caseSensitive = (flags & FindCaseSensitively) ? YES : NO;
        config.wraps = NO;

        BOOL found = findStringSynchronously(m_widget->webView(), text.toNSString(), config);
        [config release];
        if (found)
            return true;

        config = [[WKFindConfiguration alloc] init];
        config.backwards = (flags & FindBackward) ? YES : NO;
        config.caseSensitive = (flags & FindCaseSensitively) ? YES : NO;
        config.wraps = YES;

        found = findStringSynchronously(m_widget->webView(), text.toNSString(), config);
        [config release];
        if (found) {
            if (wrapped)
                *wrapped = true;
            return true;
        }
        return false;
    }
}

void MacWebKitHelpViewer::copy()
{
    QApplication::clipboard()->setText(selectedText());
}

void MacWebKitHelpViewer::stop()
{
    [m_widget->webView() stopLoading];
}

void MacWebKitHelpViewer::forward()
{
    @autoreleasepool {
        [m_widget->webView() goForward];
        emit forwardAvailable(isForwardAvailable());
        emit backwardAvailable(isBackwardAvailable());
    }
}

void MacWebKitHelpViewer::backward()
{
    @autoreleasepool {
        [m_widget->webView() goBack];
        emit forwardAvailable(isForwardAvailable());
        emit backwardAvailable(isBackwardAvailable());
    }
}

void MacWebKitHelpViewer::print(QPrinter *printer)
{
    Q_UNUSED(printer)
}

void MacWebKitHelpViewer::slotLoadStarted()
{
    HelpViewer::slotLoadStarted();
}

void MacWebKitHelpViewer::slotLoadFinished()
{
    HelpViewer::slotLoadFinished();
    emit forwardAvailable(isForwardAvailable());
    emit backwardAvailable(isBackwardAvailable());
}

void MacWebKitHelpViewer::goToHistoryItem()
{
    @autoreleasepool {
        QAction *action = qobject_cast<QAction *>(sender());
        QTC_ASSERT(action, return);
        bool ok = false;
        int index = action->data().toInt(&ok);
        QTC_ASSERT(ok, return);
        WKBackForwardList *list = m_widget->webView().backForwardList;
        WKBackForwardListItem *item = [list itemAtIndex:index];
        QTC_ASSERT(item, return);
        [m_widget->webView() goToBackForwardListItem:item];
        emit forwardAvailable(isForwardAvailable());
        emit backwardAvailable(isBackwardAvailable());
    }
}

} // namespace Internal
} // namespace Help
