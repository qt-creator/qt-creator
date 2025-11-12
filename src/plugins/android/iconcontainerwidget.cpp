// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iconcontainerwidget.h"
#include "splashscreencontainerwidget.h"
#include "androidtoolmenu.h"
#include "androidtr.h"

#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QPointer>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/fileutils.h>
#include <utils/icon.h>

namespace Android::Internal  {

using namespace Utils;

class IconWidget : public QLabel {
    Q_OBJECT

public:
    IconWidget(QWidget *parent = nullptr) : IconWidget(parent, {}, {}, {}, {}, nullptr, {}, {}) {};

    Utils::FilePath getTargetPath() const;

    IconWidget(QWidget *parent, const QSize &displaySize, const QSize &pixmapSize, const QString &title,
               const QString &tooltip, TextEditor::TextEditorWidget *textEditorWidget,
               const QString &pathPrefix = QString(), const QString &targetFileName = QString());

    void setIcon(const QIcon &icon);
    void setTargetIconFileName(const QString &name) { m_targetFileName = name; }
    Result<void> saveIcon();
    void loadIcon();
    bool hasIcon() const { return m_hasIcon; }

signals:
    void iconSelected();
    void iconRemoved();

public slots:
    void setIconFromPath();

private slots:
    void removeIcon();

protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    void initDefaults();
    QSize m_displaySize;
    QSize m_pixmapSize;
    QString m_title;
    QString m_tooltip;
    QPointer<TextEditor::TextEditorWidget> m_textEditor = nullptr;
    QString m_pathPrefix;
    QString m_targetFileName;
    QPixmap m_pixmap;
    bool m_hasIcon = false;
};

void IconWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu m(this);
    QAction *remove = m.addAction(Android::Tr::tr("Remove icon"));
    connect(remove, &QAction::triggered, this, &IconWidget::removeIcon);
    m.exec(ev->globalPos());
}

