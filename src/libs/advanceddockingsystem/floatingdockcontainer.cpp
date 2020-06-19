/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "floatingdockcontainer.h"

#include "dockareawidget.h"
#include "dockcontainerwidget.h"
#include "dockmanager.h"
#include "dockoverlay.h"
#include "dockwidget.h"
#include "linux/floatingwidgettitlebar.h"
#ifdef Q_OS_WIN
#include <windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "User32.lib")
#endif
#endif

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPointer>

static Q_LOGGING_CATEGORY(adsLog, "qtc.qmldesigner.advanceddockingsystem", QtWarningMsg)

namespace ADS
{
#ifdef Q_OS_WIN
#if 0 // set to 1 if you need this function for debugging
/**
 * Just for debugging to convert windows message identifiers to strings
 */
static const char* windowsMessageString(int messageId)
{
    switch (messageId)
    {
    case 0: return "WM_NULL";
    case 1: return "WM_CREATE";
    case 2: return "WM_DESTROY";
    case 3: return "WM_MOVE";
    case 5: return "WM_SIZE";
    case 6: return "WM_ACTIVATE";
    case 7: return "WM_SETFOCUS";
    case 8: return "WM_KILLFOCUS";
    case 10: return "WM_ENABLE";
    case 11: return "WM_SETREDRAW";
    case 12: return "WM_SETTEXT";
    case 13: return "WM_GETTEXT";
    case 14: return "WM_GETTEXTLENGTH";
    case 15: return "WM_PAINT";
    case 16: return "WM_CLOSE";
    case 17: return "WM_QUERYENDSESSION";
    case 18: return "WM_QUIT";
    case 19: return "WM_QUERYOPEN";
    case 20: return "WM_ERASEBKGND";
    case 21: return "WM_SYSCOLORCHANGE";
    case 22: return "WM_ENDSESSION";
    case 24: return "WM_SHOWWINDOW";
    case 25: return "WM_CTLCOLOR";
    case 26: return "WM_WININICHANGE";
    case 27: return "WM_DEVMODECHANGE";
    case 28: return "WM_ACTIVATEAPP";
    case 29: return "WM_FONTCHANGE";
    case 30: return "WM_TIMECHANGE";
    case 31: return "WM_CANCELMODE";
    case 32: return "WM_SETCURSOR";
    case 33: return "WM_MOUSEACTIVATE";
    case 34: return "WM_CHILDACTIVATE";
    case 35: return "WM_QUEUESYNC";
    case 36: return "WM_GETMINMAXINFO";
    case 38: return "WM_PAINTICON";
    case 39: return "WM_ICONERASEBKGND";
    case 40: return "WM_NEXTDLGCTL";
    case 42: return "WM_SPOOLERSTATUS";
    case 43: return "WM_DRAWITEM";
    case 44: return "WM_MEASUREITEM";
    case 45: return "WM_DELETEITEM";
    case 46: return "WM_VKEYTOITEM";
    case 47: return "WM_CHARTOITEM";
    case 48: return "WM_SETFONT";
    case 49: return "WM_GETFONT";
    case 50: return "WM_SETHOTKEY";
    case 51: return "WM_GETHOTKEY";
    case 55: return "WM_QUERYDRAGICON";
    case 57: return "WM_COMPAREITEM";
    case 61: return "WM_GETOBJECT";
    case 65: return "WM_COMPACTING";
    case 68: return "WM_COMMNOTIFY";
    case 70: return "WM_WINDOWPOSCHANGING";
    case 71: return "WM_WINDOWPOSCHANGED";
    case 72: return "WM_POWER";
    case 73: return "WM_COPYGLOBALDATA";
    case 74: return "WM_COPYDATA";
    case 75: return "WM_CANCELJOURNAL";
    case 78: return "WM_NOTIFY";
    case 80: return "WM_INPUTLANGCHANGEREQUEST";
    case 81: return "WM_INPUTLANGCHANGE";
    case 82: return "WM_TCARD";
    case 83: return "WM_HELP";
    case 84: return "WM_USERCHANGED";
    case 85: return "WM_NOTIFYFORMAT";
    case 123: return "WM_CONTEXTMENU";
    case 124: return "WM_STYLECHANGING";
    case 125: return "WM_STYLECHANGED";
    case 126: return "WM_DISPLAYCHANGE";
    case 127: return "WM_GETICON";
    case 128: return "WM_SETICON";
    case 129: return "WM_NCCREATE";
    case 130: return "WM_NCDESTROY";
    case 131: return "WM_NCCALCSIZE";
    case 132: return "WM_NCHITTEST";
    case 133: return "WM_NCPAINT";
    case 134: return "WM_NCACTIVATE";
    case 135: return "WM_GETDLGCODE";
    case 136: return "WM_SYNCPAINT";
    case 160: return "WM_NCMOUSEMOVE";
    case 161: return "WM_NCLBUTTONDOWN";
    case 162: return "WM_NCLBUTTONUP";
    case 163: return "WM_NCLBUTTONDBLCLK";
    case 164: return "WM_NCRBUTTONDOWN";
    case 165: return "WM_NCRBUTTONUP";
    case 166: return "WM_NCRBUTTONDBLCLK";
    case 167: return "WM_NCMBUTTONDOWN";
    case 168: return "WM_NCMBUTTONUP";
    case 169: return "WM_NCMBUTTONDBLCLK";
    case 171: return "WM_NCXBUTTONDOWN";
    case 172: return "WM_NCXBUTTONUP";
    case 173: return "WM_NCXBUTTONDBLCLK";
    case 176: return "EM_GETSEL";
    case 177: return "EM_SETSEL";
    case 178: return "EM_GETRECT";
    case 179: return "EM_SETRECT";
    case 180: return "EM_SETRECTNP";
    case 181: return "EM_SCROLL";
    case 182: return "EM_LINESCROLL";
    case 183: return "EM_SCROLLCARET";
    case 185: return "EM_GETMODIFY";
    case 187: return "EM_SETMODIFY";
    case 188: return "EM_GETLINECOUNT";
    case 189: return "EM_LINEINDEX";
    case 190: return "EM_SETHANDLE";
    case 191: return "EM_GETHANDLE";
    case 192: return "EM_GETTHUMB";
    case 193: return "EM_LINELENGTH";
    case 194: return "EM_REPLACESEL";
    case 195: return "EM_SETFONT";
    case 196: return "EM_GETLINE";
    case 197: return "EM_LIMITTEXT / EM_SETLIMITTEXT";
    case 198: return "EM_CANUNDO";
    case 199: return "EM_UNDO";
    case 200: return "EM_FMTLINES";
    case 201: return "EM_LINEFROMCHAR";
    case 202: return "EM_SETWORDBREAK";
    case 203: return "EM_SETTABSTOPS";
    case 204: return "EM_SETPASSWORDCHAR";
    case 205: return "EM_EMPTYUNDOBUFFER";
    case 206: return "EM_GETFIRSTVISIBLELINE";
    case 207: return "EM_SETREADONLY";
    case 209: return "EM_SETWORDBREAKPROC / EM_GETWORDBREAKPROC";
    case 210: return "EM_GETPASSWORDCHAR";
    case 211: return "EM_SETMARGINS";
    case 212: return "EM_GETMARGINS";
    case 213: return "EM_GETLIMITTEXT";
    case 214: return "EM_POSFROMCHAR";
    case 215: return "EM_CHARFROMPOS";
    case 216: return "EM_SETIMESTATUS";
    case 217: return "EM_GETIMESTATUS";
    case 224: return "SBM_SETPOS";
    case 225: return "SBM_GETPOS";
    case 226: return "SBM_SETRANGE";
    case 227: return "SBM_GETRANGE";
    case 228: return "SBM_ENABLE_ARROWS";
    case 230: return "SBM_SETRANGEREDRAW";
    case 233: return "SBM_SETSCROLLINFO";
    case 234: return "SBM_GETSCROLLINFO";
    case 235: return "SBM_GETSCROLLBARINFO";
    case 240: return "BM_GETCHECK";
    case 241: return "BM_SETCHECK";
    case 242: return "BM_GETSTATE";
    case 243: return "BM_SETSTATE";
    case 244: return "BM_SETSTYLE";
    case 245: return "BM_CLICK";
    case 246: return "BM_GETIMAGE";
    case 247: return "BM_SETIMAGE";
    case 248: return "BM_SETDONTCLICK";
    case 255: return "WM_INPUT";
    case 256: return "WM_KEYDOWN";
    case 257: return "WM_KEYUP";
    case 258: return "WM_CHAR";
    case 259: return "WM_DEADCHAR";
    case 260: return "WM_SYSKEYDOWN";
    case 261: return "WM_SYSKEYUP";
    case 262: return "WM_SYSCHAR";
    case 263: return "WM_SYSDEADCHAR";
    case 265: return "WM_UNICHAR / WM_WNT_CONVERTREQUESTEX";
    case 266: return "WM_CONVERTREQUEST";
    case 267: return "WM_CONVERTRESULT";
    case 268: return "WM_INTERIM";
    case 269: return "WM_IME_STARTCOMPOSITION";
    case 270: return "WM_IME_ENDCOMPOSITION";
    case 272: return "WM_INITDIALOG";
    case 273: return "WM_COMMAND";
    case 274: return "WM_SYSCOMMAND";
    case 275: return "WM_TIMER";
    case 276: return "WM_HSCROLL";
    case 277: return "WM_VSCROLL";
    case 278: return "WM_INITMENU";
    case 279: return "WM_INITMENUPOPUP";
    case 280: return "WM_SYSTIMER";
    case 287: return "WM_MENUSELECT";
    case 288: return "WM_MENUCHAR";
    case 289: return "WM_ENTERIDLE";
    case 290: return "WM_MENURBUTTONUP";
    case 291: return "WM_MENUDRAG";
    case 292: return "WM_MENUGETOBJECT";
    case 293: return "WM_UNINITMENUPOPUP";
    case 294: return "WM_MENUCOMMAND";
    case 295: return "WM_CHANGEUISTATE";
    case 296: return "WM_UPDATEUISTATE";
    case 297: return "WM_QUERYUISTATE";
    case 306: return "WM_CTLCOLORMSGBOX";
    case 307: return "WM_CTLCOLOREDIT";
    case 308: return "WM_CTLCOLORLISTBOX";
    case 309: return "WM_CTLCOLORBTN";
    case 310: return "WM_CTLCOLORDLG";
    case 311: return "WM_CTLCOLORSCROLLBAR";
    case 312: return "WM_CTLCOLORSTATIC";
    case 512: return "WM_MOUSEMOVE";
    case 513: return "WM_LBUTTONDOWN";
    case 514: return "WM_LBUTTONUP";
    case 515: return "WM_LBUTTONDBLCLK";
    case 516: return "WM_RBUTTONDOWN";
    case 517: return "WM_RBUTTONUP";
    case 518: return "WM_RBUTTONDBLCLK";
    case 519: return "WM_MBUTTONDOWN";
    case 520: return "WM_MBUTTONUP";
    case 521: return "WM_MBUTTONDBLCLK";
    case 522: return "WM_MOUSEWHEEL";
    case 523: return "WM_XBUTTONDOWN";
    case 524: return "WM_XBUTTONUP";
    case 525: return "WM_XBUTTONDBLCLK";
    case 528: return "WM_PARENTNOTIFY";
    case 529: return "WM_ENTERMENULOOP";
    case 530: return "WM_EXITMENULOOP";
    case 531: return "WM_NEXTMENU";
    case 532: return "WM_SIZING";
    case 533: return "WM_CAPTURECHANGED";
    case 534: return "WM_MOVING";
    case 536: return "WM_POWERBROADCAST";
    case 537: return "WM_DEVICECHANGE";
    case 544: return "WM_MDICREATE";
    case 545: return "WM_MDIDESTROY";
    case 546: return "WM_MDIACTIVATE";
    case 547: return "WM_MDIRESTORE";
    case 548: return "WM_MDINEXT";
    case 549: return "WM_MDIMAXIMIZE";
    case 550: return "WM_MDITILE";
    case 551: return "WM_MDICASCADE";
    case 552: return "WM_MDIICONARRANGE";
    case 553: return "WM_MDIGETACTIVE";
    case 560: return "WM_MDISETMENU";
    case 561: return "WM_ENTERSIZEMOVE";
    case 562: return "WM_EXITSIZEMOVE";
    case 563: return "WM_DROPFILES";
    case 564: return "WM_MDIREFRESHMENU";
    case 640: return "WM_IME_REPORT";
    case 641: return "WM_IME_SETCONTEXT";
    case 642: return "WM_IME_NOTIFY";
    case 643: return "WM_IME_CONTROL";
    case 644: return "WM_IME_COMPOSITIONFULL";
    case 645: return "WM_IME_SELECT";
    case 646: return "WM_IME_CHAR";
    case 648: return "WM_IME_REQUEST";
    case 656: return "WM_IME_KEYDOWN";
    case 657: return "WM_IME_KEYUP";
    case 672: return "WM_NCMOUSEHOVER";
    case 673: return "WM_MOUSEHOVER";
    case 674: return "WM_NCMOUSELEAVE";
    case 675: return "WM_MOUSELEAVE";
    case 768: return "WM_CUT";
    case 769: return "WM_COPY";
    case 770: return "WM_PASTE";
    case 771: return "WM_CLEAR";
    case 772: return "WM_UNDO";
    case 773: return "WM_RENDERFORMAT";
    case 774: return "WM_RENDERALLFORMATS";
    case 775: return "WM_DESTROYCLIPBOARD";
    case 776: return "WM_DRAWCLIPBOARD";
    case 777: return "WM_PAINTCLIPBOARD";
    case 778: return "WM_VSCROLLCLIPBOARD";
    case 779: return "WM_SIZECLIPBOARD";
    case 780: return "WM_ASKCBFORMATNAME";
    case 781: return "WM_CHANGECBCHAIN";
    case 782: return "WM_HSCROLLCLIPBOARD";
    case 783: return "WM_QUERYNEWPALETTE";
    case 784: return "WM_PALETTEISCHANGING";
    case 785: return "WM_PALETTECHANGED";
    case 786: return "WM_HOTKEY";
    case 791: return "WM_PRINT";
    case 792: return "WM_PRINTCLIENT";
    case 793: return "WM_APPCOMMAND";
    case 856: return "WM_HANDHELDFIRST";
    case 863: return "WM_HANDHELDLAST";
    case 864: return "WM_AFXFIRST";
    case 895: return "WM_AFXLAST";
    case 896: return "WM_PENWINFIRST";
    case 897: return "WM_RCRESULT";
    case 898: return "WM_HOOKRCRESULT";
    case 899: return "WM_GLOBALRCCHANGE / WM_PENMISCINFO";
    case 900: return "WM_SKB";
    case 901: return "WM_HEDITCTL / WM_PENCTL";
    case 902: return "WM_PENMISC";
    case 903: return "WM_CTLINIT";
    case 904: return "WM_PENEVENT";
    case 911: return "WM_PENWINLAST";
    default:
        return "unknown WM_ message";
    }

    return "unknown WM_ message";
}
#endif
#endif

