#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "appinfo.h"
#include "codeeditor.h"
#include "dialoghelpers.h"
#include "fileio.h"
#include "finddialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QMimeData>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_autoSaveTimer(new QTimer(this))
    , m_encoding(QString::fromLatin1(FileIo::kUtf8))
{
    ui->setupUi(this);
    m_editor = ui->textEdit;

    setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));
    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    setupToolBars();
    setupStatusBar();

    m_autoSaveTimer->setInterval(60 * 1000);      // 每 60 秒自动保存一次
    m_autoSaveTimer->setSingleShot(false);

    setupConnections();

    // 初始状态
    ui->actionToggleLineNumbers->setChecked(m_editor->lineNumbersVisible());
    ui->actionToggleWrap->setChecked(m_editor->autoWrap());
    ui->actionToggleToolBar->setChecked(true);
    ui->actionToggleStatusBar->setChecked(true);

    m_editor->document()->setModified(false);
    updateEditActions();
    updateStatusBar();
    updateWindowTitle();
    m_editor->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupToolBars()
{
    m_toolBars = {ui->toolBarFile, ui->toolBarEdit, ui->toolBarFormat};
    for (QToolBar *toolBar : m_toolBars) {
        toolBar->setMovable(false);
        toolBar->setFloatable(false);
    }
}

void MainWindow::setupStatusBar()
{
    m_lblLength = new QLabel(this);
    m_lblLines = new QLabel(this);
    m_lblPosition = new QLabel(this);
    m_lblEncoding = new QLabel(this);
    m_lblAutoSave = new QLabel(this);
    m_lblAppName = new QLabel(QStringLiteral("%1 %2")
                                  .arg(QString::fromUtf8(APP_NAME),
                                       QString::fromUtf8(APP_VERSION)),
                              this);

    const QString labelStyle = QStringLiteral("QLabel { padding-left: 8px; padding-right: 8px; }");
    QLabel *labels[] = {m_lblLength, m_lblLines, m_lblPosition, m_lblEncoding, m_lblAutoSave};
    for (QLabel *label : labels) {
        label->setStyleSheet(labelStyle);
        ui->statusbar->addPermanentWidget(label);         // 常驻区，不会被 showMessage 覆盖
    }
    m_lblAppName->setStyleSheet(labelStyle);
    ui->statusbar->addPermanentWidget(m_lblAppName);      // 小牛文本编辑器 V1.0
}

void MainWindow::setupConnections()
{
    // ---- 文件 ----
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionAutoSave, &QAction::toggled, this, &MainWindow::toggleAutoSave);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exitApplication);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);

    // ---- 编辑：全部转发给 QPlainTextEdit 自带实现 ----
    connect(ui->actionUndo, &QAction::triggered, m_editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, m_editor, &QPlainTextEdit::redo);
    connect(ui->actionSelectAll, &QAction::triggered, m_editor, &QPlainTextEdit::selectAll);

    // 重做同时支持 Ctrl+Y 与 Ctrl+Shift+Z
    ui->actionRedo->setShortcuts(QList<QKeySequence>()
                                 << QKeySequence(QStringLiteral("Ctrl+Y"))
                                 << QKeySequence(QStringLiteral("Ctrl+Shift+Z")));

    connect(ui->actionCut, &QAction::triggered, this, [this] {
        m_editor->cut();
        showMessage(tr("剪切所选内容到剪贴板"), 3000);
    });
    connect(ui->actionCopy, &QAction::triggered, this, [this] {
        m_editor->copy();
        showMessage(tr("复制所选内容到剪贴板"), 3000);
    });
    connect(ui->actionPaste, &QAction::triggered, this, [this] {
        m_editor->paste();
        showMessage(tr("粘贴剪贴板中的内容"), 3000);
    });

    // ---- 查找 / 替换 ----
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::openFindDialog);
    connect(ui->actionReplace, &QAction::triggered, this, &MainWindow::openReplaceDialog);

    // ---- 格式 ----
    connect(ui->actionFont, &QAction::triggered, this, &MainWindow::chooseFont);
    connect(ui->actionTextColor, &QAction::triggered, this, &MainWindow::chooseTextColor);
    connect(ui->actionTextBackgroundColor, &QAction::triggered,
            this, &MainWindow::chooseTextBackgroundColor);
    connect(ui->actionEditorBackgroundColor, &QAction::triggered,
            this, &MainWindow::chooseEditorBackgroundColor);
    connect(ui->actionToggleWrap, &QAction::toggled, this, &MainWindow::toggleWrap);
    connect(ui->actionToggleLineNumbers, &QAction::toggled,
            this, &MainWindow::toggleLineNumbers);

    // ---- 查看 / 帮助 ----
    connect(ui->actionToggleToolBar, &QAction::toggled, this, &MainWindow::toggleToolBars);
    connect(ui->actionToggleStatusBar, &QAction::toggled, this, &MainWindow::toggleStatusBar);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    // ---- 状态同步：六个信号都指向同一个槽 ----
    connect(m_editor, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateEditActions);
    connect(m_editor, &QPlainTextEdit::undoAvailable, this, &MainWindow::updateEditActions);
    connect(m_editor, &QPlainTextEdit::redoAvailable, this, &MainWindow::updateEditActions);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateEditActions);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateEditActions);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateEditActions);

    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusBar);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);
}

