#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QColor>
#include <QFont>
#include <QPlainTextEdit>

class QPaintEvent;
class QResizeEvent;

// 带行号区的编辑器。
//
// 继承 QPlainTextEdit，在左侧留出一条边距，用一个子控件 LineNumberArea
// 在上面画行号；行号宽度随总行数的位数动态变化，当前行用 ExtraSelection
// 画整行底色。编辑器底色、字体都可以在运行时修改。
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    bool lineNumbersVisible() const;
    void setLineNumbersVisible(bool visible);

    QColor editorBackgroundColor() const;
    void setEditorBackgroundColor(const QColor &color);

    QFont editorFont() const;
    void setEditorFont(const QFont &font);

    // 有选区时作用于选区，没有选区时作用于之后输入的文字。
    void applyCharFormat(const QTextCharFormat &format);

    bool autoWrap() const;
    void setAutoWrap(bool on);

signals:
    void editorFontChanged(const QFont &font);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount = 0);
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateHighlightColor();
    void highlightCurrentLine();

private:
    QWidget *m_lineNumberArea;
    bool m_lineNumbersVisible;
    QColor m_backgroundColor;
    QColor m_highlightColor;
};

#endif // CODEEDITOR_H
