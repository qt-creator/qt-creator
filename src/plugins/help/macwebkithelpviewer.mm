/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "macwebkithelpviewer.h"

#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/icore.h>
#include <utils/autoreleasepool.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QClipboard>
#include <QHelpEngine>
#include <QStyle>
#include <QTimer>
#include <QtMac>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

#include <QDebug>

#import <AppKit/NSMenuItem.h>
#import <Foundation/NSURLProtocol.h>
#import <Foundation/NSURLResponse.h>
#import <WebKit/DOMDocument.h>
#import <WebKit/DOMElement.h>
#import <WebKit/DOMHTMLElement.h>
#import <WebKit/DOMNodeFilter.h>
#import <WebKit/DOMNodeIterator.h>
#import <WebKit/DOMRange.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDocument.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebFrameLoadDelegate.h>
#import <WebKit/WebFrameView.h>
#import <WebKit/WebHistoryItem.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebUIDelegate.h>
#import <WebKit/WebView.h>

using namespace Utils;

// #pragma mark -- mac helpers

// copy from qcocoahelpers.mm
static int mainScreenHeight()
{
    // The first screen in the screens array is documented
    // to have the (0,0) origin.
    NSRect screenFrame = [[[NSScreen screens] firstObject] frame];
    return screenFrame.size.height;
}

static QPoint flipPoint(const NSPoint &p)
{
    return QPoint(p.x, mainScreenHeight() - p.y);
}

// #pragma mark -- DOMNodeIterator (PrivateExtensions)

@interface DOMNodeIterator (PrivateExtensions)

- (BOOL)findNode:(DOMNode *)node;
- (DOMNode *)nextTextNode;
- (DOMNode *)previousTextNode;
- (DOMNode *)gotoEnd;

@end

@implementation DOMNodeIterator (PrivateExtensions)

- (BOOL)findNode:(DOMNode *)node
{
    while (DOMNode *next = [self nextNode]) {
        if (next == node)
            return YES;
    }
    return NO;
}

- (DOMNode *)nextTextNode
{
    DOMNode *node = nil;
    do {
        node = [self nextNode];
    } while (node && node.nodeType != DOM_TEXT_NODE);
    return node;
}

- (DOMNode *)previousTextNode
{
    DOMNode *node = nil;
    do {
        node = [self previousNode];
    } while (node && node.nodeType != DOM_TEXT_NODE);
    return node;
}

- (DOMNode *)gotoEnd
{
    DOMNode *previous = nil;
    DOMNode *node = nil;
    do {
        previous = node;
        node = [self nextNode];
    } while (node);
    return previous;
}

@end

// #pragma mark -- QtHelpURLProtocol

@interface QtHelpURLProtocol : NSURLProtocol
{
}

+ (BOOL)canInitWithRequest:(NSURLRequest *)request;

@end

@implementation QtHelpURLProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
    return [[[request URL] scheme] isEqualToString:@"qthelp"];
}

+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
    return request;
}

- (void)startLoading
{
    const QUrl &url = QUrl::fromNSURL(self.request.URL);
    Help::Internal::LocalHelpManager::HelpData data;
    Help::Internal::LocalHelpManager *helpManager = Help::Internal::LocalHelpManager::instance();

    QMetaObject::invokeMethod(helpManager, "helpData", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(Help::Internal::LocalHelpManager::HelpData, data),
                              Q_ARG(QUrl, url));

    NSURL *resolvedURL = data.resolvedUrl.toNSURL();
    NSString *mimeType = data.mimeType.toNSString();
    NSData *nsdata = QtMac::toNSData(data.data); // Qt 5.3 has this in QByteArray
    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:resolvedURL
                                                        MIMEType:mimeType
                                           expectedContentLength:data.data.length()
                                                textEncodingName:@"UTF8"];
    [self.client URLProtocol:self didReceiveResponse:response
                                  cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    [self.client URLProtocol:self didLoadData:nsdata];
    [self.client URLProtocolDidFinishLoading:self];
    [response release];
}

- (void)stopLoading
{
}

@end

static void ensureProtocolHandler()
{
    static bool registered = false;
    if (!registered) {
        [NSURLProtocol registerClass:[QtHelpURLProtocol class]];
        registered = true;
    }
}

// #pragma mark -- FrameLoadDelegate