// ---------------------------------------------------------------------------
// 文件操作
// ---------------------------------------------------------------------------

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    m_editor->clear();
    m_filePath.clear();
    m_encoding = QString::fromLatin1(FileIo::kUtf8);
    m_editor->document()->setModified(false);

    updateWindowTitle();
    updateStatusBar();
    updateEditActions();
    m_editor->setFocus();
    showMessage(tr("已新建空白文档"), 2000);
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开文件"), m_lastDir,
        tr("文本文件 (*.txt *.md *.log *.ini *.json *.cpp *.h);;所有文件 (*)"));
    if (path.isEmpty())
        return;

    openPath(path);
}

bool MainWindow::openPath(const QString &path)
{
    const QFileInfo info(path);

    // 打开失败时不能动编辑器里已有的内容，所以所有检查都放在前面。
    if (!info.exists()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件不存在：\n%1").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    if (info.isDir()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("所选路径是一个文件夹：\n%1")
                                 .arg(QDir::toNativeSeparators(path)));
        return false;
    }
    if (!info.isFile()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("所选路径不是普通文件：\n%1")
                                 .arg(QDir::toNativeSeparators(path)));
        return false;
    }
    if (!info.isReadable()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件不可读：\n%1")
                                 .arg(QDir::toNativeSeparators(path)));
        return false;
    }
    if (info.size() > APP_MAX_FILE_SIZE) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件过大（%1 MB），超过 %2 MB 的打开上限。")
                                 .arg(info.size() / (1024.0 * 1024.0), 0, 'f', 1)
                                 .arg(APP_MAX_FILE_SIZE / (1024 * 1024)));
        return false;
    }

    if (!maybeSave())
        return false;

    QString text;
    QString encoding;
    QString error;
    if (!FileIo::readTextFile(path, &text, &encoding, &error)) {
        if (error.contains(QStringLiteral("NUL"))) {
            QMessageBox::warning(this, tr("打开文件"),
                                 tr("“%1”看起来是二进制文件，不是文本文件，已取消打开。")
                                     .arg(info.fileName()));
        } else {
            QMessageBox::warning(this, tr("打开文件"),
                                 tr("无法打开文件：\n%1\n\n%2")
                                     .arg(QDir::toNativeSeparators(path), error));
        }
        return false;
    }

    m_editor->setPlainText(text);
    m_editor->document()->setModified(false);
    m_encoding = encoding;
    setCurrentFile(path);

    showMessage(tr("已打开 %1（%2）").arg(info.fileName(), encoding), 5000);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;

    switch (askSaveChanges(this, displayFileName())) {   // 自绘中文按钮的询问框
    case SaveIt:
        return saveFile();
    case DiscardIt:
        return true;
    default:
        return false;                                    // 取消
    }
}

bool MainWindow::saveFile()
{
    if (m_filePath.isEmpty())
        return saveFileAs();        // 未命名文档先“另存为”
    return writeFile(m_filePath);
}

