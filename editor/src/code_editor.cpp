#include "code_editor.h"
#include <QFile>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFormat>
#include <QTextStream>
#include <QVector>

struct highlight_rule {
  QRegularExpression pattern;
  QTextCharFormat format;
};

class cpp_highlighter : public QSyntaxHighlighter {
public:
  explicit cpp_highlighter(QTextDocument *document) : QSyntaxHighlighter(document) {
    QTextCharFormat keyword_format;
    keyword_format.setForeground(QColor("#c586c0"));
    keyword_format.setFontWeight(QFont::Bold);
    const QStringList keywords = {
        "alignas", "alignof", "and", "asm", "break", "case", "catch", "class",
        "const", "constexpr", "const_cast", "continue", "decltype", "default",
        "delete", "do", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "for", "friend", "goto", "if", "inline", "mutable",
        "namespace", "new", "noexcept", "not", "nullptr", "operator", "or",
        "private", "protected", "public", "register", "reinterpret_cast",
        "return", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "throw", "true", "try", "typedef",
        "typeid", "typename", "union", "using", "virtual", "volatile", "while",
        "null"};
    for (const QString &keyword : keywords) {
      d_rules.append({QRegularExpression("\\b" + keyword + "\\b"), keyword_format});
    }

    QTextCharFormat type_format;
    type_format.setForeground(QColor("#4ec9b0"));
    const QStringList types = {
        "bool", "char", "double", "float", "int", "long", "short", "signed",
        "unsigned", "void", "auto", "size_t", "string", "SDL_Color",
        "SDL_Renderer", "SDL_Window", "SDL_FRect", "Uint8", "Uint64",
        "int32_t", "uint32_t"};
    for (const QString &type : types) {
      d_rules.append({QRegularExpression("\\b" + type + "\\b"), type_format});
    }

    QTextCharFormat function_format;
    function_format.setForeground(QColor("#dcdcaa"));
    d_rules.append({QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()"),
                    function_format});

    QTextCharFormat string_format;
    string_format.setForeground(QColor("#ce9178"));
    d_rules.append({QRegularExpression("\"([^\"\\\\]|\\\\.)*\""), string_format});
    d_rules.append({QRegularExpression("'([^'\\\\]|\\\\.)*'"), string_format});

    QTextCharFormat number_format;
    number_format.setForeground(QColor("#b5cea8"));
    d_rules.append({QRegularExpression("\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?[fFuUlL]*\\b"),
                    number_format});

    QTextCharFormat preprocessor_format;
    preprocessor_format.setForeground(QColor("#9cdcfe"));
    d_rules.append({QRegularExpression("^\\s*#\\s*\\w+"), preprocessor_format});

    d_comment_format.setForeground(QColor("#6a9955"));
    d_rules.append({QRegularExpression("//[^\n]*"), d_comment_format});
    d_comment_start = QRegularExpression("/\\*");
    d_comment_end = QRegularExpression("\\*/");
  }

protected:
  void highlightBlock(const QString &text) override {
    for (const highlight_rule &rule : d_rules) {
      QRegularExpressionMatchIterator iterator = rule.pattern.globalMatch(text);
      while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), rule.format);
      }
    }

    setCurrentBlockState(0);
    int start_index = 0;
    if (previousBlockState() != 1) {
      start_index = text.indexOf(d_comment_start);
    }
    while (start_index >= 0) {
      QRegularExpressionMatch end_match = d_comment_end.match(text, start_index + 2);
      int comment_length = 0;
      if (!end_match.hasMatch()) {
        setCurrentBlockState(1);
        comment_length = text.length() - start_index;
      } else {
        comment_length = end_match.capturedEnd() - start_index;
      }
      setFormat(start_index, comment_length, d_comment_format);
      start_index = text.indexOf(d_comment_start, start_index + comment_length);
    }
  }

private:
  QVector<highlight_rule> d_rules;
  QTextCharFormat d_comment_format;
  QRegularExpression d_comment_start;
  QRegularExpression d_comment_end;
};

class line_number_area : public QWidget {
public:
  explicit line_number_area(code_editor *editor) : QWidget(editor), d_editor(editor) {}