@interface FrameLoadDelegate : NSObject
{
    WebFrame *mainFrame;
    Help::Internal::MacWebKitHelpViewer *viewer;
}

- (id)initWithMainFrame:(WebFrame *)frame viewer:(Help::Internal::MacWebKitHelpViewer *)viewer;
- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didReceiveTitle:(NSString *)title forFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame;

@end

@implementation FrameLoadDelegate

- (id)initWithMainFrame:(WebFrame *)frame viewer:(Help::Internal::MacWebKitHelpViewer *)helpViewer
{
    self = [super init];
    if (self) {
        mainFrame = frame;
        viewer = helpViewer;
    }
    return self;
}

- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame
{
    Q_UNUSED(sender)
    if (frame == mainFrame)
        viewer->slotLoadStarted();
}

- (void)webView:(WebView *)sender didReceiveTitle:(NSString *)title forFrame:(WebFrame *)frame
{
    Q_UNUSED(sender)
    Q_UNUSED(title)
    if (frame == mainFrame)
        viewer->titleChanged();
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    Q_UNUSED(sender)
    if (frame == mainFrame)
        viewer->slotLoadFinished();
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    Q_UNUSED(sender)
    Q_UNUSED(error)
    if (frame == mainFrame)
        viewer->slotLoadFinished();
}

@end

// #pragma mark -- UIDelegate

@interface UIDelegate : NSObject
{
    Help::Internal::MacWebKitHelpWidget *widget;
}

@property (assign) BOOL openInNewPageActionVisible;

- (id)initWithWidget:(Help::Internal::MacWebKitHelpWidget *)theWidget;
- (void)webView:(WebView *)sender makeFirstResponder:(NSResponder *)responder;
- (NSArray *)webView:(WebView *)sender contextMenuItemsForElement:(NSDictionary *)element
    defaultMenuItems:(NSArray *)defaultMenuItems;
- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request;
- (void)webView:(WebView *)sender mouseDidMoveOverElement:(NSDictionary *)elementInformation
    modifierFlags:(NSUInteger)modifierFlags;
@end

@implementation UIDelegate

- (id)initWithWidget:(Help::Internal::MacWebKitHelpWidget *)theWidget
{
    self = [super init];
    if (self) {
        widget = theWidget;
        self.openInNewPageActionVisible = YES;
    }
    return self;
}

- (void)webView:(WebView *)sender makeFirstResponder:(NSResponder *)responder
{
    // make the widget get focus
    if (responder) {
        widget->setFocus();
        [sender.window makeFirstResponder:responder];
    }
}

- (NSArray *)webView:(WebView *)sender contextMenuItemsForElement:(NSDictionary *)element
    defaultMenuItems:(NSArray *)defaultMenuItems
{
    Q_UNUSED(sender)
    Q_UNUSED(element)
    NSMutableArray *ret = [[NSMutableArray alloc] init];
    for (NSMenuItem *item in defaultMenuItems) {
        switch (item.tag) {
        case WebMenuItemTagCopyLinkToClipboard:
        case WebMenuItemTagCopyImageToClipboard:
        case WebMenuItemTagCopy:
        case WebMenuItemTagGoBack:
        case WebMenuItemTagGoForward:
        case WebMenuItemTagStop:
        case WebMenuItemTagReload:
        case WebMenuItemTagOther:
        case WebMenuItemTagSearchInSpotlight:
        case WebMenuItemTagSearchWeb:
        case WebMenuItemTagLookUpInDictionary:
        case WebMenuItemTagOpenWithDefaultApplication:
            [ret addObject:item];
            break;
        case WebMenuItemTagOpenLinkInNewWindow:
        case WebMenuItemTagOpenImageInNewWindow:
            if (self.openInNewPageActionVisible) {
                item.title = QCoreApplication::translate("HelpViewer", "Open Link as New Page").toNSString();
                [ret addObject:item];
            }
        default:
            break;
        }
    }
    return [ret autorelease];
}

- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request
{
    Q_UNUSED(sender)
    Q_UNUSED(request)
    Help::Internal::MacWebKitHelpViewer* viewer
            = static_cast<Help::Internal::MacWebKitHelpViewer *>(
                Help::Internal::OpenPagesManager::instance().createPage(QUrl()));
    return viewer->widget()->webView();
}