    AbstractFloatingWidget::~AbstractFloatingWidget() = default;

    static unsigned int zOrderCounter = 0;
    /**
     * Private data class of FloatingDockContainer class (pimpl)
     */
    class FloatingDockContainerPrivate
    {
    public:
        FloatingDockContainer *q;
        DockContainerWidget *m_dockContainer = nullptr;
        unsigned int m_zOrderIndex = ++zOrderCounter;
        QPointer<DockManager> m_dockManager;
        eDragState m_draggingState = DraggingInactive;
        QPoint m_dragStartMousePosition;
        DockContainerWidget *m_dropContainer = nullptr;
        DockAreaWidget *m_singleDockArea = nullptr;
        QPoint m_dragStartPos;
        bool m_hiding = false;
        QWidget *m_mouseEventHandler = nullptr; // linux only
        FloatingWidgetTitleBar *m_titleBar = nullptr; // linux only

        /**
         * Private data constructor
         */
        FloatingDockContainerPrivate(FloatingDockContainer *parent);

        void titleMouseReleaseEvent();
        void updateDropOverlays(const QPoint &globalPosition);

        /**
         * Returns true if the given config flag is set
         */
        static bool testConfigFlag(DockManager::eConfigFlag flag)
        {
            return DockManager::testConfigFlag(flag);
        }