  QSize sizeHint() const override {
    return QSize(d_editor->line_number_area_width(), 0);
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    d_editor->line_number_area_paint_event(event);
  }

private:
  code_editor *d_editor;
};

code_editor::code_editor(QWidget *parent) : QPlainTextEdit(parent) {
  setObjectName("code");
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(10);
  font.setStyleHint(QFont::Monospace);
  setFont(font);
  setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
  setLineWrapMode(QPlainTextEdit::NoWrap);
  setFrameShape(QFrame::NoFrame);
  setCursorWidth(2);

  d_line_number_area = new line_number_area(this);

  connect(this, &QPlainTextEdit::blockCountChanged, this,
          &code_editor::update_line_number_area_width);
  connect(this, &QPlainTextEdit::updateRequest, this,
          &code_editor::update_line_number_area);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &code_editor::highlight_current_line);
  connect(document(), &QTextDocument::modificationChanged, this,
          &code_editor::modification_changed);

  new cpp_highlighter(document());
  update_line_number_area_width(0);
  highlight_current_line();
}

bool code_editor::load_file(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  QByteArray data = file.readAll();
  file.close();

  setPlainText(QString::fromUtf8(data));
  document()->setModified(false);
  document()->clearUndoRedoStacks();
  d_file_path = path;
  highlight_current_line();
  return true;
}

bool code_editor::save_file() {
  if (d_file_path.isEmpty()) {
    return false;
  }
  QFile file(d_file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  file.write(toPlainText().toUtf8());
  file.close();
  document()->setModified(false);
  return true;
}

int code_editor::line_number_area_width() const {
  int digits = 1;
  int max_blocks = qMax(1, blockCount());
  while (max_blocks >= 10) {
    max_blocks /= 10;
    ++digits;
  }
  int space = 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
  return space;
}

void code_editor::line_number_area_paint_event(QPaintEvent *event) {
  QPainter painter(d_line_number_area);
  painter.fillRect(event->rect(), QColor("#1b1d1f"));

  QTextBlock block = firstVisibleBlock();
  int block_number = block.blockNumber();
  int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
  int bottom = top + qRound(blockBoundingRect(block).height());

  while (block.isValid() && top <= event->rect().bottom()) {
    if (block.isVisible() && bottom >= event->rect().top()) {
      QString number = QString::number(block_number + 1);
      painter.setPen(QColor("#6f7378"));
      painter.drawText(0, top, d_line_number_area->width() - 6,
                       fontMetrics().height(), Qt::AlignRight, number);
    }
    block = block.next();
    top = bottom;
    bottom = top + qRound(blockBoundingRect(block).height());
    ++block_number;
  }
}

void code_editor::resizeEvent(QResizeEvent *event) {
  QPlainTextEdit::resizeEvent(event);
  QRect rect = contentsRect();
  d_line_number_area->setGeometry(
      QRect(rect.left(), rect.top(), line_number_area_width(), rect.height()));
}

void code_editor::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    QString line = textCursor().block().text();
    QString indent;
    for (int i = 0; i < line.size(); ++i) {
      QChar character = line.at(i);
      if (character == ' ' || character == '\t') {
        indent.append(character);
      } else {
        break;
      }
    }
    QPlainTextEdit::keyPressEvent(event);
    if (!indent.isEmpty()) {
      textCursor().insertText(indent);
    }
    return;
  }
  QPlainTextEdit::keyPressEvent(event);
}

void code_editor::update_line_number_area_width(int new_block_count) {
  Q_UNUSED(new_block_count);
  setViewportMargins(line_number_area_width(), 0, 0, 0);
}

void code_editor::update_line_number_area(const QRect &rect, int dy) {
  if (dy) {
    d_line_number_area->scroll(0, dy);
  } else {
    d_line_number_area->update(0, rect.y(), d_line_number_area->width(),
                               rect.height());
  }
  if (rect.contains(viewport()->rect())) {
    update_line_number_area_width(0);
  }
}

void code_editor::highlight_current_line() {
  QList<QTextEdit::ExtraSelection> selections;
  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor("#1f2224"));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    selections.append(selection);
  }
  setExtraSelections(selections);
}
