#ifndef PKGLIB_GLOBAL_H
#define PKGLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PKGLIB_LIBRARY)
#  define PKGLIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PKGLIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // PKGLIB_GLOBAL_H
