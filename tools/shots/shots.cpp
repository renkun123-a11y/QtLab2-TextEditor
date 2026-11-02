// 报告截图工具。
//
// 直接链接主工程的源码，用真实的 Menu / Action / 对话框跑一遍操作，
// 再用 QScreen::grabWindow(0) 抓取窗口区域的屏幕像素。这样截图里的
// 菜单、对话框、消息框都是程序真正显示出来的，不是拼出来的。
//
// 用法：shots.exe <工程根目录> <输出目录>
//
// 每个场景触发后稍等片刻再截图，模态对话框用 QTimer 在它的嵌套事件
// 循环里截图并关闭，避免 QMessageBox::exec() 把主流程卡住。

#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "mainwindow.h"

#include <QAction>
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QTextCursor>
#include <QThread>
#include <QTimer>
#include <QToolBar>

#include <QDebug>

namespace {

QString g_projectDir;
QString g_outputDir;
MainWindow *g_window = nullptr;

QString samplePath(const QString &name)
{
    return QDir(g_projectDir).filePath(QStringLiteral("samples/") + name);
}

void wait(int milliseconds)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < milliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(2);
    }
}

QRect screenRect()
{
    return QGuiApplication::primaryScreen()->geometry();
}

QImage grabRect(const QRect &rect)
{
    const QRect target = rect.intersected(screenRect());
    QPixmap pixmap = QGuiApplication::primaryScreen()->grabWindow(
        0, target.x(), target.y(), target.width(), target.height());
    return pixmap.toImage();
}

void saveImage(const QString &name, const QImage &image)
{
    const QString path = QDir(g_outputDir).filePath(name + QStringLiteral(".png"));
    if (!image.save(path))
        qWarning().noquote() << "保存失败" << path;
    else
        qInfo().noquote() << "截图" << name << image.size();
}

// 所有可见的顶层窗口（主窗口 + 菜单 + 对话框 + 消息框）的外接矩形
QRect visibleWindowsRect()
{
    QRect rect = g_window->frameGeometry();
    const QWidgetList widgets = QApplication::topLevelWidgets();
    for (QWidget *widget : widgets) {
        if (widget == g_window || !widget->isVisible())
            continue;
        if (widget->windowType() == Qt::ToolTip || widget->windowType() == Qt::Popup)
            continue;
        rect = rect.united(widget->frameGeometry());
    }
    // 弹出的菜单也算 Popup，需要单独合并
    for (QWidget *widget : widgets) {
        if (widget == g_window || !widget->isVisible())
            continue;
        if (widget->inherits("QMenu"))
            rect = rect.united(widget->frameGeometry());
    }
    return rect;
}

void captureAll(const QString &name)
{
    wait(120);
    saveImage(name, grabRect(visibleWindowsRect()));
}

void captureWindow(const QString &name)
{
    wait(120);
    saveImage(name, grabRect(g_window->frameGeometry()));
}

QAction *action(const QString &name)
{
    QAction *act = g_window->findChild<QAction *>(name);
    if (!act)
        qWarning().noquote() << "找不到 Action" << name;
    return act;
}

void triggerAction(const QString &name)
{
    if (QAction *act = action(name))
        act->trigger();
}

QMenu *menuByName(const QString &name)
{
    QMenu *menu = g_window->menuBar()->findChild<QMenu *>(name);
    if (!menu)
        qWarning().noquote() << "找不到菜单" << name;
    return menu;
}

void popupMenu(QMenu *menu)
{
    const QRect actionRect = g_window->menuBar()->actionGeometry(menu->menuAction());
    const QPoint pos = g_window->menuBar()->mapToGlobal(actionRect.bottomLeft()) + QPoint(0, 2);
    menu->popup(pos);
    wait(600);      // 等菜单的淡入动画结束，否则截出来是半透明的
}

void closeMenu(QMenu *menu)
{
    if (menu && menu->isVisible())
        menu->close();
    wait(150);
}

// 菜单局部截图（包含菜单栏），用于“文件菜单”这类图
void captureMenuArea(const QString &name, QMenu *menu)
{
    const QRect barRect(g_window->mapToGlobal(QPoint(0, 0)),
                        QSize(g_window->width(), g_window->menuBar()->height() + 2));
    const QRect rect = barRect.united(menu->frameGeometry()).adjusted(-1, -1, 8, 8);
    saveImage(name, grabRect(rect));
}

// 关闭模态框：询问框点“取消”，其它对话框直接关掉
void dismissModal(QWidget *modal)
{
    if (auto *box = qobject_cast<QMessageBox *>(modal)) {
        const QList<QAbstractButton *> buttons = box->buttons();
        for (QAbstractButton *button : buttons) {
            if (box->buttonRole(button) == QMessageBox::RejectRole) {
                button->click();
                return;
            }
        }
        box->reject();
        return;
    }
    modal->close();
}