        /**
         * Tests is a certain state is active
         */
        bool isState(eDragState stateId) const { return stateId == m_draggingState; }

        void setState(eDragState stateId) { m_draggingState = stateId; }

        void setWindowTitle(const QString &text)
        {
            if (Utils::HostOsInfo::isLinuxHost())
                m_titleBar->setTitle(text);
            else
                q->setWindowTitle(text);
        }

        /**
         * Reflect the current dock widget title in the floating widget windowTitle()
         * depending on the DockManager::FloatingContainerHasWidgetTitle flag
         */
        void reflectCurrentWidget(DockWidget *currentWidget)
        {
            // reflect CurrentWidget's title if configured to do so, otherwise display application name as window title
            if (testConfigFlag(DockManager::FloatingContainerHasWidgetTitle))
                setWindowTitle(currentWidget->windowTitle());
            else
                setWindowTitle(QApplication::applicationDisplayName());

            // reflect currentWidget's icon if configured to do so, otherwise display application icon as window icon
            QIcon currentWidgetIcon = currentWidget->icon();
            if (testConfigFlag(DockManager::FloatingContainerHasWidgetIcon) && !currentWidgetIcon.isNull())
                q->setWindowIcon(currentWidget->icon());
            else
                q->setWindowIcon(QApplication::windowIcon());
        }