void IconWidget::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev)

    const QString file = QFileDialog::getOpenFileName(this, Android::Tr::tr("Select icon"), QString(),
                                                      Android::Tr::tr("Images (*.png *.jpg *.jpeg *.svg)"));
    if (file.isEmpty())
        return;

    QPixmap pix(file);
    if (pix.isNull())
        return;

    m_pixmap = pix.scaled(m_pixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(m_pixmap.scaled(m_displaySize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_hasIcon = true;

    if (!saveIcon())
        qWarning() << "Failed to save icon immediately after loading";

    emit iconSelected();
};

IconWidget::IconWidget(QWidget *parent, const QSize &displaySize, const QSize &pixmapSize, const QString &title,
                       const QString &tooltip, TextEditor::TextEditorWidget *textEditorWidget,
                       const QString &pathPrefix, const QString &targetFileName): QLabel(parent),
                        m_displaySize(displaySize), m_pixmapSize(pixmapSize), m_title(title),
                        m_tooltip(tooltip), m_textEditor(textEditorWidget), m_targetFileName(targetFileName)
{
    if (pathPrefix.isEmpty()) {
        m_pathPrefix = QString();
    } else {
        m_pathPrefix = pathPrefix;
    }

    initDefaults();
    setToolTip(m_tooltip);
    setFixedSize(m_displaySize);
}

void IconWidget::initDefaults()
{
    setAlignment(Qt::AlignCenter);
    setFrameStyle(QFrame::Panel | QFrame::Raised);
    setFrameShadow(QFrame::Raised);
    setAutoFillBackground(true);
    setMinimumSize(64, 64);
    m_displaySize = m_displaySize.isEmpty() ? QSize(64, 64) : m_displaySize;
    m_pixmapSize = m_pixmapSize.isEmpty() ? m_displaySize : m_pixmapSize;
}

void IconWidget::setIconFromPath()
{
    QObject *s = sender();
    if (!s)
        return;
    auto src = qobject_cast<IconWidget *>(s);
    if (!src)
        return;
    if (!src->m_hasIcon)
        return;
    m_pixmap = src->m_pixmap;
    setPixmap(m_pixmap.scaled(m_displaySize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_hasIcon = true;
    emit iconSelected();
}

Utils::FilePath IconWidget::getTargetPath() const
{
    if (m_pathPrefix.isEmpty() || m_targetFileName.isEmpty() || !m_textEditor) {
        qWarning() << "Cannot determine target path - missing required parameters";
        return {};
    }

    const Utils::FilePath baseDir = manifestDir(m_textEditor.data());
    if (baseDir.isEmpty()) {
        qWarning() << "Base directory path is empty";
        return {};
    }

    if (!baseDir.exists()) {
        qWarning() << "Base directory does not exist:" << baseDir.toUserOutput();
        return {};
    }

    return baseDir / m_pathPrefix / m_targetFileName;
}

void IconWidget::setIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        removeIcon();
        return;
    }
    m_pixmap = icon.pixmap(m_pixmapSize);
    setPixmap(m_pixmap.scaled(m_displaySize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_hasIcon = true;
}

void IconWidget::removeIcon()
{
    m_pixmap = QPixmap();
    clear();
    m_hasIcon = false;
    emit iconRemoved();
}

void IconWidget::loadIcon()
{
    if (m_pathPrefix.isEmpty() || m_targetFileName.isEmpty() || !m_textEditor)
        return;

    const Utils::FilePath baseDir = manifestDir(m_textEditor.data());
    if (baseDir.isEmpty())
        return;

    const Utils::FilePath targetPath = getTargetPath();

    const QString p = targetPath.toUrlishString();
    if (QFile::exists(p)) {
        QPixmap pix(p);
        if (!pix.isNull()) {
            m_pixmap = pix.scaled(m_pixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            setPixmap(m_pixmap.scaled(m_displaySize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_hasIcon = true;
        }
    }
}

Result<void> IconWidget::saveIcon()
{
    if (!m_hasIcon)
        return Utils::ResultError("Icon data not available for saving.");

    const Utils::FilePath targetPath = getTargetPath();
    const QString targetFileName = targetPath.toUrlishString();
    QFileInfo fi(targetFileName);

    QDir dir = fi.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath()))
            return Utils::ResultError(QString("Failed to create directory: " + dir.absolutePath()));
    }

    QFile file(targetFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        bool saved = m_pixmap.save(&file, "PNG");
        file.close();

        if (!saved)
            return Utils::ResultError(QString("Failed to save pixmap data to file: " + targetFileName));
        if (QFile::exists(targetPath.toUrlishString())) {
            return Utils::ResultOk;
        } else {
            return Utils::ResultError(QString("Icon file does not exist after writing and closing: " + targetFileName));
        }
    }
    return Utils::ResultError(QString("Cannot open file for writing: %1 (Error: %2)").arg(targetPath.toUserOutput(), file.errorString()));
}


const char extraExtraExtraHighDpiIconPath[] = "/res/drawable-xxxhdpi/";
const char extraExtraHighDpiIconPath[] = "/res/drawable-xxhdpi/";
const char extraHighDpiIconPath[] = "/res/drawable-xhdpi/";
const char highDpiIconPath[] = "/res/drawable-hdpi/";
const char mediumDpiIconPath[] = "/res/drawable-mdpi/";
const char lowDpiIconPath[] = "/res/drawable-ldpi/";
const char imageSuffix[] = ".png";
const QSize lowDpiIconSize{32, 32};
const QSize mediumDpiIconSize{48, 48};
const QSize highDpiIconSize{72, 72};
const QSize extraHighDpiIconSize{96, 96};
const QSize extraExtraHighDpiIconSize{144, 144};
const QSize extraExtraExtraHighDpiIconSize{192, 192};

Android::Internal::IconContainerWidget::IconContainerWidget(QWidget *parent)
    : QWidget(parent), m_iconLayout(nullptr), m_textEditor(nullptr)
{
    m_iconLayout = new QHBoxLayout(this);
    m_masterIconButton = new QToolButton(this);

    const QString masterButtonLabel = Tr::tr("Select master icon");
    m_masterIconButton->setText(masterButtonLabel);
    m_masterIconButton->setToolTip(Tr::tr("Select master icon."));
    m_masterIconButton->setIconSize(lowDpiIconSize);
    m_masterIconButton->setIcon(Icon::fromTheme("document-open"));
    m_masterIconButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_masterIconButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_iconLayout->addWidget(m_masterIconButton);
}

struct IconConfig {
    QSize size;
    const char* title;
    const char* tooltip;
    const QString path;
};

static const IconConfig iconConfigs[] = {
    {lowDpiIconSize, "LDPI icon", "Select an icon suitable for low-density (ldpi) screens (~120dpi).", lowDpiIconPath},
    {mediumDpiIconSize, "MDPI icon", "Select an icon for medium-density (mdpi) screens (~160dpi).", mediumDpiIconPath},
    {highDpiIconSize, "HDPI icon", "Select an icon for high-density (hdpi) screens (~240dpi).", highDpiIconPath},
    {extraHighDpiIconSize, "XHDPI icon", "Select an icon for extra-high-density (xhdpi) screens (~320dpi).", extraHighDpiIconPath},
    {extraExtraHighDpiIconSize, "XXHDPI icon", "Select an icon for extra-extra-high-density (xxhdpi) screens (~480dpi).", extraExtraHighDpiIconPath},
    {extraExtraExtraHighDpiIconSize, "XXXHDPI icon", "Select an icon for extra-extra-extra-high-density (xxxhdpi) screens (~640dpi).", extraExtraExtraHighDpiIconPath},
    };

void Android::Internal::IconContainerWidget::initialize(TextEditor::TextEditorWidget *textEditorWidget)
{
    QTC_ASSERT(textEditorWidget, return);
    m_textEditor = textEditorWidget;

    const QString iconFileName = m_iconFileName + imageSuffix;
    m_iconLayout->addStretch(1);

    for (const auto &config : iconConfigs) {
        auto iconButton = new IconWidget(this, config.size, config.size,
                                         Tr::tr(config.title), Tr::tr(config.tooltip),
                                         textEditorWidget, config.path, iconFileName);
        m_iconLayout->addWidget(iconButton);
        m_iconLayout->addStretch(1);
        m_iconButtons.push_back(iconButton);
    }

    auto handleIconModification = [this] {
        bool iconsMaybeChanged = hasIcons();
        if (m_hasIcons != iconsMaybeChanged) {
            auto result = saveIcons();
            if (!result)
                qWarning() << "Failed to save icons after modification:" << result.error();
            emit iconsModified();
        }
        m_hasIcons = iconsMaybeChanged;
    };

    for (auto &&iconButton : m_iconButtons) {
        connect(iconButton, &IconWidget::iconRemoved, this, handleIconModification);
        connect(iconButton, &IconWidget::iconSelected, this, handleIconModification);
    }

    if (m_masterIconButton) {
        connect(m_masterIconButton, &QAbstractButton::clicked, this, [this, handleIconModification]() {
            const auto iconFile = IconContainerWidget::iconFile(lowDpiIconPath);

            if (iconFile.isEmpty())
                return;

            const QString filePath = iconFile.toUrlishString();
            QPixmap basePix(filePath);
            if (basePix.isNull()) {
                qWarning() << "Selected master icon could not be loaded:" << filePath;
                return;
            }

            for (auto &&iconButton : m_iconButtons) {
                iconButton->setIcon(QIcon(basePix));
                auto result = iconButton->saveIcon();
                if (!result) {
                    if (!result)
                        qWarning() << "Failed to save scaled icon for one of the DPI variants:" << result.error();
                }
            }

            handleIconModification();
        });
    }

    loadIcons();
}

Utils::FilePath IconContainerWidget::iconFile(const Utils::FilePath &path)
{
    return FileUtils::getOpenFilePath(
        path.toUrlishString(),
        FileUtils::homePath(),
        //: %1 expands to wildcard list for file dialog, do not change order
        Tr::tr("Images %1")
            .arg("(*.png *.jpg *.jpeg *.webp *.svg)")); // TODO: See SplashContainterWidget
}

void IconContainerWidget::loadIcons()
{
    for (auto &&iconButton : m_iconButtons) {
        iconButton->setTargetIconFileName(m_iconFileName + imageSuffix);
        iconButton->loadIcon();
    }
    m_hasIcons = hasIcons();
}

bool IconContainerWidget::hasIcons() const
{
    for (auto &&iconButton : m_iconButtons) {
        if (iconButton->hasIcon())
            return true;
    }
    return false;
}

Core::IDocument *IconEditor::document() const
{
    return m_document.get();
}

IconEditor::IconEditor(QWidget *widget)
    : m_widget(widget)
{
    m_toolBar = new QToolBar(widget);
    m_document = std::make_unique<Core::IDocument>();

    setWidget(widget);
}

QWidget *IconEditor::toolBar()
{
    return m_toolBar;
}

TextEditor::TextEditorWidget *IconContainerWidget::textEditor() const
{
    return m_textEditor.data();
}

Result<void> IconContainerWidget::saveIcons()
{
    if (!m_textEditor)
        return Utils::ResultError("No text editor available");

    for (auto &&iconButton : m_iconButtons) {
        if (iconButton->hasIcon()) {
            auto result = iconButton->saveIcon();
            if (!result)
                return result;
        }
    }
    return Utils::ResultOk;
}

IconWidget *IconEditor::ownWidget() const
{
    return static_cast<IconWidget *>(m_widget);
}

#include "iconcontainerwidget.moc"

}; // Android::Internal