bool MainWindow::saveFileAs()
{
    const QString initial = m_filePath.isEmpty()
                                ? m_lastDir + QStringLiteral("/未命名.txt")
                                : m_filePath;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("另存为"), initial, tr("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty())
        return false;

    return writeFile(path);
}

bool MainWindow::writeFile(const QString &path, bool quiet)
{
    const QString text = m_editor->toPlainText();

    // 用 GBK 保存时先确认字符都能表示，避免悄悄变成“?”
    if (!FileIo::canEncodeText(text, m_encoding)) {
        if (quiet)
            return false;

        QMessageBox box(QMessageBox::Question, tr("保存文件"),
                        tr("当前编码（%1）无法表示文档中的全部字符。\n"
                           "是否改用 UTF-8 编码保存？").arg(m_encoding),
                        QMessageBox::NoButton, this);
        QPushButton *utf8Button = box.addButton(tr("改用 UTF-8"), QMessageBox::AcceptRole);
        box.addButton(tr("取消"), QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() != utf8Button)
            return false;
        m_encoding = QString::fromLatin1(FileIo::kUtf8);
    }

    QString error;
    if (!FileIo::writeTextFile(path, text, m_encoding, &error)) {   // QSaveFile 原子写
        if (!quiet) {
            // QFileDevice 的错误文本来自系统，常见场景翻成中文，便于对照实验报告。
            if (error.contains(QStringLiteral("not writable"), Qt::CaseInsensitive)
                || error.contains(QStringLiteral("Permission denied"), Qt::CaseInsensitive)) {
                error = tr("文件是只读的，没有写入权限。");
            }
            QMessageBox::warning(this, tr("保存文件"),
                                 tr("无法保存文件：\n%1\n\n%2")
                                     .arg(QDir::toNativeSeparators(path), error));
        }
        return false;                                              // 修改标记保持不变
    }

    m_editor->document()->setModified(false);
    setCurrentFile(path);            // 更新标题、最近目录
    updateStatusBar();
    updateEditActions();

    if (!quiet)
        showMessage(tr("已保存到 %1").arg(QFileInfo(path).fileName()), 3000);
    return true;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_filePath = path;
    m_lastDir = QFileInfo(path).absolutePath();
    m_editor->document()->setModified(false);
    updateWindowTitle();
    updateStatusBar();
}

QString MainWindow::displayFileName() const
{
    if (m_filePath.isEmpty())
        return tr("新建文本文件");
    return QFileInfo(m_filePath).fileName();
}

void MainWindow::exitApplication()
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();            // 用户点了“取消”就不关
}

// ---------------------------------------------------------------------------
// 标题、状态栏、Action 使能
// ---------------------------------------------------------------------------

void MainWindow::updateWindowTitle()
{
    const bool modified = m_editor->document()->isModified();
    setWindowTitle(QStringLiteral("%1%2 - %3")
                       .arg(modified ? QStringLiteral("* ") : QString(),
                            displayFileName(),
                            QString::fromUtf8(APP_NAME)));
}

void MainWindow::updateStatusBar()
{
    const QTextCursor cursor = m_editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int column = cursor.positionInBlock() + 1;
    // characterCount() 每个文本块多计一个分隔符，减去块数即纯字符数
    const int characters = qMax(0, m_editor->document()->characterCount() - m_editor->blockCount());

    m_lblLength->setText(tr("length: %1").arg(characters));
    m_lblLines->setText(tr("lines: %1").arg(m_editor->blockCount()));
    m_lblPosition->setText(tr("Ln: %1   Col: %2").arg(line).arg(column));
    m_lblEncoding->setText(m_encoding);
    m_lblAutoSave->setText(m_autoSaveTimer->isActive() ? tr("自动保存: 开") : tr("自动保存: 关"));
}

