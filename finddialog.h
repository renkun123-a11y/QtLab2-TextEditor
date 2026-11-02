#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

class QCheckBox;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QRadioButton;

// 查找 / 替换对话框。
//
// 构造参数 replaceMode 决定是否显示“替换为”这一行：查找和替换共用同一个
// 类，布局只差一行，“取消”按钮的位置也随之调整。
//
// 对话框自身不操作编辑器，只把用户的选择通过信号发给主窗口，
// “未找到”之类的提示统一在主窗口里处理。
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(bool replaceMode, QWidget *parent = nullptr);

    void setFindText(const QString &text);
    void setReplaceText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive, bool backward);
    void replaceRequested(const QString &text, const QString &replacement,
                          bool caseSensitive, bool backward);
    void replaceAllRequested(const QString &text, const QString &replacement,
                             bool caseSensitive);

private slots:
    void onFindClicked();
    void onReplaceClicked();
    void onReplaceAllClicked();

private:
    bool validateSearchText();
    bool backward() const;

    bool m_replaceMode;

    QLineEdit *m_editFind;
    QLineEdit *m_editReplace = nullptr;

    QPushButton *m_btnFind;
    QPushButton *m_btnReplace = nullptr;
    QPushButton *m_btnReplaceAll = nullptr;
    QPushButton *m_btnCancel;

    QGroupBox *m_grpDirection;
    QRadioButton *m_radioUp;
    QRadioButton *m_radioDown;
    QCheckBox *m_chkCaseSensitive;
};

#endif // FINDDIALOG_H
