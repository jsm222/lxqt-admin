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
#ifdef Q_OS_FREEBSD
void TimeDateCtl::execPkProcess(QStringList &args)
{
    QProcess process;
    QStringList defaultArgs;
    defaultArgs << QStringLiteral("--disable-internal-agent");
    defaultArgs << QStringLiteral("/usr/local/bin/lxqt-admin-time-helper");
    defaultArgs << args;
    process.start("pkexec",defaultArgs);
    process.waitForFinished(-1);
}

    QString TimeDateCtl::execProcess(QString program,QStringList &args) const {
    QProcess process;
    process.start(program,args);
    process.waitForFinished(-1);
    return process.readAllStandardOutput();
}
#endif
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
    QStringList args;
    args << QStringLiteral("SetTimezone");
    args     << timeZone;
    execPkProcess(args);
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
    QStringList args;
    args << QStringLiteral("SetTime");
    args << dateTime.toString("yyyyMMddhhmm.ss");
    execPkProcess(args);
    #endif
    return true;
}

bool TimeDateCtl::useNtp() const
{
    #ifdef Q_OS_LINUX
    return mIface->property("NTP").toBool();
    #elif defined(Q_OS_FREEBSD)
    QStringList args;
    args << QStringLiteral("ntpd_enable");
    QString out = TimeDateCtl::execProcess(QStringLiteral("/usr/sbin/sysrc"),args);
    out.chop(1);
    return out.endsWith("YES",Qt::CaseInsensitive);
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
    QStringList args;
    args << QStringLiteral("SetNTP");
    args << ((value) ? "True" : "False");
    execPkProcess(args);

#endif
    return true;

}


bool TimeDateCtl::localRtc() const
{
    #ifdef Q_OS_LINUX
    return mIface->property("LocalRTC").toBool();
    #endif
    #ifdef Q_OS_FREEBSD
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
    #endif
    #ifdef Q_OS_FREEBSD
    QStringList args;
    args << QStringLiteral("SetLocalRTC");
    args << ((value) ? "True" : "False");
    execPkProcess(args);

    if(!value) {
    QMessageBox * msg = new QMessageBox{QMessageBox::Information, QObject::tr("lxqt-admin-user")
        , QObject::tr("Change to RTC in UTC requires a reboot")};
    msg->setAttribute(Qt::WA_DeleteOnClose, true);
    msg->show();
    }


    #endif
    return true;
}
