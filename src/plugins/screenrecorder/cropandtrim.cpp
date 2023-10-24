// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cropandtrim.h"

#include "ffmpegutils.h"
#include "screenrecordersettings.h"
#include "screenrecordertr.h"

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/qtcsettings.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>

#include <QAction>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStyleOptionSlider>
#include <QToolButton>

using namespace Utils;

namespace ScreenRecorder {

CropScene::CropScene(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void CropScene::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.drawImage(QPointF(), m_buffer);
}

void CropScene::initMouseInteraction(const QPoint &imagePos)
{
    static const auto inGripRange = [](int grip, int pos, int &clickOffset) -> bool {
        const bool inRange = pos - m_gripWidth <= grip && pos + m_gripWidth >= grip;
        if (inRange)
            clickOffset = grip - pos;
        return inRange;
    };

    m_mouse.clickOffset = {};
    if (inGripRange(imagePos.x(), m_cropRect.left(), m_mouse.clickOffset.rx())) {
        m_mouse.margin = EdgeLeft;
        m_mouse.cursorShape = Qt::SizeHorCursor;
    } else if (inGripRange(imagePos.x(), m_cropRect.right(), m_mouse.clickOffset.rx())) {
        m_mouse.margin = EdgeRight;
        m_mouse.cursorShape = Qt::SizeHorCursor;
    } else if (inGripRange(imagePos.y(), m_cropRect.top(), m_mouse.clickOffset.ry())) {
        m_mouse.margin = EdgeTop;
        m_mouse.cursorShape = Qt::SizeVerCursor;
    } else if (inGripRange(imagePos.y(), m_cropRect.bottom(), m_mouse.clickOffset.ry())) {
        m_mouse.margin = EdgeBottom;
        m_mouse.cursorShape = Qt::SizeVerCursor;
    } else if (!fullySelected() && activeMoveArea().contains(imagePos)) {
        m_mouse.margin = Move;
        m_mouse.cursorShape = Qt::SizeAllCursor;
        m_mouse.clickOffset = imagePos - m_cropRect.topLeft();
    } else {
        m_mouse.margin = Free;
        m_mouse.cursorShape = Qt::ArrowCursor;
    }
}

void CropScene::updateBuffer()
{
    if (m_buffer.isNull())
        return;

    m_buffer.fill(palette().window().color());
    const qreal dpr = m_image->devicePixelRatioF();
    QPainter p(&m_buffer);
    p.drawImage(lineWidth, lineWidth, *m_image);

    const qreal lineOffset = lineWidth / 2.0;
    const QRectF r = {
        m_cropRect.x() / dpr + lineOffset,
        m_cropRect.y() / dpr + lineOffset,
        m_cropRect.width() / dpr + lineWidth,
        m_cropRect.height() / dpr + lineWidth
    };

    p.save();
    p.setClipRegion(QRegion(m_buffer.rect()).subtracted(QRegion(r.toRect())));
    p.setOpacity(0.85);
    p.fillRect(m_buffer.rect(), qRgb(0x30, 0x30, 0x30));
    p.restore();

    const auto paintLine = [&p](const QLineF &line)
    {
        const QPen blackPen(Qt::black, lineWidth);
        p.setPen(blackPen);
        p.drawLine(line);
        const QPen whiteDotPen(Qt::white, lineWidth, Qt::DotLine);
        p.setPen(whiteDotPen);
        p.drawLine(line);
    };
    paintLine(QLineF(r.left(), 0, r.left(), m_buffer.height()));
    paintLine(QLineF(0, r.top(), m_buffer.width(), r.top()));
    paintLine(QLineF(r.right(), 0, r.right(), m_buffer.height()));
    paintLine(QLineF(0, r.bottom(), m_buffer.width(), r.bottom()));

    update();
}

QPoint CropScene::toImagePos(const QPoint &widgetPos) const
{
    const int dpr = int(m_image->devicePixelRatio());
    return {(widgetPos.x() - lineWidth) * dpr, (widgetPos.y() - lineWidth) * dpr};
}

QRect CropScene::activeMoveArea() const
{
    const qreal minRatio = 0.22;  // At least 22% width and height of current selection
    const int minAbsoluteSize = 40;
    QRect result(0, 0, qMax(int(m_cropRect.width() * minRatio), minAbsoluteSize),
                 qMax(int(m_cropRect.height() * minRatio), minAbsoluteSize));
    result.moveCenter(m_cropRect.center());
    return result;
}

QRect CropScene::cropRect() const
{
    return m_cropRect;
}

void CropScene::setCropRect(const QRect &rect)
{
    m_cropRect = rect;
    updateBuffer();
    emit cropRectChanged(m_cropRect);
}

bool CropScene::fullySelected() const
{
    return m_image && m_image->rect() == m_cropRect;
}

void CropScene::setFullySelected()
{
    if (!m_image)
        return;
    setCropRect(m_image->rect());
}

QRect CropScene::fullRect() const
{
    return m_image ? m_image->rect() : QRect();
}

void CropScene::setImage(const QImage &image)
{
    m_image = &image;
    const QSize sceneSize = m_image->deviceIndependentSize().toSize()
                                .grownBy({lineWidth, lineWidth, lineWidth, lineWidth});
    const QSize sceneSizeDpr = sceneSize * m_image->devicePixelRatio();
    m_buffer = QImage(sceneSizeDpr, QImage::Format_RGB32);
    m_buffer.setDevicePixelRatio(m_image->devicePixelRatio());
    updateBuffer();
    resize(sceneSize);
}

QImage CropScene::croppedImage() const
{
    if (!m_image)
        return {};

    return m_image->copy(m_cropRect);
}

void CropScene::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint imagePos = toImagePos(event->pos());

