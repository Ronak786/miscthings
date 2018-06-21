#ifndef DEBUG_H
#define DEBUG_H

#define _DEBUG
#ifdef _DEBUG
#define pr_info qDebug
#else
#define pr_info(...)
#endif

#include <QDebug>

#endif // DEBUG_H