// 在模态对话框自己的事件循环里截图，然后把它关掉
void scheduleModalCapture(const QString &name, int delay = 550)
{
    QTimer::singleShot(delay, [name] {
        QWidget *modal = QApplication::activeModalWidget();
        QWidget *popup = QApplication::activePopupWidget();
        QRect rect = visibleWindowsRect();
        if (modal)
            rect = rect.united(modal->frameGeometry());
        if (popup)
            rect = rect.united(popup->frameGeometry());
        saveImage(name, grabRect(rect));

        if (modal)
            dismissModal(modal);
        else if (popup)
            popup->close();
    });
}

void clickButton(QWidget *parent, const QString &textPart)
{
    const QList<QPushButton *> buttons = parent->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->text().contains(textPart)) {
            button->click();
            return;
        }
    }
    qWarning().noquote() << "找不到按钮" << textPart;
}

void setCheckBox(QWidget *parent, bool checked)
{
    const QList<QCheckBox *> boxes = parent->findChildren<QCheckBox *>();
    if (!boxes.isEmpty())
        boxes.first()->setChecked(checked);
}

FindDialog *findDialog()
{
    const QList<FindDialog *> dialogs = g_window->findChildren<FindDialog *>();
    for (FindDialog *dialog : dialogs) {
        if (dialog->isVisible())
            return dialog;
    }
    return nullptr;
}

CodeEditor *editor()
{
    return g_window->editor();
}

void openSample(const QString &name)
{
    g_window->openPath(samplePath(name));
    wait(250);
}

void openSampleClean(const QString &name)
{
    // 前一张图可能留下了修改标记，这里先清掉，避免打开文件时又弹保存询问。
    editor()->document()->setModified(false);
    g_window->openPath(samplePath(name));
    wait(250);
    g_window->statusBar()->clearMessage();
    editor()->moveCursor(QTextCursor::Start);
    wait(150);
}

// 把对话框摆在父窗口内部偏下的位置：既能露出后面的正文，又不会超出
// 窗口边界，截图时不会带进桌面背景。
void moveDialogDown(QWidget *dialog, int offset = 120)
{
    const QRect frame = g_window->frameGeometry();
    int y = frame.center().y() + offset;
    if (y + dialog->frameGeometry().height() > frame.bottom() - 12)
        y = frame.bottom() - dialog->frameGeometry().height() - 12;
    if (y < frame.top() + 12)
        y = frame.top() + 12;

    const QPoint pos(frame.center().x() - dialog->frameGeometry().width() / 2, y);
    dialog->move(pos);
    wait(120);
}

void selectOccurrence(const QString &needle, int occurrence)
{
    QTextCursor cursor(editor()->document());
    for (int i = 0; i <= occurrence; ++i) {
        cursor = editor()->document()->find(needle, cursor);
        if (cursor.isNull())
            return;
    }
    editor()->setTextCursor(cursor);
}

// ---------------------------------------------------------------------------

void shot01()
{
    openSample(QStringLiteral("sample_utf8.txt"));
    editor()->moveCursor(QTextCursor::Start);
    g_window->statusBar()->clearMessage();
    wait(200);
    captureWindow(QStringLiteral("01-main-window"));
}

void shot02()
{
    QMenu *menu = menuByName(QStringLiteral("menuFile"));
    popupMenu(menu);
    captureMenuArea(QStringLiteral("02-file-menu"), menu);
    closeMenu(menu);
}

void shot03()
{
    scheduleModalCapture(QStringLiteral("03-about-dialog"));
    triggerAction(QStringLiteral("actionAbout"));
    wait(300);
}

void shot04()
{
    triggerAction(QStringLiteral("actionFind"));
    wait(250);
    FindDialog *dialog = findDialog();
    if (!dialog)
        return;

    dialog->setFindText(QStringLiteral("hello"));
    setCheckBox(dialog, true);
    moveDialogDown(dialog, 110);
    clickButton(dialog, QStringLiteral("查找下一个"));   // 第一个 hello
    wait(150);
    clickButton(dialog, QStringLiteral("查找下一个"));   // 第二个 hello（第 8 行）
    wait(250);
    captureAll(QStringLiteral("04-find-dialog"));
    dialog->close();
    wait(150);
}

void shot05()
{
    triggerAction(QStringLiteral("actionReplace"));
    wait(250);
    FindDialog *dialog = findDialog();
    if (!dialog)
        return;

    dialog->setFindText(QStringLiteral("hello"));
    dialog->setReplaceText(QStringLiteral("HI"));
    moveDialogDown(dialog, 110);
    wait(200);
    captureAll(QStringLiteral("05-replace-dialog"));
    dialog->close();
    wait(150);
}

