#include <QtTest>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

#include "codeeditor.h"
#include "fileio.h"
#include "mainwindow.h"

// 针对实验要求里点名的功能与边界做自动化测试：
//   * 编码识别（UTF-8 / UTF-8 BOM / GBK / 二进制）
//   * 保存的原子性与只读文件失败处理
//   * 查找循环、区分大小写、替换全部的一次撤销
//   * 保存后修改标记、Action 使能状态、打开异常文件不破坏当前内容
class TestEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // FileIo
    void utf8Detection();
    void utf8BomDetection();
    void gbkDetection();
    void binaryFileRejected();
    void emptyFileIsUtf8();
    void canEncodeGbk();
    void gbkRoundTrip();
    void utf8RoundTrip();
    void writeFailureKeepsOriginal();

    // 界面功能
    void openFileUpdatesTitleAndEncoding();
    void lineNumberWidthGrowsWithDigits();
    void findWrapsAround();
    void findIsCaseSensitive();
    void replaceAllIsSingleUndoStep();
    void replaceOnceFollowsSelection();
    void saveClearsModifiedFlag();
    void editActionsFollowState();
    void openMissingFileKeepsContent();

private:
    QString path(const QString &name) const { return m_dir.filePath(name); }
    static void closeModalsSoon();

    QTemporaryDir m_dir;
};

void TestEditor::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

void TestEditor::cleanupTestCase()
{
}

void TestEditor::closeModalsSoon()
{
    // 找不到文件、未找到文本等场景会弹模态框。offscreen 平台下窗口不会
    // 真正激活，activeModalWidget() 可能取不到，所以按顶层窗口逐个找。
    QTimer::singleShot(80, [] {
        const QWidgetList widgets = QApplication::topLevelWidgets();
        for (QWidget *widget : widgets) {
            if (widget->isVisible() && qobject_cast<QMessageBox *>(widget))
                widget->close();
        }
    });
}

void TestEditor::utf8Detection()
{
    const QString file = path(QStringLiteral("utf8.txt"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QString::fromUtf8("中文内容 abc\n第二行").toUtf8());
    f.close();

    QString text;
    QString encoding;
    QString error;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QCOMPARE(encoding, QStringLiteral("UTF-8"));
    QCOMPARE(text, QString::fromUtf8("中文内容 abc\n第二行"));
}

void TestEditor::utf8BomDetection()
{
    const QString file = path(QStringLiteral("utf8_bom.txt"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray("\xEF\xBB\xBF") + QString::fromUtf8("带 BOM 的中文").toUtf8());
    f.close();

    QString text;
    QString encoding;
    QString error;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QCOMPARE(encoding, QStringLiteral("UTF-8 BOM"));
    QCOMPARE(text, QString::fromUtf8("带 BOM 的中文"));
}

void TestEditor::gbkDetection()
{
    const QString file = path(QStringLiteral("gbk.txt"));
    QString error;
    QVERIFY2(FileIo::writeTextFile(file, QString::fromUtf8("GBK 编码的中文测试"),
                                   QStringLiteral("GBK"), &error), qPrintable(error));

    QFile raw(file);
    QVERIFY(raw.open(QIODevice::ReadOnly));
    const QByteArray bytes = raw.readAll();
    QVERIFY(!bytes.startsWith(QByteArray("\xE4")));      // 不是 UTF-8 的中文首字节

    QString text;
    QString encoding;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QCOMPARE(encoding, QStringLiteral("GBK"));
    QCOMPARE(text, QString::fromUtf8("GBK 编码的中文测试"));
}

void TestEditor::binaryFileRejected()
{
    const QString file = path(QStringLiteral("fake.dat"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(64, '\0'));
    f.write("PNG");
    f.close();

    QString text;
    QString encoding;
    QString error;
    QVERIFY(!FileIo::readTextFile(file, &text, &encoding, &error));
    QVERIFY(error.contains(QString::fromUtf8("二进制")));
}

void TestEditor::emptyFileIsUtf8()
{
    const QString file = path(QStringLiteral("empty.txt"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();

    QString text = QStringLiteral("旧内容");
    QString encoding;
    QString error;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QVERIFY(text.isEmpty());
    QCOMPARE(encoding, QStringLiteral("UTF-8"));
}

void TestEditor::canEncodeGbk()
{
    QVERIFY(FileIo::canEncodeText(QString::fromUtf8("中文可以"), QStringLiteral("GBK")));
    QVERIFY(!FileIo::canEncodeText(QString::fromUtf8("\xF0\x9F\x98\x80"), QStringLiteral("GBK")));
    QVERIFY(FileIo::canEncodeText(QString::fromUtf8("\xF0\x9F\x98\x80"), QStringLiteral("UTF-8")));
}

void TestEditor::gbkRoundTrip()
{
    const QString file = path(QStringLiteral("roundtrip_gbk.txt"));
    const QString content = QString::fromUtf8("第一行中文\n第二行：全角，标点！");

    QString error;
    QVERIFY2(FileIo::writeTextFile(file, content, QStringLiteral("GBK"), &error),
             qPrintable(error));

    QString text;
    QString encoding;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QCOMPARE(encoding, QStringLiteral("GBK"));
    QCOMPARE(text, content);
}

void TestEditor::utf8RoundTrip()
{
    const QString file = path(QStringLiteral("roundtrip_utf8.txt"));
    const QString content = QString::fromUtf8("UTF-8 往返：中文、符号 ℃、emoji \xF0\x9F\x98\x80");

    QString error;
    QVERIFY2(FileIo::writeTextFile(file, content, QStringLiteral("UTF-8"), &error),
             qPrintable(error));

    QString text;
    QString encoding;
    QVERIFY2(FileIo::readTextFile(file, &text, &encoding, &error), qPrintable(error));
    QCOMPARE(encoding, QStringLiteral("UTF-8"));
    QCOMPARE(text, content);
}

void TestEditor::writeFailureKeepsOriginal()
{
    const QString file = path(QStringLiteral("readonly.txt"));
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QString::fromUtf8("原始内容").toUtf8());
    f.close();

    QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::ReadUser));

    QString error;
    const bool ok = FileIo::writeTextFile(file, QString::fromUtf8("新内容"),
                                          QStringLiteral("UTF-8"), &error);
    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());

    QFile check(file);
    QVERIFY(check.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(check.readAll()), QString::fromUtf8("原始内容"));

    QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::ReadUser
                                   | QFileDevice::WriteOwner | QFileDevice::WriteUser);
}

