/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mainwidget.h"
#include "actionhandler.h"
#include "colorthemes.h"
#include "colortoolbutton.h"
#include "errorwidget.h"
#include "graphicsscene.h"
#include "magnifier.h"
#include "navigator.h"
#include "outputtabwidget.h"
#include "scxmltagutils.h"
#include "scxmleditorconstants.h"
#include "search.h"
#include "shapestoolbox.h"
#include "stateitem.h"
#include "stateproperties.h"
#include "stateview.h"
#include "statisticsdialog.h"
#include "structure.h"
#include "undocommands.h"
#include "warning.h"
#include "warningprovider.h"

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QMimeData>

#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QItemEditorFactory>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QXmlStreamWriter>

#include "scxmluifactory.h"

#include <QCoreApplication>
#include <app/app_version.h>
#include <iostream>

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <utils/algorithm.h>
#include <utils/icon.h>

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;
using namespace ScxmlEditor::OutputPane;

void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString strOutput;
    QString prefix;
    switch (type) {
    case QtDebugMsg:
        prefix = "D";
        break;
    case QtWarningMsg:
        prefix = "W";
        break;
    case QtCriticalMsg:
        prefix = "C";
        break;
    case QtFatalMsg:
        prefix = "F";
        break;
    default:
        break;
    }

    strOutput = QString::fromLatin1("[%1] [%2]: (%3:%4): %5").arg(QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss")).arg(prefix).arg(QLatin1String(context.file)).arg(context.line).arg(msg);
    std::cerr << strOutput.toStdString() << std::endl;

    QFile file(QString::fromLatin1("%1/sceditor_log.txt").arg(QCoreApplication::applicationDirPath()));
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        std::cerr << "cannot write file" << std::endl;
        return;
    }

    QTextStream out(&file);
    out << strOutput << "\n";
    file.close();

    if (type == QtFatalMsg)
        abort();
}

static QIcon toolButtonIcon(ActionType actionType)
{
    QString iconFileName;

    switch (actionType) {
    case ActionAlignLeft:
        iconFileName = ":/scxmleditor/images/align_left.png";
        break;
    case ActionAlignRight:
        iconFileName = ":/scxmleditor/images/align_right.png";
        break;
    case ActionAlignTop:
        iconFileName = ":/scxmleditor/images/align_top.png";
        break;
    case ActionAlignBottom:
        iconFileName = ":/scxmleditor/images/align_bottom.png";
        break;
    case ActionAlignHorizontal:
        iconFileName = ":/scxmleditor/images/align_horizontal.png";
        break;
    case ActionAlignVertical:
        iconFileName = ":/scxmleditor/images/align_vertical.png";
        break;

    case ActionAdjustWidth:
        iconFileName = ":/scxmleditor/images/adjust_width.png";
        break;
    case ActionAdjustHeight:
        iconFileName = ":/scxmleditor/images/adjust_height.png";
        break;
    case ActionAdjustSize:
        iconFileName = ":/scxmleditor/images/adjust_size.png";
        break;

    default:
        return QIcon();
    }

    return Utils::Icon({{iconFileName, Utils::Theme::IconsBaseColor}}).icon();
}

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
    addStateView();
    m_uiFactory->documentChanged(NewDocument, m_document);
    documentChanged();
}

MainWidget::~MainWidget()
{
    clear();
    delete m_document;
}

QAction *MainWidget::action(ActionType type)
{
    if (type == ActionColorTheme)
        return m_colorThemes->modifyThemeAction();

    if (type >= ActionZoomIn && type < ActionLast)
        return m_actionHandler->action(type);

    return nullptr;
}

QToolButton *MainWidget::toolButton(ToolButtonType type)
{
    if (type == ToolButtonColorTheme)
        return m_colorThemes->themeToolButton();

    if (type >= ToolButtonStateColor && type < ToolButtonLast)
        return m_toolButtons[type];

    return nullptr;
}

