#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QVector>

struct ui_element {
  QString name;
  QString type;
  QMap<QString, QVariant> properties;
};

QMap<QString, QVariant> default_element_properties(const QString &type);
ui_element element_from_json(const QJsonObject &object);
QJsonObject element_to_json(const ui_element &element);
QVector<ui_element> default_elements();

#endif
