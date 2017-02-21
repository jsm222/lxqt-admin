/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "timedatectl.h"
#include <QtGlobal>
#include <QProcess>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QMessageBox>


TimeDateCtl::TimeDateCtl()
{
    mIface = new QDBusInterface(QStringLiteral("org.freedesktop.timedate1"),
                                QStringLiteral("/org/freedesktop/timedate1"),
                                QStringLiteral("org.freedesktop.timedate1"),
                                QDBusConnection::systemBus());
}

TimeDateCtl::~TimeDateCtl()
{
    delete mIface;
}


QString TimeDateCtl::timeZone() const
{
#ifdef Q_OS_LINUX
    return mIface->property("Timezone").toString();
#elif defined(Q_OS_FREEBSD)
    QFile tzFile("/var/db/zoneinfo");
    if (!tzFile.open(QFile::ReadOnly | QFile::Text)) return "Unknown";
    QTextStream in(&tzFile);
    QString lastTZone = in.readLine();
    return lastTZone;
    #endif
}

bool TimeDateCtl::setTimeZone(QString timeZone, QString& errorMessage)
{
#ifdef Q_OS_LINUX
    mIface->call("SetTimezone", timeZone, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
    return true;
#elif defined(Q_OS_FREEBSD)
        FILE		*f;
        char		path_zoneinfo_file[MAXPATHLEN];

        if ((size_t)snprintf(path_zoneinfo_file, sizeof(path_zoneinfo_file),
            "%s/%s",  _PATH_ZONEINFO, timeZone.toStdString().c_str()) >= sizeof(path_zoneinfo_file)) {
            errorMessage =  QObject::tr("%s/%s name too long").arg(_PATH_ZONEINFO).arg(timeZone);
            return false;
        }
    if (access(path_zoneinfo_file, R_OK) != 0) {
               errorMessage =  QObject::tr("Error: Cannot access %1 %2").arg(path_zoneinfo_file).arg(strerror(errno));
               return false;
            }
            if (unlink(_PATH_LOCALTIME) < 0 && errno != ENOENT) {
                errorMessage =  QObject::tr("Error: Could not unlink %1 %2").arg(_PATH_LOCALTIME).arg(strerror(errno));
                return false;
            }
            if (symlink(path_zoneinfo_file, _PATH_LOCALTIME) < 0) {
                errorMessage = QObject::tr("Error: Cannot create symbolic link %1 to %2: %3").arg(path_zoneinfo_file).arg(_PATH_LOCALTIME).arg(strerror(errno));
                return false;
            }
            if ((f = fopen(_PATH_DB, "w")) != NULL) {
                        fprintf(f, "%s\n", timeZone.toStdString().c_str());
                        fclose(f);

                }
    return true;
#endif
}

bool TimeDateCtl::setDateTime(QDateTime dateTime, QString& errorMessage)
{

#ifdef Q_OS_LINUX
    // the timedatectl dbus service accepts "usec" input.
    // Qt can only get "msec"  => convert to usec here.
    mIface->call("SetTime", dateTime.toMSecsSinceEpoch() * 1000, false, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
#elif defined(Q_OS_FREEBSD)
    struct timeval time;
    struct timezone tzp;
    gettimeofday(&time,&tzp);
    time.tv_sec = dateTime.toMSecsSinceEpoch()/1000;
    settimeofday(&time,&tzp);
#endif
    return true;
}

bool TimeDateCtl::useNtp() const
{
#ifdef Q_OS_LINUX
    return mIface->property("NTP").toBool();
#elif defined(Q_OS_FREEBSD)
    QProcess process;
    QStringList args = QStringList();
    args  << QStringLiteral("/etc/rc.d/ntpd") << QStringLiteral("rcvar");
    process.start(QStringLiteral("/bin/sh"),args);
    process.waitForFinished(-1);
    QString t = process.readAllStandardOutput();
    QStringList o = t.split("\n");
    for (QStringList::iterator it = o.begin();
          it != o.end(); ++it) {
         QString current = *it;
         if(!current.startsWith("#") && current.contains(QString("ntpd_enable"),Qt::CaseInsensitive) && current.contains(QString("yes"),Qt::CaseInsensitive)) {
         return true;
        }
     }
#endif
    return false;
}

bool TimeDateCtl::setUseNtp(bool value, QString& errorMessage)
{
#ifdef Q_OS_LINUX
    mIface->call("SetNTP", value, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
#elif defined(Q_OS_FREEBSD)
//TODO implement setUseNtp
#endif
    return true;

}


bool TimeDateCtl::localRtc() const
{
#ifdef Q_OS_LINUX
    return mIface->property("LocalRTC").toBool();
#elif defined(Q_OS_FREEBSD)
    return QFile("/etc/wall_cmos_clock").exists();
#endif
}

bool TimeDateCtl::setLocalRtc(bool value, QString& errorMessage)
{
#ifdef Q_OS_LINUX
    mIface->call("SetLocalRTC", value, false, true);
    QDBusError err = mIface->lastError();
    if(err.isValid())
    {
        errorMessage = err.message();
        return false;
    }
#elif defined(Q_OS_FREEBSD)
//TODO implement setLocalRTC
#endif
    return true;
}