        /**
         * Handles escape key press when dragging around the floating widget
         */
        void handleEscapeKey();
    }; // class FloatingDockContainerPrivate

    FloatingDockContainerPrivate::FloatingDockContainerPrivate(FloatingDockContainer *parent)
        : q(parent)
    {}

    void FloatingDockContainerPrivate::titleMouseReleaseEvent()
    {
        setState(DraggingInactive);
        if (!m_dropContainer)
            return;

        if (m_dockManager->dockAreaOverlay()->dropAreaUnderCursor() != InvalidDockWidgetArea
            || m_dockManager->containerOverlay()->dropAreaUnderCursor() != InvalidDockWidgetArea) {
            DockOverlay *overlay = m_dockManager->containerOverlay();
            if (!overlay->dropOverlayRect().isValid())
                overlay = m_dockManager->dockAreaOverlay();

            // Resize the floating widget to the size of the highlighted drop area rectangle
            QRect rect = overlay->dropOverlayRect();
            int frameWidth = (q->frameSize().width() - q->rect().width()) / 2;
            int titleBarHeight = q->frameSize().height() - q->rect().height() - frameWidth;
            if (rect.isValid()) {
                QPoint topLeft = overlay->mapToGlobal(rect.topLeft());
                topLeft.ry() += titleBarHeight;
                q->setGeometry(QRect(topLeft, QSize(rect.width(), rect.height() - titleBarHeight)));
                QApplication::processEvents();
            }
            m_dropContainer->dropFloatingWidget(q, QCursor::pos());
        }

        m_dockManager->containerOverlay()->hideOverlay();
        m_dockManager->dockAreaOverlay()->hideOverlay();
    }

