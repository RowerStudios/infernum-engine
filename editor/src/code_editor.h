#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include <QPlainTextEdit>
#include <QString>

class code_editor : public QPlainTextEdit {
  Q_OBJECT
public:
  explicit code_editor(QWidget *parent = nullptr);

  bool load_file(const QString &path);
  bool save_file();
  QString file_path() const { return d_file_path; }

  void line_number_area_paint_event(QPaintEvent *event);
  int line_number_area_width() const;

signals:
  void modification_changed(bool modified);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void update_line_number_area_width(int new_block_count);
  void update_line_number_area(const QRect &rect, int dy);
  void highlight_current_line();

private:
  QWidget *d_line_number_area = nullptr;
  QString d_file_path;
};

#endif
