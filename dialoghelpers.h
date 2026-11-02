#ifndef DIALOGHELPERS_H
#define DIALOGHELPERS_H

#include <QString>

class QWidget;

// 关闭 / 新建 / 打开之前询问是否保存，三个选项分别对应
// “保存”“不保存”“取消”。
enum SaveChoice {
    SaveIt,
    DiscardIt,
    CancelIt
};

// 自绘中文按钮的询问框：QMessageBox 的标准按钮在未加载翻译时是英文，
// 这里统一用 addButton 指定中文按钮，交互和系统消息框保持一致。
SaveChoice askSaveChanges(QWidget *parent, const QString &fileName);

#endif // DIALOGHELPERS_H