    if (m_mouse.dragging) {
        switch (m_mouse.margin) {
        case EdgeLeft:
            m_cropRect.setLeft(qBound(0, imagePos.x() - m_mouse.clickOffset.x(),
                                      m_cropRect.right()));
            break;
        case EdgeTop:
            m_cropRect.setTop(qBound(0, imagePos.y() - m_mouse.clickOffset.y(),
                                     m_cropRect.bottom()));
            break;
        case EdgeRight:
            m_cropRect.setRight(qBound(m_cropRect.left(), imagePos.x() - m_mouse.clickOffset.x(),
                                       m_image->width() - 1));
            break;
        case EdgeBottom:
            m_cropRect.setBottom(qBound(m_cropRect.top(), imagePos.y() - m_mouse.clickOffset.y(),
                                        m_image->height() - 1));
            break;
        case Free:
            m_cropRect = QRect(m_mouse.startImagePos, imagePos).normalized()
                             .intersected(m_image->rect());
            break;
        case Move: {
            const QPoint topLeft(qBound(0, imagePos.x() - m_mouse.clickOffset.x(),
                                        m_image->width() - m_cropRect.width()),
                                 qBound(0, imagePos.y() - m_mouse.clickOffset.y(),
                                        m_image->height() - m_cropRect.height()));
            m_cropRect = QRect(topLeft, m_cropRect.size());
            break;
        }
        }
        emit cropRectChanged(m_cropRect);
        updateBuffer();
    } else {
        initMouseInteraction(imagePos);
        setCursor(m_mouse.cursorShape);
    }

    QWidget::mouseMoveEvent(event);
}

void CropScene::mousePressEvent(QMouseEvent *event)
{
    const QPoint imagePos = toImagePos(event->pos());

    m_mouse.dragging = true;
    m_mouse.startImagePos = imagePos;

    QWidget::mousePressEvent(event);
}

void CropScene::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouse.dragging = false;
    setCursor(Qt::ArrowCursor);

    QWidget::mouseReleaseEvent(event);
}

class CropWidget : public QWidget
{
public:
    explicit CropWidget(QWidget *parent = nullptr);

    QRect cropRect() const;
    void setCropRect(const QRect &rect);

    void setImage(const QImage &image);

private:
    void updateWidgets();
    void onSpinBoxChanged();
    void onCropRectChanged();

    CropScene *m_cropScene;

    QSpinBox *m_xSpinBox;
    QSpinBox *m_ySpinBox;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;

    CropSizeWarningIcon *m_warningIcon;
    QToolButton *m_resetButton;
};

