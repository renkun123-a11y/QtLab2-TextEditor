#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QList>

class CodeEditor;
class FindDialog;
class QLabel;
class QTimer;
class QToolBar;
class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // 以下接口同时供命令行参数、拖放打开和自动化测试使用。
    bool openPath(const QString &path);
    bool saveFile();
    bool saveFileAs();
    bool maybeSave();
    bool findText(const QString &text, bool caseSensitive, bool backward);
    int replaceAll(const QString &text, const QString &replacement, bool caseSensitive);
    bool replaceOnce(const QString &text, const QString &replacement,
                     bool caseSensitive, bool backward);

    QString currentFilePath() const;
    QString currentEncoding() const;
    CodeEditor *editor() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void exitApplication();

    void chooseFont();
    void chooseTextColor();
    void chooseTextBackgroundColor();
    void chooseEditorBackgroundColor();
    void toggleWrap(bool on);
    void toggleLineNumbers(bool on);
    void toggleToolBars(bool visible);
    void toggleStatusBar(bool visible);
    void toggleAutoSave(bool on);
    void autoSave();

    void showAbout();
    void openFindDialog();
    void openReplaceDialog();
    void handleFindRequest(const QString &text, bool caseSensitive, bool backward);
    void handleReplaceRequest(const QString &text, const QString &replacement,
                              bool caseSensitive, bool backward);
    void handleReplaceAllRequest(const QString &text, const QString &replacement,
                                 bool caseSensitive);

    void updateEditActions();
    void updateStatusBar();
    void updateWindowTitle();

private:
    void setupStatusBar();
    void setupToolBars();
    void setupConnections();
    bool findNext(const QString &text, bool caseSensitive, bool backward, bool notify);
    bool writeFile(const QString &path, bool quiet = false);
    void setCurrentFile(const QString &path);
    QString displayFileName() const;
    void showMessage(const QString &text, int timeout = 3000);

    Ui::MainWindow *ui;
    CodeEditor *m_editor;

    FindDialog *m_findDialog = nullptr;
    FindDialog *m_replaceDialog = nullptr;
    QTimer *m_autoSaveTimer;

    QList<QToolBar *> m_toolBars;

    QLabel *m_lblLength;
    QLabel *m_lblLines;
    QLabel *m_lblPosition;
    QLabel *m_lblEncoding;
    QLabel *m_lblAutoSave;
    QLabel *m_lblAppName;

    QString m_filePath;      // 当前文档的完整路径，空表示“新建文本文件”
    QString m_encoding;      // 当前文档的编码，保存时按原编码写回
    QString m_lastDir;       // 文件对话框的起始目录
};

#endif // MAINWINDOW_H