void shot06()
{
    const QString demo = QDir(g_projectDir).filePath(QStringLiteral("samples/demo.txt"));
    QFile::remove(demo);
    QFile::copy(samplePath(QStringLiteral("sample_utf8.txt")), demo);

    g_window->openPath(demo);
    wait(250);

    QTextCursor cursor = editor()->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor()->setTextCursor(cursor);
    editor()->insertPlainText(QStringLiteral("\n新增一行：保存之后标题前的 * 号会消失。"));
    wait(200);

    g_window->saveFile();
    wait(250);
    captureWindow(QStringLiteral("06-saved-title"));
}

void shot07()
{
    QTextCursor cursor = editor()->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor()->setTextCursor(cursor);
    editor()->insertPlainText(QStringLiteral("编辑之后，标题前面出现 * 号。\n"));
    wait(200);

    QMenu *menu = menuByName(QStringLiteral("menuEdit"));
    popupMenu(menu);
    captureAll(QStringLiteral("07-edited-title"));
    closeMenu(menu);
}

void shot08()
{
    editor()->selectAll();
    wait(200);

    QMenu *menu = menuByName(QStringLiteral("menuEdit"));
    popupMenu(menu);
    captureAll(QStringLiteral("08-select-all"));
    closeMenu(menu);
}

void shot09()
{
    // 选中第 8 行复制，然后到文末粘贴，状态栏会给出粘贴提示
    selectOccurrence(QStringLiteral("hello again, and hello once more."), 0);
    triggerAction(QStringLiteral("actionCopy"));
    wait(150);

    QTextCursor cursor(editor()->document());
    cursor.movePosition(QTextCursor::End);
    editor()->setTextCursor(cursor);
    editor()->insertPlainText(QStringLiteral("\n"));    // 到下一行再粘贴
    triggerAction(QStringLiteral("actionPaste"));
    wait(250);
    captureWindow(QStringLiteral("09-paste"));
}

void shot10()
{
    openSampleClean(QStringLiteral("sample_utf8.txt"));
    scheduleModalCapture(QStringLiteral("10-font-dialog"));
    triggerAction(QStringLiteral("actionFont"));
    wait(300);
}

void shot11()
{
    openSampleClean(QStringLiteral("sample_utf8.txt"));
    editor()->setEditorBackgroundColor(QColor(QStringLiteral("#fdf6e3")));
    editor()->moveCursor(QTextCursor::Down);
    editor()->moveCursor(QTextCursor::EndOfLine);
    wait(300);
    captureWindow(QStringLiteral("11-editor-background"));
    editor()->setEditorBackgroundColor(QColor(Qt::white));
}

void shot12()
{
    triggerAction(QStringLiteral("actionToggleToolBar"));    // 取消勾选 -> 隐藏
    wait(300);
    captureWindow(QStringLiteral("12-toolbar-hidden"));
    triggerAction(QStringLiteral("actionToggleToolBar"));    // 恢复
    wait(150);
}

void shot13()
{
    triggerAction(QStringLiteral("actionToggleStatusBar"));
    wait(300);
    captureWindow(QStringLiteral("13-statusbar-hidden"));
    triggerAction(QStringLiteral("actionToggleStatusBar"));
    wait(150);
}

void shot14()
{
    openSample(QStringLiteral("sample_utf8.txt"));
    QTextCursor cursor(editor()->document());
    editor()->setTextCursor(cursor);
    wait(200);

    QMenu *menu = menuByName(QStringLiteral("menuEdit"));
    popupMenu(menu);
    captureAll(QStringLiteral("14-edit-menu-disabled"));
    closeMenu(menu);
}

void shot15()
{
    openSample(QStringLiteral("sample_utf8.txt"));
    editor()->moveCursor(QTextCursor::Start);
    wait(150);

    g_window->findText(QStringLiteral("hello"), true, false);   // 第 1 个
    wait(120);
    g_window->findText(QStringLiteral("hello"), true, false);   // 第 2 个
    wait(120);
    g_window->findText(QStringLiteral("hello"), true, false);   // 第 3 个
    wait(120);
    g_window->findText(QStringLiteral("hello"), true, false);   // 循环回到开头
    wait(250);
    captureWindow(QStringLiteral("15-find-wrap"));
}

void shot16()
{
    triggerAction(QStringLiteral("actionFind"));
    wait(250);
    FindDialog *dialog = findDialog();
    if (!dialog)
        return;

    g_window->statusBar()->clearMessage();
    dialog->setFindText(QStringLiteral("zzz"));
    moveDialogDown(dialog, 110);
    wait(200);

    scheduleModalCapture(QStringLiteral("16-find-not-found"), 450);
    clickButton(dialog, QStringLiteral("查找下一个"));
    wait(250);

    dialog->close();
    wait(150);
}