CropWidget::CropWidget(QWidget *parent)
    : QWidget(parent)
{
    m_cropScene = new CropScene;

    auto scrollArea = new QScrollArea;
    scrollArea->setWidget(m_cropScene);

    for (auto s : {&m_xSpinBox, &m_ySpinBox, &m_widthSpinBox, &m_heightSpinBox}) {
        *s = new QSpinBox;
        (*s)->setMaximum(99999); // Will be adjusted in CropWidget::setImage
        (*s)->setSuffix(" px");
    }
    m_widthSpinBox->setMinimum(1);
    m_heightSpinBox->setMinimum(1);

    m_resetButton = new QToolButton;
    m_resetButton->setIcon(Icons::RESET.icon());

    m_warningIcon = new CropSizeWarningIcon(CropSizeWarningIcon::StandardVariant);

    auto saveImageButton = new QToolButton;
    saveImageButton->setToolTip(Tr::tr("Save current, cropped frame as image file."));
    saveImageButton->setIcon(Icons::SAVEFILE.icon());

    auto copyImageToClipboardAction = new QAction(Tr::tr("Copy current, cropped frame as image "
                                                         "to the clipboard."), this);
    copyImageToClipboardAction->setIcon(Icons::SNAPSHOT.icon());
    copyImageToClipboardAction->setShortcut(QKeySequence::Copy);

    auto copyImageToClipboardButton = new QToolButton;
    copyImageToClipboardButton->setDefaultAction(copyImageToClipboardAction);

    using namespace Layouting;
    Column {
        scrollArea,
        Row {
            Tr::tr("X:"), m_xSpinBox,
            Space(4), Tr::tr("Y:"), m_ySpinBox,
            Space(16), Tr::tr("Width:"), m_widthSpinBox,
            Space(4), Tr::tr("Height:"), m_heightSpinBox,
            m_resetButton,
            m_warningIcon,
            st,
            saveImageButton,
            copyImageToClipboardButton,
        },
        noMargin(),
    }.attachTo(this);

    connect(m_xSpinBox, &QSpinBox::valueChanged, this, &CropWidget::onSpinBoxChanged);
    connect(m_ySpinBox, &QSpinBox::valueChanged, this, &CropWidget::onSpinBoxChanged);
    connect(m_widthSpinBox, &QSpinBox::valueChanged, this, &CropWidget::onSpinBoxChanged);
    connect(m_heightSpinBox, &QSpinBox::valueChanged, this, &CropWidget::onSpinBoxChanged);
    connect(m_cropScene, &CropScene::cropRectChanged, this, &CropWidget::onCropRectChanged);
    connect(m_resetButton, &QToolButton::pressed, this, [this] {
        m_cropScene->setFullySelected();
    });
    connect(saveImageButton, &QToolButton::clicked, this, [this] {
        FilePathAspect &lastDir = Internal::settings().lastSaveImageDirectory;
        const QString ext(".png");
        FilePath file = FileUtils::getSaveFilePath(nullptr, Tr::tr("Save Current Frame As"),
                                                   lastDir(), "*" + ext);
        if (!file.isEmpty()) {
            if (!file.endsWith(ext))
                file = file.stringAppended(ext);
            lastDir.setValue(file.parentDir());
            lastDir.writeToSettingsImmediatly();
            const QImage image = m_cropScene->croppedImage();
            image.save(file.toString());
        }
    });
    connect(copyImageToClipboardAction, &QAction::triggered, this, [this] {
        const QImage image = m_cropScene->croppedImage();
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setImage(image);
    });

    updateWidgets();
}

QRect CropWidget::cropRect() const
{
    return m_cropScene->cropRect();
}

void CropWidget::setCropRect(const QRect &rect)
{
    m_cropScene->setCropRect(rect);
}

void CropWidget::setImage(const QImage &image)
{
    const QRect rBefore = m_cropScene->fullRect();
    m_cropScene->setImage(image);
    const QRect rAfter = m_cropScene->fullRect();
    if (rBefore != rAfter) {
        m_xSpinBox->setMaximum(rAfter.width() - 1);
        m_ySpinBox->setMaximum(rAfter.height() - 1);
        m_widthSpinBox->setMaximum(rAfter.width());
        m_heightSpinBox->setMaximum(rAfter.height());
    }
    updateWidgets();
}

void CropWidget::updateWidgets()
{
    m_resetButton->setEnabled(!m_cropScene->fullySelected());
    const QRect r = m_cropScene->cropRect();
    m_warningIcon->setCropSize(r.size());
}

void CropWidget::onSpinBoxChanged()
{
    const QRect spinBoxRect = {
        m_xSpinBox->value(),
        m_ySpinBox->value(),
        m_widthSpinBox->value(),
        m_heightSpinBox->value()
    };
    m_cropScene->setCropRect(spinBoxRect.intersected(m_cropScene->fullRect()));
}