void MainWindow::updateEditActions()
{
    const bool hasSelection = m_editor->textCursor().hasSelection();
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    const bool canPaste = mimeData && mimeData->hasText();

    ui->actionUndo->setEnabled(m_editor->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(m_editor->document()->isRedoAvailable());
    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(canPaste);
}

QString MainWindow::currentFilePath() const
{
    return m_filePath;
}

QString MainWindow::currentEncoding() const
{
    return m_encoding;
}

CodeEditor *MainWindow::editor() const
{
    return m_editor;
}

void MainWindow::showMessage(const QString &text, int timeout)
{
    ui->statusbar->showMessage(text, timeout);
}

// ---------------------------------------------------------------------------
// 格式设置
// ---------------------------------------------------------------------------

void MainWindow::chooseFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, m_editor->font(), this, tr("字体"),
                                            QFontDialog::DontUseNativeDialog);
    if (!ok)
        return;

    m_editor->setEditorFont(font);                        // 同时刷新行号宽度、Tab 宽度
    showMessage(tr("字体：%1 %2 磅").arg(font.family()).arg(font.pointSize()), 3000);
}

void MainWindow::chooseTextColor()
{
    QColor initial = m_editor->currentCharFormat().foreground().color();
    if (!initial.isValid())
        initial = QColor(Qt::black);

    const QColor color = QColorDialog::getColor(initial, this, tr("字体颜色"),
                                                QColorDialog::DontUseNativeDialog);
    if (!color.isValid())
        return;

    QTextCharFormat format;
    format.setForeground(color);
    m_editor->applyCharFormat(format);                    // 有选区作用于选区，否则作用于后续输入
    showMessage(tr("字体颜色：%1").arg(color.name()), 3000);
}

void MainWindow::chooseTextBackgroundColor()
{
    QColor initial = m_editor->currentCharFormat().background().color();
    if (!initial.isValid())
        initial = QColor(Qt::white);

    const QColor color = QColorDialog::getColor(initial, this, tr("字体背景色"),
                                                QColorDialog::DontUseNativeDialog);
    if (!color.isValid())
        return;

    QTextCharFormat format;
    format.setBackground(color);
    m_editor->applyCharFormat(format);
    showMessage(tr("字体背景色：%1").arg(color.name()), 3000);
}

void MainWindow::chooseEditorBackgroundColor()
{
    const QColor color = QColorDialog::getColor(m_editor->editorBackgroundColor(), this,
                                                tr("编辑器背景色"),
                                                QColorDialog::DontUseNativeDialog);
    if (!color.isValid())
        return;

    m_editor->setEditorBackgroundColor(color);
    showMessage(tr("编辑器背景色：%1").arg(color.name()), 3000);
}

void MainWindow::toggleWrap(bool on)
{
    m_editor->setAutoWrap(on);
    showMessage(on ? tr("已开启自动换行") : tr("已关闭自动换行"), 2000);
}

void MainWindow::toggleLineNumbers(bool on)
{
    m_editor->setLineNumbersVisible(on);
    showMessage(on ? tr("已显示行号") : tr("已隐藏行号"), 2000);
}

// ---------------------------------------------------------------------------
// 工具栏 / 状态栏 / 自动保存
// ---------------------------------------------------------------------------

void MainWindow::toggleToolBars(bool visible)
{
    for (QToolBar *toolBar : m_toolBars)      // 文件 / 编辑 / 格式三个工具栏一起切换
        toolBar->setVisible(visible);
}

void MainWindow::toggleStatusBar(bool visible)
{
    ui->statusbar->setVisible(visible);
}

void MainWindow::toggleAutoSave(bool on)
{
    if (on) {
        m_autoSaveTimer->start();
        showMessage(tr("自动保存已开启：每 60 秒保存一次"), 3000);
    } else {
        m_autoSaveTimer->stop();
        showMessage(tr("自动保存已关闭"), 2000);
    }
    updateStatusBar();
}

void MainWindow::autoSave()
{
    if (m_filePath.isEmpty()) {
        showMessage(tr("自动保存：当前文档还没有保存路径，已跳过"), 3000);
        return;
    }
    if (!m_editor->document()->isModified())
        return;

    if (writeFile(m_filePath, true))
        showMessage(tr("自动保存完成：%1").arg(displayFileName()), 3000);
    else
        showMessage(tr("自动保存失败：%1").arg(displayFileName()), 3000);
}

// ---------------------------------------------------------------------------
// 查找 / 替换
// ---------------------------------------------------------------------------