void shot17()
{
    openSample(QStringLiteral("sample_utf8.txt"));
    triggerAction(QStringLiteral("actionReplace"));
    wait(250);
    FindDialog *dialog = findDialog();
    if (!dialog)
        return;

    dialog->setFindText(QStringLiteral("hello"));
    dialog->setReplaceText(QStringLiteral("HI"));
    moveDialogDown(dialog, 130);
    wait(200);

    scheduleModalCapture(QStringLiteral("17-replace-all"), 450);
    clickButton(dialog, QStringLiteral("全部替换"));
    wait(300);

    dialog->close();
    wait(100);
}

void shot18()
{
    // 上一步“全部替换”之后按一次撤销，3 处替换应被整体撤销
    editor()->undo();
    wait(350);
    captureWindow(QStringLiteral("18-replace-undo"));
}

void shot19()
{
    openSample(QStringLiteral("long_line.txt"));
    editor()->moveCursor(QTextCursor::End);
    wait(300);
    captureWindow(QStringLiteral("19-line-numbers-wrap"));
}

void shot20()
{
    triggerAction(QStringLiteral("actionToggleLineNumbers"));   // 关闭行号
    wait(300);
    captureWindow(QStringLiteral("20-line-numbers-off"));
    triggerAction(QStringLiteral("actionToggleLineNumbers"));   // 恢复
    wait(150);
}

void shot21()
{
    openSample(QStringLiteral("sample_utf8.txt"));
    QTextCursor cursor = editor()->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor()->setTextCursor(cursor);
    editor()->insertPlainText(QStringLiteral("修改一下，然后直接关窗口。\n"));
    wait(200);

    scheduleModalCapture(QStringLiteral("21-close-prompt"));
    g_window->close();      // closeEvent -> maybeSave() -> 询问框
    wait(250);
    if (!g_window->isVisible())
        g_window->show();
}

void shot22()
{
    // 上一张图是“取消”关闭，文档仍处于已修改状态；这里只为了继续测
    // 打开文件的异常分支，直接把修改标记清掉，避免又弹出保存询问。
    editor()->document()->setModified(false);
    g_window->statusBar()->clearMessage();

    scheduleModalCapture(QStringLiteral("22-open-missing"));
    g_window->openPath(QStringLiteral("D:/nope/missing_file.txt"));
    wait(250);
}

void shot23()
{
    scheduleModalCapture(QStringLiteral("23-open-binary"));
    g_window->openPath(samplePath(QStringLiteral("fake_binary.dat")));
    wait(250);
}

void shot24()
{
    openSample(QStringLiteral("sample_gbk.txt"));
    wait(250);
    captureWindow(QStringLiteral("24-open-gbk"));
}

void shot25()
{
    const QString readonlyFile = samplePath(QStringLiteral("readonly.txt"));
    QFile::setPermissions(readonlyFile, QFileDevice::ReadOwner | QFileDevice::ReadUser);

    g_window->openPath(readonlyFile);
    wait(250);
    editor()->moveCursor(QTextCursor::End);
    editor()->insertPlainText(QStringLiteral("\n再补一行，然后按 Ctrl+S。"));
    wait(200);

    scheduleModalCapture(QStringLiteral("25-save-failed"));
    g_window->saveFile();
    wait(300);

    QFile::setPermissions(readonlyFile, QFileDevice::ReadOwner | QFileDevice::ReadUser
                                            | QFileDevice::WriteOwner | QFileDevice::WriteUser);
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("小牛文本编辑器截图工具"));
    app.setQuitOnLastWindowClosed(false);

    if (argc < 3) {
        qWarning() << "用法: shots.exe <工程根目录> <输出目录>";
        return 1;
    }
    g_projectDir = QDir(QString::fromLocal8Bit(argv[1])).absolutePath();
    g_outputDir = QDir(QString::fromLocal8Bit(argv[2])).absolutePath();
    QDir().mkpath(g_outputDir);

    MainWindow window;
    g_window = &window;
    window.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    window.resize(780, 520);
    window.move(40, 40);
    window.show();
    wait(700);

    // 把鼠标挪到窗口外，免得工具栏按钮一直显示悬停高亮。
    QCursor::setPos(QGuiApplication::primaryScreen()->geometry().bottomRight() - QPoint(30, 30));
    wait(150);

    shot01();
    shot02();
    shot03();
    shot04();
    shot05();
    shot06();
    shot07();
    shot08();
    shot09();
    shot10();
    shot11();
    shot12();
    shot13();
    shot14();
    shot15();
    shot16();
    shot17();
    shot18();
    shot19();
    shot20();
    shot21();
    shot22();
    shot23();
    shot24();
    shot25();

    qInfo() << "全部截图完成，输出目录：" << g_outputDir;
    return 0;
}
