#ifndef WIDGETSTATUSBAR_H
#define WIDGETSTATUSBAR_H

#include <QWidget>

namespace Ui {
class WidgetStatusBar;
}

class WidgetStatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetStatusBar(QWidget *parent = 0);
    ~WidgetStatusBar();

private:
    Ui::WidgetStatusBar *ui;
};

#endif // WIDGETSTATUSBAR_H