- (void)webView:(WebView *)sender mouseDidMoveOverElement:(NSDictionary *)elementInformation
    modifierFlags:(NSUInteger)modifierFlags
{
    Q_UNUSED(sender)
    if (!elementInformation || (modifierFlags & NSDeviceIndependentModifierFlagsMask) != 0) {
        widget->hideToolTip();
        return;
    }
    NSURL *url = [elementInformation objectForKey:WebElementLinkURLKey];
    if (!url) {
        widget->hideToolTip();
        return;
    }
    widget->startToolTipTimer(flipPoint(NSEvent.mouseLocation),
                       QString::fromNSString(url.absoluteString));
}

@end

// #pragma mark -- MyWebView
@interface MyWebView : WebView
@end

// work around Qt + WebView issue QTBUG-26593, that Qt does not pass mouseMoved: events up the event chain,
// but the Web(HTML)View is only handling mouse moved for hovering etc if the event was passed up
// to the NSWindow (arguably a bug in Web(HTML)View)
@implementation MyWebView

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if (NSArray *trackingArray = [self trackingAreas]) {
        NSUInteger size = [trackingArray count];
        for (NSUInteger i = 0; i < size; ++i) {
            NSTrackingArea *t = [trackingArray objectAtIndex:i];
            [self removeTrackingArea:t];
        }
    }
    NSUInteger trackingOptions = NSTrackingActiveInActiveApp | NSTrackingInVisibleRect
            | NSTrackingMouseMoved;
    NSTrackingArea *ta = [[[NSTrackingArea alloc] initWithRect:[self frame]
                                                      options:trackingOptions
                                                        owner:self
                                                     userInfo:nil]
                                                                autorelease];
    [self addTrackingArea:ta];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
    [self.window mouseMoved:theEvent];
}

@end

namespace Help {
namespace Internal {

// #pragma mark -- MacWebKitHelpWidget

class MacWebKitHelpWidgetPrivate
{
public:
    MacWebKitHelpWidgetPrivate()
        : m_savedResponder(nil)
    {
    }

    ~MacWebKitHelpWidgetPrivate()
    {
        [m_webView release];
        [m_frameLoadDelegate release];
        [m_uiDelegate release];
    }

    WebView *m_webView;
    FrameLoadDelegate *m_frameLoadDelegate;
    UIDelegate *m_uiDelegate;
    NSResponder *m_savedResponder;
    QTimer m_toolTipTimer;
    QPoint m_toolTipPos;
    QString m_toolTipText;
};

// #pragma mark -- MacWebKitHelpWidget

MacWebKitHelpWidget::MacWebKitHelpWidget(MacWebKitHelpViewer *parent)
    : QMacCocoaViewContainer(0, parent),
      d(new MacWebKitHelpWidgetPrivate)
{
    d->m_toolTipTimer.setSingleShot(true);
    connect(&d->m_toolTipTimer, &QTimer::timeout, this, &MacWebKitHelpWidget::showToolTip);
    AutoreleasePool pool; Q_UNUSED(pool)
    d->m_webView = [[MyWebView alloc] init];
    // Turn layered rendering on.
    // Otherwise the WebView will render empty after any QQuickWidget was shown.
    d->m_webView.wantsLayer = YES;
    d->m_frameLoadDelegate = [[FrameLoadDelegate alloc] initWithMainFrame:d->m_webView.mainFrame
                                                                   viewer:parent];
    [d->m_webView setFrameLoadDelegate:d->m_frameLoadDelegate];
    d->m_uiDelegate = [[UIDelegate alloc] initWithWidget:this];
    [d->m_webView setUIDelegate:d->m_uiDelegate];

    setCocoaView(d->m_webView);
}

MacWebKitHelpWidget::~MacWebKitHelpWidget()
{
    delete d;
}

void MacWebKitHelpWidget::setOpenInNewPageActionVisible(bool visible)
{
    d->m_uiDelegate.openInNewPageActionVisible = visible;
}

WebView *MacWebKitHelpWidget::webView() const
{
    return d->m_webView;
}

void MacWebKitHelpWidget::startToolTipTimer(const QPoint &pos, const QString &text)
{
    int delay = style()->styleHint(QStyle::SH_ToolTip_WakeUpDelay, 0, this, 0);
    d->m_toolTipPos = pos;
    d->m_toolTipText = text;
    d->m_toolTipTimer.start(delay);
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
}

void MacWebKitHelpWidget::showToolTip()
{
    QToolTip::showText(d->m_toolTipPos, d->m_toolTipText, this);
}

// #pragma mark -- MacWebKitHelpViewer

MacWebKitHelpViewer::MacWebKitHelpViewer(QWidget *parent)
    : HelpViewer(parent),
      m_widget(new MacWebKitHelpWidget(this))
{
    static bool responderHackInstalled = false;
    if (!responderHackInstalled) {
        responderHackInstalled = true;
        new MacResponderHack(qApp);
    }

    AutoreleasePool pool; Q_UNUSED(pool)
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_widget, 10);
}

MacWebKitHelpViewer::~MacWebKitHelpViewer()
{

}

QFont MacWebKitHelpViewer::viewerFont() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    WebPreferences *preferences = m_widget->webView().preferences;
    QString family = QString::fromNSString([preferences standardFontFamily]);
    int size = [preferences defaultFontSize];
    return QFont(family, size);
}