void CropWidget::onCropRectChanged()
{
    const QRect rect = m_cropScene->cropRect();
    const struct { QSpinBox *spinBox; int value; } updates[] = {
                   {m_xSpinBox, rect.x()},
                   {m_ySpinBox, rect.y()},
                   {m_widthSpinBox, rect.width()},
                   {m_heightSpinBox, rect.height()},
                   };
    for (const auto &update : updates) {
        update.spinBox->blockSignals(true);
        update.spinBox->setValue(update.value);
        update.spinBox->blockSignals(false);
    }
    updateWidgets();
}

class SelectionSlider : public QSlider
{
public:
    explicit SelectionSlider(QWidget *parent = nullptr);

    void setSelectionRange(FrameRange range);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    FrameRange m_range;
};

SelectionSlider::SelectionSlider(QWidget *parent)
    : QSlider(Qt::Horizontal, parent)
    , m_range({-1, -1})
{
    setPageStep(50);
}

void SelectionSlider::setSelectionRange(FrameRange range)
{
    m_range = range;
    update();
}

void SelectionSlider::paintEvent(QPaintEvent *)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_SliderHandle; // Draw only the handle. We draw the rest, here.

    const int tickOffset = style()->pixelMetric(QStyle::PM_SliderTickmarkOffset, &opt, this);
    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderGroove, this)
                           .adjusted(tickOffset, 0, -tickOffset, 0);
    grooveRect.setTop(rect().top());
    grooveRect.setBottom(rect().bottom());
    const QColor bgColor = palette().window().color();
    const QColor fgColor = palette().text().color();
    const QColor grooveColor = StyleHelper::mergedColors(bgColor, fgColor, 80);
    const QColor selectionColor = StyleHelper::mergedColors(bgColor, fgColor, 45);
    QPainter p(this);
    p.fillRect(grooveRect, grooveColor);
    const qreal pixelsPerFrame = grooveRect.width() / qreal(maximum());
    const int startPixels = int(m_range.first * pixelsPerFrame);
    const int endPixels = int((maximum() - m_range.second) * pixelsPerFrame);
    p.fillRect(grooveRect.adjusted(startPixels, 0, -endPixels, 0), selectionColor);
    style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}

class TrimWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrimWidget(const ClipInfo &clip, QWidget *parent = nullptr);

    void setCurrentFrame(int frame);
    int currentFrame() const;
    void setTrimRange(FrameRange range);
    FrameRange trimRange() const;

signals:
    void positionChanged();
    void trimRangeChanged(FrameRange range);

private:
    void resetTrimRange();
    void updateTrimWidgets();
    void emitTrimRangeChange();

    ClipInfo m_clipInfo;
    SelectionSlider *m_frameSlider;
    TimeLabel *m_currentTime;
    TimeLabel *m_clipDuration;

    struct {
        QPushButton *button;
        TimeLabel *timeLabel;
    } m_trimStart, m_trimEnd;
    TimeLabel *m_trimRange;
    QToolButton *m_trimResetButton;
};

TrimWidget::TrimWidget(const ClipInfo &clip, QWidget *parent)
    : QWidget(parent)
    , m_clipInfo(clip)
{
    m_frameSlider = new SelectionSlider;

    m_currentTime = new TimeLabel(m_clipInfo);

    m_clipDuration = new TimeLabel(m_clipInfo);

    m_trimStart.button = new QPushButton(Tr::tr("Start:"));
    m_trimStart.timeLabel = new TimeLabel(m_clipInfo);

    m_trimEnd.button = new QPushButton(Tr::tr("End:"));
    m_trimEnd.timeLabel = new TimeLabel(m_clipInfo);

    m_trimRange = new TimeLabel(m_clipInfo);

    m_trimResetButton = new QToolButton;
    m_trimResetButton->setIcon(Icons::RESET.icon());

    using namespace Layouting;
    Column {
        Row { m_frameSlider, m_currentTime, "/", m_clipDuration },
        Group {
            title(Tr::tr("Trimming")),
            Row {
                m_trimStart.button, m_trimStart.timeLabel,
                Space(20),
                m_trimEnd.button, m_trimEnd.timeLabel,
                Stretch(), Space(20),
                Tr::tr("Range:"), m_trimRange,
                m_trimResetButton,
            },
        },
        noMargin(),
    }.attachTo(this);

    connect(m_frameSlider, &QSlider::valueChanged, this, [this]() {
        m_currentTime->setFrame(currentFrame());
        updateTrimWidgets();
        emit positionChanged();
    });
    connect(m_trimStart.button, &QPushButton::clicked, this, [this] (){
        m_trimStart.timeLabel->setFrame(currentFrame());
        updateTrimWidgets();
        emitTrimRangeChange();
    });
    connect(m_trimEnd.button, &QPushButton::clicked, this, [this] (){
        m_trimEnd.timeLabel->setFrame(currentFrame());
        updateTrimWidgets();
        emitTrimRangeChange();
    });
    connect(m_trimResetButton, &QToolButton::clicked, this, &TrimWidget::resetTrimRange);

    m_frameSlider->setMaximum(m_clipInfo.framesCount());
    m_currentTime->setFrame(currentFrame());
    m_clipDuration->setFrame(m_clipInfo.framesCount());
    resetTrimRange();
}

