#include "codeeditor.h"

#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>

// 行号区本身只负责把绘制工作交回 CodeEditor，保持 CodeEditor 对外只有一个类。
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *m_editor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_lineNumbersVisible(true)
    , m_backgroundColor(Qt::white)
{
    QFont editorFont(QStringLiteral("Consolas"), 12);
    editorFont.setStyleHint(QFont::Monospace);
    setEditorFont(editorFont);

    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    // 对话框弹出时主窗口会失去焦点，Inactive 状态的选区颜色默认会变灰，
    // 看起来像“选中的文字没有选中”，这里统一成和 Active 一致。
    QPalette pal = palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight,
                 pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText,
                 pal.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(pal);

    updateHighlightColor();
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_lineNumbersVisible)
        return 0;

    int digits = 1;
    int maxLines = qMax(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_backgroundColor.darker(106));   // 行号区底色

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    const QColor numberColor = m_backgroundColor.lightness() > 128
                                   ? QColor(120, 120, 120)
                                   : QColor(170, 170, 170);
    painter.setPen(numberColor);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

bool CodeEditor::lineNumbersVisible() const
{
    return m_lineNumbersVisible;
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (m_lineNumbersVisible == visible)
        return;

    m_lineNumbersVisible = visible;
    m_lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth(blockCount());
    viewport()->update();
}

QColor CodeEditor::editorBackgroundColor() const
{
    return m_backgroundColor;
}

void CodeEditor::setEditorBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;

    QPalette pal = palette();
    pal.setColor(QPalette::Base, color);
    pal.setColor(QPalette::Text, color.lightness() > 128 ? QColor(Qt::black) : QColor(Qt::white));
    setPalette(pal);

    updateHighlightColor();             // 当前行高亮跟着底色走
    m_lineNumberArea->update();         // 行号区底色同步
    viewport()->update();
}

QFont CodeEditor::editorFont() const
{
    return font();
}

void CodeEditor::setEditorFont(const QFont &font)
{
    setFont(font);
    // Tab 宽度跟随字体，否则换成比例字体后缩进会错位。
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);

    updateLineNumberAreaWidth(blockCount());
    viewport()->update();
    emit editorFontChanged(font);
}

void CodeEditor::applyCharFormat(const QTextCharFormat &format)
{
    mergeCurrentCharFormat(format);
}

bool CodeEditor::autoWrap() const
{
    return lineWrapMode() != QPlainTextEdit::NoWrap;
}

void CodeEditor::setAutoWrap(bool on)
{
    setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    updateLineNumberAreaWidth(blockCount());
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect area = contentsRect();
    m_lineNumberArea->setGeometry(QRect(area.left(), area.top(),
                                        lineNumberAreaWidth(), area.height()));
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::updateHighlightColor()
{
    // 底色浅时用淡黄色，底色已经偏暗时改用比底色稍亮/稍暗的颜色，
    // 保证当前行在任何底色上都看得见。
    if (m_backgroundColor.lightness() > 160)
        m_highlightColor = QColor(255, 255, 204);
    else if (m_backgroundColor.lightness() > 100)
        m_highlightColor = m_backgroundColor.darker(108);
    else
        m_highlightColor = m_backgroundColor.lighter(135);

    highlightCurrentLine();
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 有选区时不画当前行高亮：额外选区画在文本选区之上，
    // 否则选中的文字会被高亮色盖住，看起来像没有选中。
    if (!isReadOnly() && !textCursor().hasSelection()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(m_highlightColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}
