#include "mylistwidget.h"

MyListWidget::MyListWidget()
{

}

void MyListWidget::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
        QWidget::keyPressEvent(event);
        break;
    default:
        QListWidget::keyPressEvent(event);
        break;
    }
}