void TrimWidget::setCurrentFrame(int frame)
{
    m_frameSlider->setValue(frame);
}

int TrimWidget::currentFrame() const
{
    return m_frameSlider->value();
}

void TrimWidget::setTrimRange(FrameRange range)
{
    m_trimStart.timeLabel->setFrame(range.first);
    m_trimEnd.timeLabel->setFrame(range.second);
    m_frameSlider->setSelectionRange(trimRange());
}

FrameRange TrimWidget::trimRange() const
{
    return { m_trimStart.timeLabel->frame(), m_trimEnd.timeLabel->frame() };
}

void TrimWidget::resetTrimRange()
{
    setTrimRange({0, m_clipInfo.framesCount()});
    emitTrimRangeChange();
    updateTrimWidgets();
}

void TrimWidget::updateTrimWidgets()
{
    const int current = currentFrame();
    const int trimStart = m_trimStart.timeLabel->frame();
    const int trimEnd = m_trimEnd.timeLabel->frame();
    m_trimStart.button->setEnabled(current < m_clipInfo.framesCount() && current < trimEnd);
    m_trimEnd.button->setEnabled(current > 0 && current > trimStart);
    m_trimRange->setFrame(trimEnd - trimStart);
    m_frameSlider->setSelectionRange(trimRange());
    m_trimResetButton->setEnabled(!m_clipInfo.isCompleteRange(trimRange()));
}

void TrimWidget::emitTrimRangeChange()
{
    emit trimRangeChanged(trimRange());
}

class CropAndTrimDialog : public QDialog
{
public:
    explicit CropAndTrimDialog(const ClipInfo &clip, QWidget *parent = nullptr);

    void setCropRect(const QRect &rect);
    QRect cropRect() const;
    void setTrimRange(FrameRange range);
    FrameRange trimRange() const;

    int currentFrame() const;
    void setCurrentFrame(int frame);

private:
    void onSeekPositionChanged();
    void startFrameFetch();

    ClipInfo m_clipInfo;
    CropWidget *m_cropWidget;
    TrimWidget *m_trimWidget;
    QImage m_previewImage;

    Process *m_process;
    int m_nextFetchFrame = -1;
};

CropAndTrimDialog::CropAndTrimDialog(const ClipInfo &clip, QWidget *parent)
    : QDialog(parent, Qt::Window)
    , m_clipInfo(clip)
{
    setWindowTitle(Tr::tr("Crop and Trim"));

    m_cropWidget = new CropWidget;

    m_trimWidget = new TrimWidget(m_clipInfo);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        Group {
            title("Cropping"),
            Column { m_cropWidget },
        },
        Space(16),
        m_trimWidget,
        buttonBox,
    }.attachTo(this);

    m_process = new Process(this);
    connect(m_process, &Process::done, this, [this] {
        if (m_process->exitCode() != 0) {
            FFmpegUtils::reportError(m_process->commandLine(),
                                     m_process->readAllRawStandardError());
            return;
        }
        const QByteArray &imageData = m_process->rawStdOut();
        startFrameFetch();
        if (imageData.isEmpty())
            return;
        m_previewImage = QImage(reinterpret_cast<const uchar*>(imageData.constData()),
                                m_clipInfo.dimensions.width(), m_clipInfo.dimensions.height(),
                                QImage::Format_RGB32);
        m_previewImage.detach();
        m_cropWidget->setImage(m_previewImage);
    });
    connect(m_trimWidget, &TrimWidget::positionChanged,
            this, &CropAndTrimDialog::onSeekPositionChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onSeekPositionChanged();
    resize(1000, 800);
}