    void FloatingDockContainerPrivate::updateDropOverlays(const QPoint &globalPosition)
    {
        if (!q->isVisible() || !m_dockManager)
            return;

        auto containers = m_dockManager->dockContainers();
        DockContainerWidget *topContainer = nullptr;
        for (auto containerWidget : containers) {
            if (!containerWidget->isVisible())
                continue;

            if (m_dockContainer == containerWidget)
                continue;

            QPoint mappedPos = containerWidget->mapFromGlobal(globalPosition);
            if (containerWidget->rect().contains(mappedPos)) {
                if (!topContainer || containerWidget->isInFrontOf(topContainer))
                    topContainer = containerWidget;
            }
        }

        m_dropContainer = topContainer;
        auto containerOverlay = m_dockManager->containerOverlay();
        auto dockAreaOverlay = m_dockManager->dockAreaOverlay();

        if (!topContainer) {
            containerOverlay->hideOverlay();
            dockAreaOverlay->hideOverlay();
            return;
        }

        int visibleDockAreas = topContainer->visibleDockAreaCount();
        containerOverlay->setAllowedAreas(visibleDockAreas > 1 ? OuterDockAreas : AllDockAreas);
        DockWidgetArea containerArea = containerOverlay->showOverlay(topContainer);
        containerOverlay->enableDropPreview(containerArea != InvalidDockWidgetArea);
        auto dockArea = topContainer->dockAreaAt(globalPosition);
        if (dockArea && dockArea->isVisible() && visibleDockAreas > 0) {
            dockAreaOverlay->enableDropPreview(true);
            dockAreaOverlay->setAllowedAreas((visibleDockAreas == 1) ? NoDockWidgetArea
                                                                     : dockArea->allowedAreas());
            DockWidgetArea area = dockAreaOverlay->showOverlay(dockArea);

            // A CenterDockWidgetArea for the dockAreaOverlay() indicates that the mouse is in
            // the title bar. If the ContainerArea is valid then we ignore the dock area of the
            // dockAreaOverlay() and disable the drop preview
            if ((area == CenterDockWidgetArea) && (containerArea != InvalidDockWidgetArea)) {
                dockAreaOverlay->enableDropPreview(false);
                containerOverlay->enableDropPreview(true);
            } else {
                containerOverlay->enableDropPreview(InvalidDockWidgetArea == area);
            }
        } else {
            dockAreaOverlay->hideOverlay();
        }
    }

