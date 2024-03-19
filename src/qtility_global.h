#ifndef QTILITY_GLOBAL_H
#define QTILITY_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QOPENREMOTE_LIBRARY)
#define QOPENREMOTE_EXPORT Q_DECL_EXPORT
#else
#define QOPENREMOTE_EXPORT Q_DECL_IMPORT
#endif

#endif