bool MainWindow::findText(const QString &text, bool caseSensitive, bool backward)
{
    return findNext(text, caseSensitive, backward, true);
}

bool MainWindow::findNext(const QString &text, bool caseSensitive, bool backward, bool notify)
{
    if (text.isEmpty())
        return false;

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;

    if (m_editor->find(text, flags)) {          // 先从当前位置往后（前）找
        m_editor->centerCursor();
        return true;
    }

    // 找不到：移到文档另一端再找一次（循环查找）
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    if (m_editor->find(text, flags)) {
        m_editor->centerCursor();
        showMessage(backward ? tr("已到文档开头，从末尾继续查找")
                             : tr("已到文档末尾，从开头继续查找"),
                    4000);
        return true;
    }

    if (notify)
        QMessageBox::information(this, tr("查找"), tr("未找到“%1”。").arg(text));
    return false;
}

bool MainWindow::replaceOnce(const QString &text, const QString &replacement,
                             bool caseSensitive, bool backward)
{
    const QTextCursor cursor = m_editor->textCursor();
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const bool selectionMatches = cursor.hasSelection()
                                  && QString::compare(cursor.selectedText(), text, cs) == 0;

    if (!selectionMatches)                       // 当前选区不是查找内容：先选中下一个
        return findText(text, caseSensitive, backward);

    QTextCursor editCursor = cursor;
    editCursor.insertText(replacement);
    m_editor->setTextCursor(editCursor);

    // 立刻选中下一个，方便连续替换；替换完最后一处时不必再弹“未找到”。
    findNext(text, caseSensitive, backward, false);
    return true;
}

int MainWindow::replaceAll(const QString &text, const QString &replacement, bool caseSensitive)
{
    if (text.isEmpty())
        return 0;

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();                    // 合成一次撤销

    int count = 0;
    QTextDocument *document = m_editor->document();
    for (;;) {
        QTextCursor found = document->find(text, cursor, flags);
        if (found.isNull())
            break;
        found.insertText(replacement);
        cursor = found;                         // 从插入文本之后继续找，避免死循环
        ++count;
        if (count >= 100000)
            break;                              // 安全阀
    }

    cursor.endEditBlock();
    return count;
}

void MainWindow::openFindDialog()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(false, this);
        connect(m_findDialog, &FindDialog::findRequested, this, &MainWindow::handleFindRequest);
    }

    const QString selected = m_editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        m_findDialog->setFindText(selected);

    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::openReplaceDialog()
{
    if (!m_replaceDialog) {
        m_replaceDialog = new FindDialog(true, this);
        connect(m_replaceDialog, &FindDialog::findRequested,
                this, &MainWindow::handleFindRequest);
        connect(m_replaceDialog, &FindDialog::replaceRequested,
                this, &MainWindow::handleReplaceRequest);
        connect(m_replaceDialog, &FindDialog::replaceAllRequested,
                this, &MainWindow::handleReplaceAllRequest);
    }

    const QString selected = m_editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator))
        m_replaceDialog->setFindText(selected);

    m_replaceDialog->show();
    m_replaceDialog->raise();
    m_replaceDialog->activateWindow();
}

void MainWindow::handleFindRequest(const QString &text, bool caseSensitive, bool backward)
{
    findText(text, caseSensitive, backward);
}

void MainWindow::handleReplaceRequest(const QString &text, const QString &replacement,
                                      bool caseSensitive, bool backward)
{
    replaceOnce(text, replacement, caseSensitive, backward);
}

void MainWindow::handleReplaceAllRequest(const QString &text, const QString &replacement,
                                         bool caseSensitive)
{
    const int count = replaceAll(text, replacement, caseSensitive);
    if (count > 0) {
        showMessage(tr("已替换 %1 处。").arg(count), 4000);
        QMessageBox::information(this, tr("替换"), tr("已替换 %1 处。").arg(count));
    } else {
        QMessageBox::information(this, tr("替换"), tr("未找到“%1”。").arg(text));
    }
}

// ---------------------------------------------------------------------------
// 帮助
// ---------------------------------------------------------------------------

void MainWindow::showAbout()
{
    AboutDialog dialog(this);
    dialog.exec();
}