void MacWebKitHelpViewer::setViewerFont(const QFont &font)
{
    AutoreleasePool pool; Q_UNUSED(pool)
    WebPreferences *preferences = m_widget->webView().preferences;
    [preferences setStandardFontFamily:font.family().toNSString()];
    [preferences setDefaultFontSize:font.pointSize()];
}

void MacWebKitHelpViewer::scaleUp()
{
    AutoreleasePool pool; Q_UNUSED(pool)
    m_widget->webView().textSizeMultiplier += 0.1;
}

void MacWebKitHelpViewer::scaleDown()
{
    AutoreleasePool pool; Q_UNUSED(pool)
    m_widget->webView().textSizeMultiplier = qMax(0.1, m_widget->webView().textSizeMultiplier - 0.1);
}

void MacWebKitHelpViewer::resetScale()
{
    AutoreleasePool pool; Q_UNUSED(pool)
    m_widget->webView().textSizeMultiplier = 1.0;
}

qreal MacWebKitHelpViewer::scale() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
            return m_widget->webView().textSizeMultiplier;
}

void MacWebKitHelpViewer::setScale(qreal scale)
{
    m_widget->webView().textSizeMultiplier = (scale <= 0.0 ? 1.0 : scale);
}

QString MacWebKitHelpViewer::title() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    return QString::fromNSString(m_widget->webView().mainFrameTitle);
}

QUrl MacWebKitHelpViewer::source() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    WebDataSource *dataSource = m_widget->webView().mainFrame.dataSource;
    if (!dataSource)
        dataSource = m_widget->webView().mainFrame.provisionalDataSource;
    return QUrl::fromNSURL(dataSource.request.URL);
}

void MacWebKitHelpViewer::setSource(const QUrl &url)
{
    AutoreleasePool pool; Q_UNUSED(pool)
    ensureProtocolHandler();
    [m_widget->webView().mainFrame loadRequest:[NSURLRequest requestWithURL:url.toNSURL()]];
}

void MacWebKitHelpViewer::scrollToAnchor(const QString &anchor)
{
    QUrl url = source();
    url.setFragment(anchor);
    setSource(url);
}

void MacWebKitHelpViewer::setHtml(const QString &html)
{
    AutoreleasePool pool; Q_UNUSED(pool)
    [m_widget->webView().mainFrame
            loadHTMLString:html.toNSString()
                   baseURL:[NSURL fileURLWithPath:Core::ICore::resourcePath().toNSString()]];
}

QString MacWebKitHelpViewer::selectedText() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    return QString::fromNSString(m_widget->webView().selectedDOMRange.text);
}

bool MacWebKitHelpViewer::isForwardAvailable() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    return m_widget->webView().canGoForward;
}

bool MacWebKitHelpViewer::isBackwardAvailable() const
{
    AutoreleasePool pool; Q_UNUSED(pool)
    return m_widget->webView().canGoBack;
}

