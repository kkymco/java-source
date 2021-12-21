#ifndef NOTIFICATOR_H
#define NOTIFICATOR_H

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
#ifdef USE_DBUS
class QDBusInterface;
#endif
