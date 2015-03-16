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

#ifndef ANDROIDMANIFESTEDITORWIDGET_H
#define ANDROIDMANIFESTEDITORWIDGET_H

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

    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    void preSave();
    void postSave();

    Core::IEditor *editor() const;
    TextEditor::TextEditorWidget *textEditorWidget() const;

public slots:
    void setDirty(bool dirty = true);

signals:
    void guiChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
private slots:
    void setLDPIIcon();
    void setMDPIIcon();
    void setHDPIIcon();
    void defaultPermissionOrFeatureCheckBoxClicked();
    void addPermission();
    void removePermission();
    void updateAddRemovePermissionButtons();
    void setAppName();
    void setPackageName();
    void updateInfoBar();
    void updateSdkVersions();
    void startParseCheck();
    void delayedParseCheck();
private:
    void initializePage();
    bool syncToWidgets();
    void syncToWidgets(const QDomDocument &doc);
    void syncToEditor();

    bool checkDocument(const QDomDocument &doc, QString *errorMessage,
                       int *errorLine, int *errorColumn);
    enum IconDPI { LowDPI, MediumDPI, HighDPI };
    QIcon icon(const QString &baseDir, IconDPI dpi);
    QString iconPath(const QString &baseDir, IconDPI dpi);
    void copyIcon(IconDPI dpi, const QString &baseDir, const QString &filePath);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();
    Q_SLOT void updateTargetComboBox();

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
    bool m_setAppName;
    bool m_appNameInStringsXml;
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


#endif // ANDROIDMANIFESTEDITORWIDGET_H