void TestEditor::openFileUpdatesTitleAndEncoding()
{
    const QString file = path(QStringLiteral("title.txt"));
    QString error;
    QVERIFY(FileIo::writeTextFile(file, QString::fromUtf8("标题测试"), QStringLiteral("UTF-8"),
                                  &error));

    MainWindow window;
    QVERIFY(window.openPath(file));
    QVERIFY(window.windowTitle().contains(QStringLiteral("title.txt")));
    QCOMPARE(window.currentEncoding(), QStringLiteral("UTF-8"));
    QCOMPARE(window.editor()->toPlainText(), QString::fromUtf8("标题测试"));
}

void TestEditor::lineNumberWidthGrowsWithDigits()
{
    MainWindow window;
    CodeEditor *editor = window.editor();

    editor->setPlainText(QStringLiteral("1"));
    const int widthOneDigit = editor->lineNumberAreaWidth();

    QStringList lines;
    for (int i = 0; i < 120; ++i)
        lines << QStringLiteral("line %1").arg(i);
    editor->setPlainText(lines.join(QLatin1Char('\n')));
    const int widthThreeDigits = editor->lineNumberAreaWidth();

    QVERIFY(widthThreeDigits > widthOneDigit);

    editor->setLineNumbersVisible(false);
    QCOMPARE(editor->lineNumberAreaWidth(), 0);
}

void TestEditor::findWrapsAround()
{
    MainWindow window;
    CodeEditor *editor = window.editor();
    editor->setPlainText(QStringLiteral("hello one\nhello two\nhello three"));

    QTextCursor cursor(editor->document());
    editor->setTextCursor(cursor);

    QVERIFY(window.findText(QStringLiteral("hello"), true, false));
    QCOMPARE(editor->textCursor().blockNumber(), 0);
    QVERIFY(window.findText(QStringLiteral("hello"), true, false));
    QCOMPARE(editor->textCursor().blockNumber(), 1);
    QVERIFY(window.findText(QStringLiteral("hello"), true, false));
    QCOMPARE(editor->textCursor().blockNumber(), 2);
    QVERIFY(window.findText(QStringLiteral("hello"), true, false));   // 循环回第一处
    QCOMPARE(editor->textCursor().blockNumber(), 0);
}

