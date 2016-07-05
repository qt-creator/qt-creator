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

#pragma once

#include <texteditor/texteditor.h>

#include <QAbstractListModel>
#include <QStackedWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDomDocument;
class QDomElement;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QListView;
class QSpinBox;
class QToolButton;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace Android {
namespace Internal {
class AndroidManifestEditor;
class AndroidManifestEditorWidget;


class PermissionsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    PermissionsModel(QObject *parent = 0 );
    void setPermissions(const QStringList &permissions);
    const QStringList &permissions();
    QModelIndex addPermission(const QString &permission);
    bool updatePermission(const QModelIndex &index, const QString &permission);
    void removePermission(int index);
    QVariant data(const QModelIndex &index, int role) const;

protected:
    int rowCount(const QModelIndex &parent) const;

private:
    QStringList m_permissions;
};

class AndroidManifestTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    explicit AndroidManifestTextEditorWidget(AndroidManifestEditorWidget *parent);
};

class AndroidManifestEditorWidget : public QStackedWidget
{
    Q_OBJECT
public:
    enum EditorPage {
        General = 0,
        Source = 1
    };

    explicit AndroidManifestEditorWidget();

    bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    void preSave();
    void postSave();

    Core::IEditor *editor() const;
    TextEditor::TextEditorWidget *textEditorWidget() const;

    void setDirty(bool dirty = true);

signals:
    void guiChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void setLDPIIcon();
    void setMDPIIcon();
    void setHDPIIcon();
    void defaultPermissionOrFeatureCheckBoxClicked();
    void addPermission();
    void removePermission();
    void updateAddRemovePermissionButtons();
    void setPackageName();
    void updateInfoBar();
    void updateSdkVersions();
    void startParseCheck();
    void delayedParseCheck();
    void initializePage();
    bool syncToWidgets();
    void syncToWidgets(const QDomDocument &doc);
    void syncToEditor();
    void updateAfterFileLoad();

    bool checkDocument(const QDomDocument &doc, QString *errorMessage,
                       int *errorLine, int *errorColumn);
    enum IconDPI { LowDPI, MediumDPI, HighDPI };
    QIcon icon(const QString &baseDir, IconDPI dpi);
    QString iconPath(const QString &baseDir, IconDPI dpi);
    void copyIcon(IconDPI dpi, const QString &baseDir, const QString &filePath);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();
    void updateTargetComboBox();

    void parseManifest(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseApplication(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseActivity(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    bool parseMetaData(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseUsesSdk(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    QString parseUsesPermission(QXmlStreamReader &reader,
                                QXmlStreamWriter &writer,
                                const QSet<QString> &permissions);
    QString parseComment(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseUnknownElement(QXmlStreamReader &reader, QXmlStreamWriter &writer);

    bool m_dirty; // indicates that we need to call syncToEditor()
    bool m_stayClean;
    int m_errorLine;
    int m_errorColumn;

    QLineEdit *m_packageNameLineEdit;
    QLabel *m_packageNameWarningIcon;
    QLabel *m_packageNameWarning;
    QSpinBox *m_versionCode;
    QLineEdit *m_versionNameLinedit;
    QComboBox *m_androidMinSdkVersion;
    QComboBox *m_androidTargetSdkVersion;

    // Application
    QLineEdit *m_appNameLineEdit;
    QLineEdit *m_activityNameLineEdit;
    QComboBox *m_targetLineEdit;
    QToolButton *m_lIconButton;
    QToolButton *m_mIconButton;
    QToolButton *m_hIconButton;
    QString m_lIconPath; // only set if the user changed the icon
    QString m_mIconPath;
    QString m_hIconPath;

    // Permissions
    QCheckBox *m_defaultPermissonsCheckBox;
    QCheckBox *m_defaultFeaturesCheckBox;
    PermissionsModel *m_permissionsModel;
    QListView *m_permissionsListView;
    QPushButton *m_addPermissionButton;
    QPushButton *m_removePermissionButton;
    QComboBox *m_permissionsComboBox;

    QTimer m_timerParseCheck;
    TextEditor::TextEditorWidget *m_textEditorWidget;
    AndroidManifestEditor *m_editor;
};
} // namespace Internal
} // namespace Android
