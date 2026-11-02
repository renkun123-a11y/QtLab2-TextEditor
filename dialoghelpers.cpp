#include "dialoghelpers.h"

#include <QMessageBox>
#include <QPushButton>

SaveChoice askSaveChanges(QWidget *parent, const QString &fileName)
{
    QMessageBox box(parent);
    box.setWindowTitle(QObject::tr("保存修改"));
    box.setIcon(QMessageBox::Warning);
    box.setText(QObject::tr("文档“%1”已被修改。\n是否保存修改？").arg(fileName));

    QPushButton *saveButton = box.addButton(QObject::tr("保存"), QMessageBox::AcceptRole);
    box.addButton(QObject::tr("不保存"), QMessageBox::DestructiveRole);
    box.addButton(QObject::tr("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);

    box.exec();

    if (box.clickedButton() == saveButton)
        return SaveIt;

    // “不保存”以外的关闭方式（Esc、右上角关闭）一律按取消处理，
    // 避免误丢修改。
    if (box.buttonRole(box.clickedButton()) == QMessageBox::DestructiveRole)
        return DiscardIt;

    return CancelIt;
}
