#ifndef PROPERTY_EDITOR_H
#define PROPERTY_EDITOR_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QWidget>

class QLineEdit;
class QTreeWidget;

class property_editor : public QWidget {
  Q_OBJECT
public:
  explicit property_editor(QWidget *parent = nullptr);
  void set_properties(const QMap<QString, QVariant> &values);
  void update_value(const QString &key, const QVariant &value);
  QVariant property_value(const QString &key) const;
  QMap<QString, QVariant> properties() const;

signals:
  void property_changed(const QString &key, const QVariant &value);

private slots:
  void filter_rows(const QString &text);

private:
  QWidget *create_editor(const QString &key, const QVariant &value);
  void add_property_row(const QString &category, const QString &name, const QString &key, const QVariant &value);
  QLineEdit *d_search = nullptr;
  QTreeWidget *d_tree = nullptr;
  QMap<QString, QVariant> d_values;
};

#endif
