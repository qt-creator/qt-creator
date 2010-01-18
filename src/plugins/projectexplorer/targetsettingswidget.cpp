#include "targetsettingswidget.h"
#include "ui_targetsettingswidget.h"

static int WIDTH = 750;

using namespace ProjectExplorer::Internal;

TargetSettingsWidget::TargetSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TargetSettingsWidget),
    m_targetSelector(new TargetSelector(this))
{
    ui->setupUi(this);
    ui->separator->setStyleSheet(QLatin1String("* { "
        "background-image: url(:/projectexplorer/images/targetseparatorbackground.png);"
        "}"));
    m_targetSelector->raise();
    connect(m_targetSelector, SIGNAL(addButtonClicked()),
            this, SIGNAL(addButtonClicked()));
    connect(m_targetSelector, SIGNAL(currentIndexChanged(int,int)),
            this, SIGNAL(currentIndexChanged(int,int)));
    updateTargetSelector();
}

TargetSettingsWidget::~TargetSettingsWidget()
{
    delete ui;
}

void TargetSettingsWidget::addTarget(const QString &name)
{
    m_targetSelector->addTarget(name);
    updateTargetSelector();
}

void TargetSettingsWidget::removeTarget(int index)
{
    m_targetSelector->removeTarget(index);
}

QString TargetSettingsWidget::targetNameAt(int index) const
{
    return m_targetSelector->targetAt(index).name;
}

void TargetSettingsWidget::setCentralWidget(QWidget *widget)
{
    ui->scrollArea->setWidget(widget);
}

int TargetSettingsWidget::targetCount() const
{
    return m_targetSelector->targetCount();
}

int TargetSettingsWidget::currentIndex() const
{
    return m_targetSelector->currentIndex();
}

int TargetSettingsWidget::currentSubIndex() const
{
    return m_targetSelector->currentSubIndex();
}

void TargetSettingsWidget::updateTargetSelector()
{
    m_targetSelector->setGeometry((WIDTH-m_targetSelector->minimumSizeHint().width())/2, 12,
        m_targetSelector->minimumSizeHint().width(),
        m_targetSelector->minimumSizeHint().height());
}

void TargetSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