void MainWidget::init()
{
    createUi();

    m_uiFactory = new ScxmlUiFactory(this);
    m_stateProperties->setUIFactory(m_uiFactory);
    m_colorThemes = new ColorThemes(this);
    m_shapesFrame->setUIFactory(m_uiFactory);

    m_navigator = new Navigator(this);
    m_navigator->setVisible(false);

    m_magnifier = new Magnifier(this);
    m_magnifier->setVisible(false);

    // Create, init and connect Error-pane
    m_errorPane = new ErrorWidget;
    m_outputPaneWindow->addPane(m_errorPane);

    connect(m_outputPaneWindow, &OutputTabWidget::visibilityChanged, this, &MainWidget::handleTabVisibilityChanged);

    // Init warningProvider
    auto provider = static_cast<WarningProvider*>(m_uiFactory->object(Constants::C_OBJECTNAME_WARNINGPROVIDER));
    if (provider)
        provider->init(m_errorPane->warningModel());

    connect(m_errorPane, &ErrorWidget::mouseExited, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->scene()->unhighlightAll();
    });

    connect(m_errorPane, &ErrorWidget::warningEntered, [this](Warning *w) {
        StateView *view = m_views.last();
        if (view)
            view->scene()->highlightWarningItem(w);
    });

    connect(m_errorPane, &ErrorWidget::warningSelected, [this](Warning *w) {
        StateView *view = m_views.last();
        if (view)
            view->scene()->selectWarningItem(w);
    });

    connect(m_errorPane, &ErrorWidget::warningDoubleClicked, [this](Warning *w) {
        StateView *view = m_views.last();
        if (view)
            view->view()->zoomToItem(view->scene()->findItem(view->scene()->tagByWarning(w)));
    });

    // Create and init Error-pane
    m_searchPane = new Search;
    m_outputPaneWindow->addPane(m_searchPane);

    m_document = new ScxmlDocument;
    connect(m_document, &ScxmlDocument::endTagChange, this, &MainWidget::endTagChange);
    connect(m_document, &ScxmlDocument::documentChanged, this, &MainWidget::dirtyChanged);

    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, &MainWidget::initView);

    m_actionHandler = new ActionHandler(this);

    // Connect actions
    connect(m_actionHandler->action(ActionZoomIn), &QAction::triggered, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->view()->zoomIn();
    });
    connect(m_actionHandler->action(ActionZoomOut), &QAction::triggered, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->view()->zoomOut();
    });
    connect(m_actionHandler->action(ActionFitToView), &QAction::triggered, this, &MainWidget::fitToView);
    connect(m_actionHandler->action(ActionPan), &QAction::toggled, this, [this](bool toggled) {
        StateView *view = m_views.last();
        if (view)
            view->view()->setPanning(toggled);
    });
    connect(m_actionHandler->action(ActionMagnifier), &QAction::toggled, this, &MainWidget::setMagnifier);

    connect(m_navigator, &Navigator::hideFrame, this, [this]() { m_actionHandler->action(ActionNavigator)->setChecked(false); });
    connect(m_actionHandler->action(ActionNavigator), &QAction::toggled, m_navigator, &Navigator::setVisible);

    connect(m_actionHandler->action(ActionCopy), &QAction::triggered, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->scene()->copy();
    });

    connect(m_actionHandler->action(ActionCut), &QAction::triggered, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->scene()->cut();
    });

    connect(m_actionHandler->action(ActionPaste), &QAction::triggered, this, [this]() {
        StateView *view = m_views.last();
        if (view)
            view->scene()->paste(view->view()->mapToScene(QPoint(30, 30)));
    });

    connect(m_actionHandler->action(ActionExportToImage), &QAction::triggered, this, &MainWidget::exportToImage);
    connect(m_actionHandler->action(ActionScreenshot), &QAction::triggered, this, &MainWidget::saveScreenShot);

    connect(m_errorPane->warningModel(), &WarningModel::warningsChanged, this, [this]() {
        m_actionHandler->action(ActionFullNamespace)->setEnabled(m_errorPane->warningModel()->count(Warning::ErrorType) <= 0);
    });

    connect(m_actionHandler->action(ActionFullNamespace), &QAction::triggered, this, [this](bool checked) {
        m_document->setUseFullNameSpace(checked);
    });

    connect(m_actionHandler->action(ActionAlignLeft), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignLeft); });
    connect(m_actionHandler->action(ActionAlignRight), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignRight); });
    connect(m_actionHandler->action(ActionAlignTop), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignTop); });
    connect(m_actionHandler->action(ActionAlignBottom), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignBottom); });
    connect(m_actionHandler->action(ActionAlignHorizontal), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignHorizontal); });
    connect(m_actionHandler->action(ActionAlignVertical), &QAction::triggered, this, [this]() { alignButtonClicked(ActionAlignVertical); });
    connect(m_actionHandler->action(ActionAdjustWidth), &QAction::triggered, this, [this]() { adjustButtonClicked(ActionAdjustWidth); });
    connect(m_actionHandler->action(ActionAdjustHeight), &QAction::triggered, this, [this]() { adjustButtonClicked(ActionAdjustHeight); });
    connect(m_actionHandler->action(ActionAdjustSize), &QAction::triggered, this, [this]() { adjustButtonClicked(ActionAdjustSize); });

    connect(m_actionHandler->action(ActionStatistics), &QAction::triggered, this, [this]() {
        StatisticsDialog dialog;
        dialog.setDocument(m_document);
        dialog.exec();
    });

    // Init ToolButtons
    auto stateColorButton = new ColorToolButton("StateColor", ":/scxmleditor/images/state_color.png", tr("State Color"));
    auto fontColorButton = new ColorToolButton("FontColor", ":/scxmleditor/images/font_color.png", tr("Font Color"));
    QToolButton *alignToolButton = createToolButton(toolButtonIcon(ActionAlignLeft), tr("Align Left"), QToolButton::MenuButtonPopup);
    QToolButton *adjustToolButton = createToolButton(toolButtonIcon(ActionAdjustWidth), tr("Adjust Width"), QToolButton::MenuButtonPopup);

    // Connect state color change
    connect(stateColorButton, &ColorToolButton::colorSelected, [this](const QString &color) {
        StateView *view = m_views.last();
        if (view)
            view->scene()->setEditorInfo(Constants::C_SCXML_EDITORINFO_STATECOLOR, color);
    });

    // Connect font color change
    connect(fontColorButton, &ColorToolButton::colorSelected, [this](const QString &color) {
        StateView *view = m_views.last();
        if (view)
            view->scene()->setEditorInfo(Constants::C_SCXML_EDITORINFO_FONTCOLOR, color);
    });

    // Connect alignment change
    alignToolButton->setProperty("currentAlignment", ActionAlignLeft);
    connect(alignToolButton, &QToolButton::clicked, this, [=] {
        StateView *view = m_views.last();
        if (view)
            view->scene()->alignStates(alignToolButton->property("currentAlignment").toInt());
    });

    // Connect alignment change
    adjustToolButton->setProperty("currentAdjustment", ActionAdjustWidth);
    connect(adjustToolButton, &QToolButton::clicked, this, [=] {
        StateView *view = m_views.last();
        if (view)
            view->scene()->adjustStates(adjustToolButton->property("currentAdjustment").toInt());
    });

    auto alignmentMenu = new QMenu(tr("Alignment"), this);
    for (int i = ActionAlignLeft; i <= ActionAlignVertical; ++i)
        alignmentMenu->addAction(m_actionHandler->action(ActionType(i)));
    alignToolButton->setMenu(alignmentMenu);

    auto adjustmentMenu = new QMenu(tr("Adjustment"), this);
    for (int i = ActionAdjustWidth; i <= ActionAdjustSize; ++i)
        adjustmentMenu->addAction(m_actionHandler->action(ActionType(i)));
    adjustToolButton->setMenu(adjustmentMenu);

    m_toolButtons << stateColorButton << fontColorButton << alignToolButton << adjustToolButton;

    const QSettings *s = Core::ICore::settings();
    m_horizontalSplitter->restoreState(s->value(Constants::C_SETTINGS_SPLITTER).toByteArray());

    m_actionHandler->action(ActionPaste)->setEnabled(false);

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &MainWidget::saveSettings);
}