void TestEditor::findIsCaseSensitive()
{
    MainWindow window;
    CodeEditor *editor = window.editor();
    editor->setPlainText(QStringLiteral("Hello hello"));

    QTextCursor cursor(editor->document());
    editor->setTextCursor(cursor);
    QVERIFY(window.findText(QStringLiteral("hello"), true, false));
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("hello"));
    QCOMPARE(editor->textCursor().selectionStart(), 6);

    editor->setTextCursor(cursor);
    QVERIFY(window.findText(QStringLiteral("hello"), false, false));
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("Hello"));
}

void TestEditor::replaceAllIsSingleUndoStep()
{
    MainWindow window;
    CodeEditor *editor = window.editor();
    editor->setPlainText(QStringLiteral("hello hello hello"));

    QCOMPARE(window.replaceAll(QStringLiteral("hello"), QStringLiteral("HI"), true), 3);
    QCOMPARE(editor->toPlainText(), QStringLiteral("HI HI HI"));

    QVERIFY(editor->document()->isUndoAvailable());
    editor->undo();
    QCOMPARE(editor->toPlainText(), QStringLiteral("hello hello hello"));
}

void TestEditor::replaceOnceFollowsSelection()
{
    MainWindow window;
    CodeEditor *editor = window.editor();
    editor->setPlainText(QStringLiteral("hello world"));

    QTextCursor cursor(editor->document());
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);      // 选中第一个 hello
    editor->setTextCursor(cursor);

    QVERIFY(window.replaceOnce(QStringLiteral("hello"), QStringLiteral("HI"), true, false));
    QCOMPARE(editor->toPlainText(), QStringLiteral("HI world"));
}

void TestEditor::saveClearsModifiedFlag()
{
    const QString file = path(QStringLiteral("save.txt"));
    QString error;
    QVERIFY(FileIo::writeTextFile(file, QStringLiteral("abc"), QStringLiteral("UTF-8"), &error));

    MainWindow window;
    QVERIFY(window.openPath(file));
    QVERIFY(!window.editor()->document()->isModified());

    window.editor()->insertPlainText(QStringLiteral("def"));
    QVERIFY(window.editor()->document()->isModified());
    QVERIFY(window.windowTitle().startsWith(QStringLiteral("* ")));

    QVERIFY(window.saveFile());
    QVERIFY(!window.editor()->document()->isModified());
    QVERIFY(!window.windowTitle().startsWith(QStringLiteral("* ")));
}

void TestEditor::editActionsFollowState()
{
    QApplication::clipboard()->setText(QStringLiteral("clipboard"));

    MainWindow window;
    window.editor()->setPlainText(QStringLiteral("abcdef"));
    QTextCursor cursor(window.editor()->document());
    window.editor()->setTextCursor(cursor);

    QAction *cut = window.findChild<QAction *>(QStringLiteral("actionCut"));
    QAction *copy = window.findChild<QAction *>(QStringLiteral("actionCopy"));
    QAction *paste = window.findChild<QAction *>(QStringLiteral("actionPaste"));
    QVERIFY(cut && copy && paste);

    QVERIFY(!cut->isEnabled());
    QVERIFY(!copy->isEnabled());
    QVERIFY(paste->isEnabled());          // 剪贴板里有文本

    window.editor()->selectAll();
    QVERIFY(cut->isEnabled());
    QVERIFY(copy->isEnabled());
}

void TestEditor::openMissingFileKeepsContent()
{
    const QString file = path(QStringLiteral("keep.txt"));
    QString error;
    QVERIFY(FileIo::writeTextFile(file, QString::fromUtf8("不要被清空"), QStringLiteral("UTF-8"),
                                  &error));

    MainWindow window;
    QVERIFY(window.openPath(file));

    closeModalsSoon();
    QVERIFY(!window.openPath(path(QStringLiteral("no_such_file.txt"))));
    QCOMPARE(window.editor()->toPlainText(), QString::fromUtf8("不要被清空"));
    QVERIFY(window.currentFilePath().endsWith(QStringLiteral("keep.txt")));
}

QTEST_MAIN(TestEditor)
#include "tst_editor.moc"