    void FloatingDockContainerPrivate::handleEscapeKey()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        setState(DraggingInactive);
        m_dockManager->containerOverlay()->hideOverlay();
        m_dockManager->dockAreaOverlay()->hideOverlay();
    }

    FloatingDockContainer::FloatingDockContainer(DockManager *dockManager)
        : FloatingWidgetBaseType(dockManager)
        , d(new FloatingDockContainerPrivate(this))
    {
        d->m_dockManager = dockManager;
        d->m_dockContainer = new DockContainerWidget(dockManager, this);
        connect(d->m_dockContainer,
                &DockContainerWidget::dockAreasAdded,
                this,
                &FloatingDockContainer::onDockAreasAddedOrRemoved);
        connect(d->m_dockContainer,
                &DockContainerWidget::dockAreasRemoved,
                this,
                &FloatingDockContainer::onDockAreasAddedOrRemoved);

    #ifdef Q_OS_LINUX
        d->m_titleBar = new FloatingWidgetTitleBar(this);
        setWindowFlags(windowFlags() | Qt::Tool);
        QDockWidget::setWidget(d->m_dockContainer);
        QDockWidget::setFloating(true);
        QDockWidget::setFeatures(DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
        setTitleBarWidget(d->m_titleBar);
        connect(d->m_titleBar,
                &FloatingWidgetTitleBar::closeRequested,
                this,
                &FloatingDockContainer::close);
    #else
        setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
        QBoxLayout *boxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);
        setLayout(boxLayout);
        boxLayout->addWidget(d->m_dockContainer);
    #endif
        dockManager->registerFloatingWidget(this);
    }

    FloatingDockContainer::FloatingDockContainer(DockAreaWidget *dockArea)
        : FloatingDockContainer(dockArea->dockManager())
    {
        d->m_dockContainer->addDockArea(dockArea);
    #ifdef Q_OS_LINUX
        d->m_titleBar->enableCloseButton(isClosable());
    #endif
        if (auto dw = topLevelDockWidget())
            dw->emitTopLevelChanged(true);

        d->m_dockManager->notifyWidgetOrAreaRelocation(dockArea);
    }

    FloatingDockContainer::FloatingDockContainer(DockWidget *dockWidget)
        : FloatingDockContainer(dockWidget->dockManager())
    {
        d->m_dockContainer->addDockWidget(CenterDockWidgetArea, dockWidget);
    #ifdef Q_OS_LINUX
        d->m_titleBar->enableCloseButton(isClosable());
    #endif
        if (auto dw = topLevelDockWidget())
            dw->emitTopLevelChanged(true);

        d->m_dockManager->notifyWidgetOrAreaRelocation(dockWidget);
    }

    FloatingDockContainer::~FloatingDockContainer()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        if (d->m_dockManager)
            d->m_dockManager->removeFloatingWidget(this);

        delete d;
    }

    DockContainerWidget *FloatingDockContainer::dockContainer() const { return d->m_dockContainer; }

    void FloatingDockContainer::changeEvent(QEvent *event)
    {
        QWidget::changeEvent(event);
        if ((event->type() == QEvent::ActivationChange) && isActiveWindow()) {
            qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::ActivationChange";
            d->m_zOrderIndex = ++zOrderCounter;
            return;
        }
    }