void MacWebKitHelpViewer::addBackHistoryItems(QMenu *backMenu)
{
    AutoreleasePool pool; Q_UNUSED(pool)
    WebBackForwardList *list = m_widget->webView().backForwardList;
    int backListCount = list.backListCount;
    for (int i = 0; i < backListCount; ++i) {
        int historyIndex = -(i+1);
        QAction *action = new QAction(backMenu);
        action->setText(QString::fromNSString([list itemAtIndex:historyIndex].title));
        action->setData(historyIndex);
        connect(action, SIGNAL(triggered()), this, SLOT(goToHistoryItem()));
        backMenu->addAction(action);
    }
}

void MacWebKitHelpViewer::addForwardHistoryItems(QMenu *forwardMenu)
{
    AutoreleasePool pool; Q_UNUSED(pool)
    WebBackForwardList *list = m_widget->webView().backForwardList;
    int forwardListCount = list.forwardListCount;
    for (int i = 0; i < forwardListCount; ++i) {
        int historyIndex = i + 1;
        QAction *action = new QAction(forwardMenu);
        action->setText(QString::fromNSString([list itemAtIndex:historyIndex].title));
        action->setData(historyIndex);
        connect(action, SIGNAL(triggered()), this, SLOT(goToHistoryItem()));
        forwardMenu->addAction(action);
    }
}

void MacWebKitHelpViewer::setOpenInNewPageActionVisible(bool visible)
{
    m_widget->setOpenInNewPageActionVisible(visible);
}

DOMRange *MacWebKitHelpViewer::findText(NSString *text, bool forward, bool caseSensitive, DOMNode *startNode, int startOffset)
{
    QTC_ASSERT(text, return nil);
    if (text.length == 0)
        return nil;
    DOMDocument *document = m_widget->webView().mainFrame.DOMDocument;
    // search only the body
    DOMNodeIterator *iterator = [document createNodeIterator:document.body whatToShow:DOM_SHOW_ALL
            filter:nil expandEntityReferences:NO];

    DOMNode *selectionStart = nil;
    int selectionStartOffset = 0;
    DOMNode *currentNode = startNode;
    int currentOffset = startOffset;
    NSString *searchTerm = caseSensitive ? text : [text lowercaseString];
    int searchTermLength = searchTerm.length;
    int indexInSearchTerm = forward ? 0 : searchTerm.length - 1;
    if (!currentNode) { // search whole body from end
        if (forward)
            currentNode = document.body;
        else
            currentNode = [iterator gotoEnd];
    } else { // otherwise find the start node
        QTC_ASSERT([iterator findNode:currentNode], return nil);
    }
    if (!forward) { // findNode leaves iterator behind currentNode, we need to go back
        QTC_ASSERT([iterator previousNode] == currentNode, return nil);
    }
    if (currentNode.nodeType != DOM_TEXT_NODE) { // we only want text nodes
        currentNode = forward ? [iterator nextTextNode] : [iterator previousTextNode];
        currentOffset = -1; // search whole node
    }
    while (currentNode) {
        NSString *currentText = caseSensitive ? currentNode.nodeValue : [currentNode.nodeValue lowercaseString];
        int currentTextLength = currentText.length;
        if (currentOffset < 0) // search whole node
            currentOffset = forward ? 0 : currentTextLength - 1;
        while (currentOffset < currentTextLength/*forward*/ && currentOffset >= 0/*backward*/) {
            if ([currentText characterAtIndex:currentOffset] == [searchTerm characterAtIndex:indexInSearchTerm]) {
                indexInSearchTerm += forward ? 1 : -1;
                if (!selectionStart) {
                    selectionStart = currentNode;
                    selectionStartOffset = currentOffset;
                }
            } else {
                indexInSearchTerm = forward ? 0 : searchTerm.length - 1;
                selectionStart = nil;
            }
            currentOffset += forward ? 1 : -1;
            if (indexInSearchTerm >= searchTermLength/*forward*/ || indexInSearchTerm < 0/*backward*/) {
                // we have found a match!
                DOMRange *range = [document createRange];
                if (forward) {
                    [range setStart:selectionStart offset:selectionStartOffset];
                    [range setEnd:currentNode offset:currentOffset];
                } else {
                    [range setStart:currentNode offset:(currentOffset + 1)]; // was already decreased
                    [range setEnd:selectionStart offset:(selectionStartOffset + 1)];
                }
                return range;
            }
        }
        currentNode = forward ? [iterator nextTextNode] : [iterator previousTextNode];
        currentOffset = -1; // search whole node
    }
    return nil;
}

