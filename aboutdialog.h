#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

// “关于”对话框：用 QGridLayout 手工排版，内容来自 appinfo.h。
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
};

#endif // ABOUTDIALOG_H
