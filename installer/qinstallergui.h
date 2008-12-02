/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QINSTALLERGUI_H
#define QINSTALLERGUI_H

#include <QtGui/QWizard>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QInstaller;

// FIXME: move to private classes
class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QRadioButton;
class QTreeView;
class QTreeWidget;

class QInstallerGui : public QWizard
{
    Q_OBJECT

public:
    explicit QInstallerGui(QInstaller *installer, QWidget *parent = 0);

signals:
    void interrupted();

public slots:
    void cancelButtonClicked();
    void reject();
    void showFinishedPage();
    void showWarning(const QString &msg);
};


class QInstallerPage : public QWizardPage
{
    Q_OBJECT

public:
    QInstallerPage(QInstaller *installer);

    virtual bool isInterruptible() const { return false; }
    virtual QPixmap watermarkPixmap() const;
    virtual QPixmap logoPixmap() const;
    virtual QString productName() const;

protected:
    QInstaller *installer() const;

    // Inserts widget into the same layout like a sibling identified
    // by its name. Default position is just behind the sibling.
    virtual void insertWidget(QWidget *widget, const QString &siblingName,
        int offset = 1);
    virtual QWidget *findWidget(const QString &objectName) const;

    virtual void setVisible(bool visible); // reimp
    virtual int nextId() const; // reimp

    virtual void entering() {} // called on entering
    virtual void leaving() {}  // called on leaving

    virtual void forward() const {} // called when going forwards
    //virtual void backward() const {}  // called when going back
    bool isConstructing() const { return m_fresh; }

private:
    QInstaller *m_installer;
    bool m_fresh;
};


class QInstallerIntroductionPage : public QInstallerPage
{
public:
    explicit QInstallerIntroductionPage(QInstaller *installer);
};


class QInstallerLicenseAgreementPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerLicenseAgreementPage(QInstaller *installer);
    bool isComplete() const;

private:
    QRadioButton *m_acceptRadioButton;
    QRadioButton *m_rejectRadioButton;
};


class QInstallerComponentSelectionPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerComponentSelectionPage(QInstaller *installer);
    ~QInstallerComponentSelectionPage();

protected:
    //void entering();
    void leaving();

private:
    class Private;
    Private *d;
};


class QInstallerTargetDirectoryPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerTargetDirectoryPage(QInstaller *installer);
    QString targetDir() const;
    void setTargetDir(const QString &dirName);

protected:
    void entering();
    void leaving();

private slots:
    void targetDirSelected();
    void dirRequested();

private:
    QLineEdit *m_lineEdit;
};


class QInstallerReadyForInstallationPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerReadyForInstallationPage(QInstaller *installer);
};


class QInstallerPerformInstallationPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerPerformInstallationPage(QInstaller *installer);

protected:
    void initializePage();
    bool isComplete() const;
    bool isInterruptible() const { return true; }

signals:
    void installationRequested();

private slots:
    void installationStarted();
    void installationFinished();
    void updateProgress();

private:
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QTimer *m_updateTimer;
};


class QInstallerPerformUninstallationPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerPerformUninstallationPage(QInstaller *installer);

protected:
    void initializePage();
    bool isComplete() const;
    bool isInterruptible() const { return true; }

signals:
    void uninstallationRequested();

private slots:
    void uninstallationStarted();
    void uninstallationFinished();
    void updateProgress();

private:
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QTimer *m_updateTimer;
};


class QInstallerFinishedPage : public QInstallerPage
{
    Q_OBJECT

public:
    explicit QInstallerFinishedPage(QInstaller *installer);

public slots:
    void handleFinishClicked();

protected:
    void entering();
    void leaving();

private:
    QCheckBox *m_runItCheckBox;
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // QINSTALLERGUI_H