void MainWidget::endTagChange(ScxmlDocument::TagChange change, const ScxmlTag *tag, const QVariant &value)
{
    Q_UNUSED(tag)

    switch (change) {
    case ScxmlDocument::TagChangeFullNameSpace:
        m_actionHandler->action(ActionFullNamespace)->setChecked(value.toBool());
        break;
    default:
        break;
    }
}

QString saveImageFileFilter()
{
    const auto imageFormats = QImageWriter::supportedImageFormats();
    const QByteArrayList supportedFormats = Utils::transform(imageFormats, [](const QByteArray &in)
        { return QByteArray("*.") + in; });
    return MainWidget::tr("Images (%1)").arg(QString::fromUtf8(supportedFormats.join(' ')));
}

void MainWidget::exportToImage()
{
    StateView *view = m_views.last();
    if (!view)
        return;

    QString suggestedFileName = QFileInfo(m_document->fileName()).baseName();
    if (suggestedFileName.isEmpty())
        suggestedFileName = tr("Untitled");

    QSettings *s = Core::ICore::settings();
    const QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString lastFolder = s->value(
                Constants::C_SETTINGS_LASTEXPORTFOLDER, documentsLocation).toString();
    suggestedFileName = QString::fromLatin1("%1/%2_%3.png")
            .arg(lastFolder)
            .arg(suggestedFileName)
            .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    const QString selectedFileName = QFileDialog::getSaveFileName(this,
                                                                  tr("Export Canvas to Image"),
                                                                  suggestedFileName,
                                                                  saveImageFileFilter());
    if (!selectedFileName.isEmpty()) {
        const QRectF r = view->scene()->itemsBoundingRect();
        QImage image(r.size().toSize(), QImage::Format_ARGB32);
        image.fill(QColor(0xef, 0xef, 0xef));

        QPainter painter(&image);
        view->scene()->render(&painter, QRectF(), r);

        if (image.save(selectedFileName)) {
            s->setValue(Constants::C_SETTINGS_LASTEXPORTFOLDER,
                        QFileInfo(selectedFileName).absolutePath());
        } else {
            QMessageBox::warning(this, tr("Export Failed"), tr("Could not export to image."));
        }
    }
}