void CropAndTrimDialog::onSeekPositionChanged()
{
    // -1, because frame numbers are 0-based
    m_nextFetchFrame = qMin(m_trimWidget->currentFrame(), m_clipInfo.framesCount() - 1);
    if (!m_process->isRunning())
        startFrameFetch();
}

void CropAndTrimDialog::startFrameFetch()
{
    if (m_nextFetchFrame == -1)
        return;

    const CommandLine cl = {
        Internal::settings().ffmpegTool(),
        {
            "-v", "error",
            "-ss", m_clipInfo.timeStamp(m_nextFetchFrame),
            "-i", m_clipInfo.file.toUserOutput(),
            "-threads", "1",
            "-frames:v", "1",
            "-f", "rawvideo",
            "-pix_fmt", "bgra",
            "-"
        }
    };
    m_process->close();
    m_nextFetchFrame = -1;
    m_process->setCommand(cl);
    m_process->setWorkingDirectory(Internal::settings().ffmpegTool().parentDir());
    m_process->start();
}

void CropAndTrimDialog::setCropRect(const QRect &rect)
{
    m_cropWidget->setCropRect(rect);
}

QRect CropAndTrimDialog::cropRect() const
{
    return m_cropWidget->cropRect();
}

void CropAndTrimDialog::setTrimRange(FrameRange range)
{
    m_trimWidget->setTrimRange(range);
}

FrameRange CropAndTrimDialog::trimRange() const
{
    return m_trimWidget->trimRange();
}

int CropAndTrimDialog::currentFrame() const
{
    return m_trimWidget->currentFrame();
}

void CropAndTrimDialog::setCurrentFrame(int frame)
{
    m_trimWidget->setCurrentFrame(frame);
}

CropAndTrimWidget::CropAndTrimWidget(QWidget *parent)
    : StyledBar(parent)
{
    m_button = new QToolButton;
    m_button->setText(Tr::tr("Crop and Trim..."));

    m_cropSizeWarningIcon = new CropSizeWarningIcon(CropSizeWarningIcon::ToolBarVariant);

    using namespace Layouting;
    Row {
        m_button,
        m_cropSizeWarningIcon,
        noMargin(), spacing(0),
    }.attachTo(this);

    connect(m_button, &QPushButton::clicked, this, [this] {
        CropAndTrimDialog dlg(m_clipInfo, Core::ICore::dialogParent());
        dlg.setCropRect(m_cropRect);
        dlg.setTrimRange(m_trimRange);
        dlg.setCurrentFrame(m_currentFrame);
        if (dlg.exec() == QDialog::Accepted) {
            m_cropRect = dlg.cropRect();
            m_trimRange = dlg.trimRange();
            m_currentFrame = dlg.currentFrame();
            emit cropRectChanged(m_cropRect);
            emit trimRangeChanged(m_trimRange);
            updateWidgets();
        }
    });

    updateWidgets();
}

void CropAndTrimWidget::setClip(const ClipInfo &clip)
{
    if (clip.dimensions != m_clipInfo.dimensions)
        m_cropRect = {QPoint(), clip.dimensions}; // Reset only if clip size changed
    m_clipInfo = clip;
    m_currentFrame = 0;
    m_trimRange = {m_currentFrame, m_clipInfo.framesCount()};
    updateWidgets();
}

void CropAndTrimWidget::updateWidgets()
{
    if (!m_clipInfo.isNull()) {
        const QString cropText =
            !m_clipInfo.isCompleteArea(m_cropRect)
                ? Tr::tr("Crop to %1x%2px.").arg(m_cropRect.width()).arg(m_cropRect.height())
                : Tr::tr("Complete area.");

        const QString trimText =
            !m_clipInfo.isCompleteRange(m_trimRange)
                ? Tr::tr("Frames %1 to %2.").arg(m_trimRange.first).arg(m_trimRange.second)
                : Tr::tr("Complete clip.");

        m_button->setToolTip(cropText + " " + trimText);
    }

    m_cropSizeWarningIcon->setCropSize(m_cropRect.size());
}

} // namespace ScreenRecorder

#include "cropandtrim.moc"
