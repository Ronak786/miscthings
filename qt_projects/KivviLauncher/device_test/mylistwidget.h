#ifndef MYLISTWIDGET_H
#define MYLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QKeyEvent>

class MyListWidget : public QListWidget
{
public:
    MyListWidget();
    void keyPressEvent(QKeyEvent *event);
};

#endif // MYLISTWIDGET_H