void MainWidget::saveScreenShot()
{
    StateView *view = m_views.last();
    if (!view)
        return;

    QSettings *s = Core::ICore::settings();
    const QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString lastFolder =
            s->value(Constants::C_SETTINGS_LASTSAVESCREENSHOTFOLDER, documentsLocation).toString();
    const QString filename = QFileDialog::getSaveFileName(this,
                                                          tr("Save Screenshot"),
                                                          lastFolder + "/scxml_screenshot.png",
                                                          saveImageFileFilter());
    if (!filename.isEmpty()) {
        const QImage image = view->view()->grabView();

        if (image.save(filename)) {
            s->setValue(Constants::C_SETTINGS_LASTSAVESCREENSHOTFOLDER,
                        QFileInfo(filename).absolutePath());
        } else {
            QMessageBox::warning(this, tr("Saving Failed"), tr("Could not save the screenshot."));
        }
    }
}

void MainWidget::saveSettings()
{
    QSettings *s = Core::ICore::settings();
    s->setValue(Constants::C_SETTINGS_SPLITTER, m_horizontalSplitter->saveState());
}

void MainWidget::addStateView(BaseItem *item)
{
    auto view = new StateView(qgraphicsitem_cast<StateItem*>(item));

    view->scene()->setActionHandler(m_actionHandler);
    view->scene()->setWarningModel(m_errorPane->warningModel());
    view->setUiFactory(m_uiFactory);

    connect(view, &QObject::destroyed, this, [=] {
        // TODO: un-lambdafy
        m_views.removeAll(view);
        m_document->popRootTag();
        m_searchPane->setDocument(m_document);
        m_structure->setDocument(m_document);
        m_stateProperties->setDocument(m_document);
        m_colorThemes->setDocument(m_document);
        StateItem *it = view->parentState();
        if (it) {
            it->updateEditorInfo(true);
            it->shrink();

            // Update transitions
            auto scene = static_cast<GraphicsScene*>(it->scene());
            if (scene) {
                QVector<ScxmlTag*> childTransitionTags;
                TagUtils::findAllTransitionChildren(it->tag(), childTransitionTags);
                for (int i = 0; i < childTransitionTags.count(); ++i) {
                    BaseItem *item = scene->findItem(childTransitionTags[i]);
                    if (item)
                        item->updateEditorInfo();
                }
            }
        }
    });
    connect(view->view(), &GraphicsView::panningChanged, m_actionHandler->action(ActionPan), &QAction::setChecked);
    connect(view->view(), &GraphicsView::magnifierChanged, m_actionHandler->action(ActionMagnifier), &QAction::setChecked);
    connect(view->scene(), &GraphicsScene::openStateView, this, &MainWidget::addStateView, Qt::QueuedConnection);
    connect(view->scene(), &GraphicsScene::selectedStateCountChanged, this, [this](int count) {
        bool currentView = sender() == m_views.last()->scene();

        // Enable/disable alignments
        for (int i = ActionAlignLeft; i <= ActionAdjustSize; ++i)
            m_actionHandler->action(ActionType(i))->setEnabled(currentView && count >= 2);
        m_toolButtons[ToolButtonAlignment]->setEnabled(currentView && count >= 2);
        m_toolButtons[ToolButtonAdjustment]->setEnabled(currentView && count >= 2);
    });

    // Enable/disable color buttons
    connect(view->scene(), &GraphicsScene::selectedBaseItemCountChanged, this, [this](int count) {
        m_toolButtons[ToolButtonStateColor]->setEnabled(count > 0);
        m_toolButtons[ToolButtonFontColor]->setEnabled(count > 0);
    });

    connect(view->scene(), &GraphicsScene::pasteAvailable, this, [this](bool para) {
        bool currentView = sender() == m_views.last()->scene();
        m_actionHandler->action(ActionPaste)->setEnabled(currentView && para);
    });

    if (m_views.count() > 0)
        m_views.last()->scene()->unselectAll();

    if (item) {
        m_document->pushRootTag(item->tag());
        view->setDocument(m_document);
        m_searchPane->setDocument(m_document);
        m_structure->setDocument(m_document);
        m_stateProperties->setDocument(m_document);
        m_colorThemes->setDocument(m_document);
    }
    m_views << view;

    m_stackedWidget->setCurrentIndex(m_stackedWidget->addWidget(view));
}

