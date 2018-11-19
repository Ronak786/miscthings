#include "widgetstatusbar.h"
#include "ui_widgetstatusbar.h"
#include <QHBoxLayout>
#include <QLabel>

WidgetStatusBar::WidgetStatusBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetStatusBar)
{
    ui->setupUi(this);

    QLabel *labelMobileSignal = new QLabel();
    QLabel *labelSystemTime = new QLabel();
    QLabel *labelBattery = new QLabel();

    QHBoxLayout *hBoxLayout = new QHBoxLayout();
    hBoxLayout->addWidget(labelMobileSignal);
    hBoxLayout->addWidget(labelSystemTime);
    hBoxLayout->addWidget(labelBattery);

    this->setLayout(hBoxLayout);
}

WidgetStatusBar::~WidgetStatusBar()
{
    delete ui;
}
