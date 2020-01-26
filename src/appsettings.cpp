/****************************************************************************
**
** Copyright (C) 2020 Rinigus https://github.com/rinigus
**
** This file is part of Flatpak Runner.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

#include "appsettings.h"

#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QSettings>

#include <algorithm>

#define SET_GENERAL "General"
#define SET_APP "Application "
#define SETTINGS_VERSION 1

AppSettings::AppSettings(QObject *parent) : QObject(parent)
{
  bool setdefaults = false;
  QSettings settings;

  setdefaults = (setdefaults || settings.value(SET_GENERAL "/version", 0).toInt() < SETTINGS_VERSION);
  setdefaults = (setdefaults || settings.value(SET_APP + defaultApp() + "/env").toMap().size() == 0);

  if (setdefaults)
    {
      QMap<QString, QString> env;
      env["QT_QUICK_CONTROLS_STYLE"] = "Plasma";
      env["QT_QUICK_CONTROLS_MOBILE"] = "1";
      setAppEnv(defaultApp(), env);
    }

  settings.setValue(SET_GENERAL "/version", SETTINGS_VERSION);
}

int AppSettings::appDpi(QString flatpak, bool merge) const
{
  QSettings settings;
  int d = settings.value(SET_APP + flatpak + "/dpi", 0).toInt();
  if (d > 0 || !merge) return d;
  int s = appScaling(flatpak);
  if (s > 1) return (int)(defaultDpi() / ((float)s));
  d = settings.value(SET_APP + defaultApp() + "/dpi", 0).toInt();
  if (d > 0) return d;
  return defaultDpi();
}

QMap<QString, QString> AppSettings::appEnv(QString flatpak, bool merged) const
{
  QSettings settings;
  QMap<QString, QVariant> mv = settings.value(SET_APP + flatpak + "/env").toMap();
  QMap<QString, QString> m;
  for (auto i = mv.constBegin(); i != mv.constEnd(); ++i)
    m.insert(i.key(), i.value().toString());
  if (flatpak == defaultApp()) return m;
  if (merged)
    {
      QMap<QString, QString> d = appEnv(defaultApp());
      for (auto i = d.constBegin(); i != d.constEnd(); ++i)
        if (!m.contains(i.key()))
          m.insert(i.key(), i.value());
    }
  return m;
}

QString AppSettings::appEnvJson(QString flatpak, bool merged) const
{
  QMap<QString, QString> m = appEnv(flatpak, merged);
  QMap<QString, QVariant> v;
  for (auto i = m.constBegin(); i != m.constEnd(); ++i)
    v.insert(i.key(), i.value());
  return QJsonDocument::fromVariant(v).toJson();
}

QString AppSettings::appIcon(QString flatpak) const
{
  QSettings settings;
  return settings.value(SET_APP + flatpak + "/icon").toString();
}

QString AppSettings::appName(QString flatpak) const
{
  QSettings settings;
  return settings.value(SET_APP + flatpak + "/name").toString();
}

int AppSettings::appScaling(QString flatpak, bool merge) const
{
  QSettings settings;
  int s = settings.value(SET_APP + flatpak + "/scaling", 0).toInt();
  if (s > 0 || !merge) return s;
  s = settings.value(SET_APP + defaultApp() + "/scaling", 1).toInt();
  if (s < 1) s = 1;
  return s;
}

QStringList AppSettings::apps() const
{
  QSettings settings;
  QStringList arr = settings.value(SET_GENERAL "/applist").toStringList();
  std::sort(arr.begin(), arr.end(), [&](QString a, QString b){
    return appName(a) + a < appName(b) + b;
  });
  return arr;
}

QString AppSettings::defaultApp() const
{
  return "default";
}

int AppSettings::defaultDpi() const
{
  return qGuiApp->primaryScreen()->physicalDotsPerInch();
}

void AppSettings::updateApps(QString appsInJson)
{
  QJsonArray arr = QJsonDocument::fromJson(appsInJson.toUtf8()).array();
  QSettings settings;
  QStringList applist;
  for (QJsonArray::const_iterator iter=arr.constBegin(); iter!=arr.constEnd(); ++iter)
    {
      QJsonObject obj = (*iter).toObject();
      QString flatpak = obj.value("flatpak").toString();
      QString name = obj.value("name").toString();
      QString icon = obj.value("icon").toString();
      if (flatpak.isEmpty()) continue;
      applist.append(flatpak);
      settings.setValue(SET_APP + flatpak + "/name", name);
      settings.setValue(SET_APP + flatpak + "/icon", icon);
    }
  settings.setValue(SET_GENERAL "/applist", applist);

  emit appListChanged();
}

void AppSettings::rmAppEnvVar(QString flatpak, QString name)
{
  QMap<QString,QString> e = appEnv(flatpak);
  if (e.contains(name))
    {
      e.remove(name);
      setAppEnv(flatpak, e);
    }
}

void AppSettings::setAppDpi(QString flatpak, int dpi)
{
  QSettings settings;
  if (dpi < 1)
    {
      settings.remove(SET_APP + flatpak + "/dpi");
    }
  settings.setValue(SET_APP + flatpak + "/dpi", dpi);
}

void AppSettings::setAppEnv(QString flatpak, QMap<QString, QString> env)
{
  QSettings settings;
  QMap<QString, QVariant> m;
  for (auto i = env.constBegin(); i != env.constEnd(); ++i)
    m.insert(i.key(), i.value());
  settings.setValue(SET_APP + flatpak + "/env", m);
}

void AppSettings::setAppEnvVar(QString flatpak, QString name, QString value)
{
  QMap<QString,QString> e = appEnv(flatpak);
  e.insert(name, value);
  setAppEnv(flatpak, e);
}

void AppSettings::setAppScaling(QString flatpak, int scaling)
{
  QSettings settings;
  if (scaling < 1)
    {
      settings.remove(SET_APP + flatpak + "/scaling");
    }
  settings.setValue(SET_APP + flatpak + "/scaling", scaling);
}