void MainWidget::initView(int id)
{
    for (int i = 0; i < m_views.count(); ++i)
        m_views[i]->scene()->setTopMostScene(m_views[i] == m_views.last());

    // Init and connect current view
    auto view = qobject_cast<StateView*>(m_stackedWidget->widget(id));
    if (!view)
        return;

    m_searchPane->setGraphicsScene(view->scene());
    m_structure->setGraphicsScene(view->scene());
    m_navigator->setCurrentView(view->view());
    m_navigator->setCurrentScene(view->scene());
    m_magnifier->setCurrentView(view->view());
    m_magnifier->setCurrentScene(view->scene());
    view->scene()->unselectAll();
}

void MainWidget::newDocument()
{
    clear();
    addStateView();
    m_document->setFileName(QString());
    m_uiFactory->documentChanged(NewDocument, m_document);
    documentChanged();
}

void MainWidget::clear()
{
    // Clear and delete all stateviews
    while (m_views.count() > 0) {
        m_views.last()->clear();
        delete m_views.takeLast();
    }

    if (m_document)
        m_document->clear();
}

void MainWidget::handleTabVisibilityChanged(bool visible)
{
    QLayout *layout = m_mainContentWidget->layout();
    if (visible) {
        // Ensure that old widget is not splitter
        if (!qobject_cast<QSplitter*>(layout->itemAt(0)->widget())) {
            auto splitter = new QSplitter(Qt::Vertical);
            splitter->setHandleWidth(1);
            splitter->setChildrenCollapsible(false);
            while (layout->count() > 0) {
                QWidget *w = layout->takeAt(0)->widget();
                if (w)
                    splitter->addWidget(w);
            }
            layout->addWidget(splitter);
        }
    } else {
        // Ensure that old widget is splitter
        if (qobject_cast<QSplitter*>(layout->itemAt(0)->widget())) {
            auto splitter = static_cast<QSplitter*>(layout->takeAt(0)->widget());
            auto newLayout = new QVBoxLayout;
            newLayout->setContentsMargins(0, 0, 0, 0);
            if (splitter) {
                newLayout->addWidget(splitter->widget(0));
                newLayout->addWidget(splitter->widget(1));
                splitter->deleteLater();
            }
            delete layout;
            m_mainContentWidget->setLayout(newLayout);
        }
    }
}