#ifdef Q_OS_WIN
bool FloatingDockContainer::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    QWidget::nativeEvent(eventType, message, result);
    MSG *msg = static_cast<MSG *>(message);
    switch (msg->message)
    {
        case WM_MOVING:
        {
            if (d->isState(DraggingFloatingWidget))
                d->updateDropOverlays(QCursor::pos());
        }
        break;

        case WM_NCLBUTTONDOWN:
             if (msg->wParam == HTCAPTION && d->isState(DraggingInactive))
             {
                qCInfo(adsLog) << Q_FUNC_INFO << "WM_NCLBUTTONDOWN" << eventType;
                d->m_dragStartPos = pos();
                d->setState(DraggingMousePressed);
             }
             break;

        case WM_NCLBUTTONDBLCLK:
             d->setState(DraggingInactive);
             break;

        case WM_ENTERSIZEMOVE:
             if (d->isState(DraggingMousePressed))
             {
                qCInfo(adsLog) << Q_FUNC_INFO << "WM_ENTERSIZEMOVE" << eventType;
                d->setState(DraggingFloatingWidget);
                d->updateDropOverlays(QCursor::pos());
             }
             break;

        case WM_EXITSIZEMOVE:
             if (d->isState(DraggingFloatingWidget))
             {
                qCInfo(adsLog) << Q_FUNC_INFO << "WM_EXITSIZEMOVE" << eventType;
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
                    d->handleEscapeKey();
                else
                    d->titleMouseReleaseEvent();
             }
             break;
    }
    return false;
}
#endif

    void FloatingDockContainer::closeEvent(QCloseEvent *event)
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        d->setState(DraggingInactive);
        event->ignore();

        if (isClosable()) {
            auto dw = topLevelDockWidget();
            if (dw && dw->features().testFlag(DockWidget::DockWidgetDeleteOnClose)) {
                if (!dw->closeDockWidgetInternal())
                    return;
            }

            this->hide();
        }
    }

    void FloatingDockContainer::hideEvent(QHideEvent *event)
    {
        Super::hideEvent(event);
        if (event->spontaneous())
            return;

        // Prevent toogleView() events during restore state
        if (d->m_dockManager->isRestoringState())
            return;

        d->m_hiding = true;
        for (auto dockArea : d->m_dockContainer->openedDockAreas()) {
            for (auto dockWidget : dockArea->openedDockWidgets())
                dockWidget->toggleView(false);
        }
        d->m_hiding = false;
    }

    void FloatingDockContainer::showEvent(QShowEvent *event)
    {
         Super::showEvent(event);
     #ifdef Q_OS_LINUX
         if (DockManager::testConfigFlag(DockManager::FocusHighlighting))
             window()->activateWindow();
     #endif
    }

    void FloatingDockContainer::startFloating(const QPoint &dragStartMousePos,
                                              const QSize &size,
                                              eDragState dragState,
                                              QWidget *mouseEventHandler)
    {
    #ifndef Q_OS_LINUX
        Q_UNUSED(mouseEventHandler)
    #endif
        resize(size);
        d->setState(dragState);
        d->m_dragStartMousePosition = dragStartMousePos;

    #ifdef Q_OS_LINUX
        if (DraggingFloatingWidget == dragState) {
            setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
            d->m_mouseEventHandler = mouseEventHandler;
            if (d->m_mouseEventHandler)
                d->m_mouseEventHandler->grabMouse();
        }
    #endif
        moveFloating();
        show();
    }

    void FloatingDockContainer::moveFloating()
    {
        const int borderSize = (frameSize().width() - size().width()) / 2;
        const QPoint moveToPos = QCursor::pos() - d->m_dragStartMousePosition
                                 - QPoint(borderSize, 0);
        move(moveToPos);

        switch (d->m_draggingState)
        {
        case DraggingMousePressed:
            d->setState(DraggingFloatingWidget);
            d->updateDropOverlays(QCursor::pos());
            break;

        case DraggingFloatingWidget:
            d->updateDropOverlays(QCursor::pos());
            // On macOS when hiding the DockAreaOverlay the application would set
            // the main window as the active window for some reason. This fixes
            // that by resetting the active window to the floating widget after
            // updating the overlays.
            if (Utils::HostOsInfo::isMacHost())
                QApplication::setActiveWindow(this);

            break;
        default:
            break;
        }
    }

    bool FloatingDockContainer::isClosable() const
    {
        return d->m_dockContainer->features().testFlag(DockWidget::DockWidgetClosable);
    }

    void FloatingDockContainer::onDockAreasAddedOrRemoved()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;
        auto topLevelDockArea = d->m_dockContainer->topLevelDockArea();
        if (topLevelDockArea) {
            d->m_singleDockArea = topLevelDockArea;
            DockWidget *currentWidget = d->m_singleDockArea->currentDockWidget();
            d->reflectCurrentWidget(currentWidget);
            connect(d->m_singleDockArea,
                    &DockAreaWidget::currentChanged,
                    this,
                    &FloatingDockContainer::onDockAreaCurrentChanged);
        } else {
            if (d->m_singleDockArea) {
                disconnect(d->m_singleDockArea,
                           &DockAreaWidget::currentChanged,
                           this,
                           &FloatingDockContainer::onDockAreaCurrentChanged);
                d->m_singleDockArea = nullptr;
            }
            d->setWindowTitle(QApplication::applicationDisplayName());
            setWindowIcon(QApplication::windowIcon());
        }
    }

    void FloatingDockContainer::updateWindowTitle()
    {
        // If this floating container will be hidden, then updating the window
        // title is not required anymore
        if (d->m_hiding)
            return;

        if (auto topLevelDockArea = d->m_dockContainer->topLevelDockArea()) {
            if (DockWidget *currentWidget = topLevelDockArea->currentDockWidget())
                d->reflectCurrentWidget(currentWidget);
        } else {
            d->setWindowTitle(QApplication::applicationDisplayName());
            setWindowIcon(QApplication::windowIcon());
        }
    }

    void FloatingDockContainer::onDockAreaCurrentChanged(int index)
    {
        Q_UNUSED(index)
        DockWidget *currentWidget = d->m_singleDockArea->currentDockWidget();
        d->reflectCurrentWidget(currentWidget);
    }

    bool FloatingDockContainer::restoreState(DockingStateReader &stream, bool testing)
    {
        if (!d->m_dockContainer->restoreState(stream, testing))
            return false;

        onDockAreasAddedOrRemoved();
        return true;
    }

    bool FloatingDockContainer::hasTopLevelDockWidget() const
    {
        return d->m_dockContainer->hasTopLevelDockWidget();
    }

    DockWidget *FloatingDockContainer::topLevelDockWidget() const
    {
        return d->m_dockContainer->topLevelDockWidget();
    }

    QList<DockWidget *> FloatingDockContainer::dockWidgets() const
    {
        return d->m_dockContainer->dockWidgets();
    }

    void FloatingDockContainer::finishDragging()
    {
        qCInfo(adsLog) << Q_FUNC_INFO;

    #ifdef Q_OS_LINUX
        setAttribute(Qt::WA_X11NetWmWindowTypeDock, false);
        setWindowOpacity(1);
        activateWindow();
        if (d->m_mouseEventHandler) {
            d->m_mouseEventHandler->releaseMouse();
            d->m_mouseEventHandler = nullptr;
        }
    #endif
        d->titleMouseReleaseEvent();
    }