bool MacWebKitHelpViewer::findText(const QString &text, Core::FindFlags flags, bool incremental,
                                   bool fromSearch, bool *wrapped)
{
    Q_UNUSED(incremental);
    Q_UNUSED(fromSearch);
    AutoreleasePool pool; Q_UNUSED(pool)
    if (wrapped)
        *wrapped = false;
    bool forward = !(flags & Core::FindBackward);
    bool caseSensitive = (flags & Core::FindCaseSensitively);
    WebView *webView = m_widget->webView();

    // WebView searchFor:.... grabs first responder, and when losing first responder afterwards,
    // it removes the selection and forgets the search state, making it pretty useless for us

    // define the start node and offset for the search
    DOMNode *start = nil; // default is search whole body
    int startOffset = -1;
    DOMRange *selection = webView.selectedDOMRange;
    if (selection) {
        if (QString::fromNSString(selection.text).compare(
                    text, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) != 0) {
            // for incremental search we want to search from the beginning of the selection
            start = selection.startContainer;
            startOffset = selection.startOffset;
        } else {
            // search for next occurrence
            if (forward) {
                start = selection.endContainer;
                startOffset = selection.endOffset;
            } else {
                start = selection.startContainer;
                startOffset = selection.startOffset;
            }
        }
    }
    DOMRange *newSelection = findText(text.toNSString(), forward, caseSensitive,
                                      start, startOffset);
    if (!newSelection && start != nil) { // wrap
        start = nil;
        startOffset = -1;
        newSelection = findText(text.toNSString(), forward, caseSensitive,
                                              start, startOffset);
        if (newSelection && wrapped)
            *wrapped = true;
    }
    if (newSelection) {
        // found, select and scroll there
        [webView setSelectedDOMRange:newSelection affinity:NSSelectionAffinityDownstream];
        if (forward)
            [newSelection.endContainer.parentElement scrollIntoViewIfNeeded:YES];
        else
            [newSelection.startContainer.parentElement scrollIntoViewIfNeeded:YES];
        return true;
    }
    return false;
}

void MacWebKitHelpViewer::copy()
{
    QApplication::clipboard()->setText(selectedText());
}

void MacWebKitHelpViewer::stop()
{
    [m_widget->webView() stopLoading:nil];
}

void MacWebKitHelpViewer::forward()
{
    AutoreleasePool pool; Q_UNUSED(pool)
    [m_widget->webView() goForward];
    emit forwardAvailable(isForwardAvailable());
    emit backwardAvailable(isBackwardAvailable());
}

void MacWebKitHelpViewer::backward()
{
    AutoreleasePool pool; Q_UNUSED(pool)
    [m_widget->webView() goBack];
    emit forwardAvailable(isForwardAvailable());
    emit backwardAvailable(isBackwardAvailable());
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
    AutoreleasePool pool; Q_UNUSED(pool)
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    bool ok = false;
    int index = action->data().toInt(&ok);
    QTC_ASSERT(ok, return);
    WebBackForwardList *list = m_widget->webView().backForwardList;
    WebHistoryItem *item = [list itemAtIndex:index];
    QTC_ASSERT(item, return);
    [m_widget->webView() goToBackForwardItem:item];
    emit forwardAvailable(isForwardAvailable());
    emit backwardAvailable(isBackwardAvailable());
}

// #pragma mark -- MacResponderHack

MacResponderHack::MacResponderHack(QObject *parent)
    : QObject(parent)
{
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(responderHack(QWidget*,QWidget*)));
}

void MacResponderHack::responderHack(QWidget *old, QWidget *now)
{
    // On focus change, Qt does not make the corresponding QNSView firstResponder.
    // That breaks when embedding native NSView into a Qt hierarchy. When the focus is changed
    // by clicking with the mouse into a widget, everything is fine, because Cocoa automatically
    // adapts firstResponder in that case, but it breaks when setting the Qt focus from code.
    Q_UNUSED(old)
    if (!now)
        return;
    AutoreleasePool pool; Q_UNUSED(pool)
    NSView *view;
    if (QMacCocoaViewContainer *viewContainer = qobject_cast<QMacCocoaViewContainer *>(now))
        view = viewContainer->cocoaView();
    else
        view = (NSView *)now->effectiveWinId();
    [view.window makeFirstResponder:view];
}

} // Internal
} // Help