bool MainWidget::load(const QString &fileName)
{
    clear();
    addStateView();
    m_document->load(fileName);
    m_uiFactory->documentChanged(AfterLoad, m_document);
    documentChanged();
    return !m_document->hasError();
}

void MainWidget::documentChanged()
{
    StateView *view = m_views.last();

    view->view()->setDrawingEnabled(false);
    view->view()->update();

    setEnabled(false);

    m_structure->setDocument(m_document);
    m_searchPane->setDocument(m_document);
    m_stateProperties->setDocument(m_document);
    m_colorThemes->setDocument(m_document);
    view->setDocument(m_document);

    if (!m_document->hasLayouted())
        view->scene()->runAutomaticLayout();

    view->view()->setDrawingEnabled(true);
    view->view()->fitSceneToView();

    undoStack()->clear();
    undoStack()->setClean();

    setEnabled(true);
    emit dirtyChanged(false);

    m_actionHandler->action(ActionFullNamespace)->setChecked(m_document->useFullNameSpace());
}

void MainWidget::createUi()
{
    m_outputPaneWindow = new OutputPane::OutputTabWidget;
    m_stackedWidget = new QStackedWidget;
    m_shapesFrame = new ShapesToolbox;

    m_mainContentWidget = new QWidget;
    m_mainContentWidget->setLayout(new QVBoxLayout);
    m_mainContentWidget->layout()->setMargin(0);
    m_mainContentWidget->layout()->addWidget(m_stackedWidget);
    m_mainContentWidget->layout()->addWidget(m_outputPaneWindow);

    m_stateProperties = new StateProperties;
    m_structure = new Structure;
    auto verticalSplitter = new Core::MiniSplitter(Qt::Vertical);
    verticalSplitter->addWidget(m_structure);
    verticalSplitter->addWidget(m_stateProperties);

    m_horizontalSplitter = new Core::MiniSplitter(Qt::Horizontal);
    m_horizontalSplitter->addWidget(m_shapesFrame);
    m_horizontalSplitter->addWidget(m_mainContentWidget);
    m_horizontalSplitter->addWidget(verticalSplitter);
    m_horizontalSplitter->setStretchFactor(0, 0);
    m_horizontalSplitter->setStretchFactor(1, 1);
    m_horizontalSplitter->setStretchFactor(2, 0);

    setLayout(new QVBoxLayout);
    layout()->addWidget(m_horizontalSplitter);
    layout()->setMargin(0);
}

void MainWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if (m_autoFit) {
        fitToView();
        m_autoFit = false;
    }
}

void MainWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    QRect r(QPoint(0, 0), e->size());
    QRect navigatorRect(m_navigator->pos(), m_navigator->size());

    if (!r.contains(navigatorRect)) {
        m_navigator->move(qBound(0, m_navigator->pos().x(), r.width() - navigatorRect.width() + 1),
            qBound(0, m_navigator->pos().y(), r.height() - navigatorRect.height() + 1));
    }

    int s = qMin(r.width(), r.height()) / 2;
    m_magnifier->setFixedSize(s, s);
    m_magnifier->setTopLeft(QPoint(m_shapesFrame->width(), 0));
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_magnifier->isVisible()) {
        QPoint p = event->pos() - m_magnifier->rect().center();
        p.setX(qBound(m_stackedWidget->x(), p.x(), m_stackedWidget->x() + m_stackedWidget->width()));
        p.setY(qBound(m_stackedWidget->y(), p.y(), m_stackedWidget->y() + m_stackedWidget->height()));
        m_magnifier->move(p);
    }

    QWidget::mouseMoveEvent(event);
}

void MainWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        if (e->key() == Qt::Key_F)
            m_outputPaneWindow->showPane(m_searchPane);
    }

    QWidget::keyPressEvent(e);
}

void MainWidget::refresh()
{
    m_uiFactory->refresh();
}

WarningModel *MainWidget::warningModel() const
{
    return m_errorPane->warningModel();
}

ScxmlUiFactory *MainWidget::uiFactory() const
{
    return m_uiFactory;
}

void MainWidget::setMagnifier(bool m)
{
    m_magnifier->setVisible(m);
    if (m) {
        QPoint p = mapFromGlobal(QCursor::pos());
        m_magnifier->move(p - m_magnifier->rect().center());
    }
}

QToolButton *MainWidget::createToolButton(const QIcon &icon, const QString &tooltip, QToolButton::ToolButtonPopupMode mode)
{
    auto button = new QToolButton;
    button->setIcon(icon);
    button->setToolTip(tooltip);
    button->setPopupMode(mode);

    return button;
}

void MainWidget::alignButtonClicked(ActionType alignType)
{
    if (alignType >= ActionAlignLeft && alignType <= ActionAlignVertical) {
        m_toolButtons[ToolButtonAlignment]->setIcon(toolButtonIcon(alignType));
        m_toolButtons[ToolButtonAlignment]->setToolTip(m_actionHandler->action(alignType)->toolTip());
        m_toolButtons[ToolButtonAlignment]->setProperty("currentAlignment", alignType);
        StateView *view = m_views.last();
        if (view)
            view->scene()->alignStates(alignType);
    }
}

void MainWidget::adjustButtonClicked(ActionType adjustType)
{
    if (adjustType >= ActionAdjustWidth && adjustType <= ActionAdjustSize) {
        m_toolButtons[ToolButtonAdjustment]->setIcon(toolButtonIcon(adjustType));
        m_toolButtons[ToolButtonAdjustment]->setToolTip(m_actionHandler->action(adjustType)->toolTip());
        m_toolButtons[ToolButtonAdjustment]->setProperty("currentAdjustment", adjustType);
        StateView *view = m_views.last();
        if (view)
            view->scene()->adjustStates(adjustType);
    }
}

QString MainWidget::contents() const
{
    return QLatin1String(m_document->content());
}

QUndoStack *MainWidget::undoStack() const
{
    return m_document->undoStack();
}

QString MainWidget::errorMessage() const
{
    return m_document->lastError();
}

QString MainWidget::fileName() const
{
    return m_document->fileName();
}

void MainWidget::setFileName(const QString &filename)
{
    m_document->setFileName(filename);
}

bool MainWidget::isDirty() const
{
    return m_document->changed();
}

void MainWidget::fitToView()
{
    StateView *view = m_views.last();
    if (view)
        view->view()->fitSceneToView();
}

bool MainWidget::save()
{
    m_uiFactory->documentChanged(BeginSave, m_document);
    bool ok = m_document->save();
    m_uiFactory->documentChanged(AfterSave, m_document);

    return ok;
}

bool MainWidget::event(QEvent *e)
{
    if (e->type() == QEvent::WindowBlocked)
        m_windowBlocked = true;

    if (e->type() == QEvent::WindowActivate) {
        if (m_windowBlocked)
            m_windowBlocked = false;
        else
            refresh();
    }

    return QWidget::event(e);
}