#ifdef Q_OS_MACOS
bool FloatingDockContainer::event(QEvent *event)
{
    switch (d->m_draggingState)
    {
    case DraggingInactive:
    {
        // Normally we would check here, if the left mouse button is pressed.
        // But from QT version 5.12.2 on the mouse events from
        // QEvent::NonClientAreaMouseButtonPress return the wrong mouse button
        // The event always returns Qt::RightButton even if the left button
        // is clicked.
        // It is really great to work around the whole NonClientMouseArea
        // bugs
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 2))
        if (event->type() == QEvent::NonClientAreaMouseButtonPress
                /*&& QGuiApplication::mouseButtons().testFlag(Qt::LeftButton)*/)
#else
        if (event->type() == QEvent::NonClientAreaMouseButtonPress
                && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
#endif
        {
            qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonPress"
                                          << event->type();
            d->m_dragStartPos = pos();
            d->setState(DraggingMousePressed);
        }
    }
    break;

    case DraggingMousePressed:
        switch (event->type())
        {
        case QEvent::NonClientAreaMouseButtonDblClick:
            qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonDblClick";
            d->setState(DraggingInactive);
            break;

        case QEvent::Resize:
            // If the first event after the mouse press is a resize event, then
            // the user resizes the window instead of dragging it around.
            // But there is one exception. If the window is maximized,
            // then dragging the window via title bar will cause the widget to
            // leave the maximized state. This in turn will trigger a resize event.
            // To know, if the resize event was triggered by user via moving a
            // corner of the window frame or if it was caused by a windows state
            // change, we check, if we are not in maximized state.
            if (!isMaximized())
                d->setState(DraggingInactive);
            break;

        default:
            break;
        }
        break;

    case DraggingFloatingWidget:
        if (event->type() == QEvent::NonClientAreaMouseButtonRelease)
        {
            qCInfo(adsLog) << Q_FUNC_INFO << "QEvent::NonClientAreaMouseButtonRelease";
            d->titleMouseReleaseEvent();
        }
        break;

    default:
        break;
    }

#if (ADS_DEBUG_LEVEL > 0)
    qDebug() << Q_FUNC_INFO << event->type();
#endif
    return QWidget::event(event);
}

void FloatingDockContainer::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    switch (d->m_draggingState)
    {
    case DraggingMousePressed:
        d->setState(DraggingFloatingWidget);
        d->updateDropOverlays(QCursor::pos());
        break;

    case DraggingFloatingWidget:
        d->updateDropOverlays(QCursor::pos());
        // On macOS when hiding the DockAreaOverlay the application would set
        // the main window as the active window for some reason. This fixes
        // that by resetting the active window to the floating widget after
        // updating the overlays.
        QApplication::setActiveWindow(this);
        break;
    default:
        break;
    }
}
#endif
} // namespace ADS
