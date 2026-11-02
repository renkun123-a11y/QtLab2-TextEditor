#include "finddialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>

FindDialog::FindDialog(bool replaceMode, QWidget *parent)
    : QDialog(parent)
    , m_replaceMode(replaceMode)
{
    setWindowTitle(m_replaceMode ? tr("替换") : tr("查找"));
    setModal(true);

    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(12, 12, 12, 12);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 78);
    grid->setColumnStretch(1, 1);

    // ---- 查找目标 ----
    m_editFind = new QLineEdit(this);
    m_btnFind = new QPushButton(tr("查找下一个(&F)"), this);
    m_btnFind->setDefault(true);

    grid->addWidget(new QLabel(tr("查找目标:")), 0, 0);
    grid->addWidget(m_editFind, 0, 1);
    grid->addWidget(m_btnFind, 0, 2);

    int row = 1;

    // ---- 替换为（仅替换模式）----
    if (m_replaceMode) {
        m_editReplace = new QLineEdit(this);
        m_btnReplace = new QPushButton(tr("替换(&R)"), this);
        grid->addWidget(new QLabel(tr("替换为:")), row, 0);
        grid->addWidget(m_editReplace, row, 1);
        grid->addWidget(m_btnReplace, row, 2);
        ++row;
    }

    // ---- 方向：向上 / 向下 ----
    m_grpDirection = new QGroupBox(tr("方向"), this);
    QHBoxLayout *directionLayout = new QHBoxLayout(m_grpDirection);
    directionLayout->setContentsMargins(9, 6, 9, 6);
    directionLayout->setSpacing(12);

    m_radioUp = new QRadioButton(tr("向上(&U)"), m_grpDirection);
    m_radioDown = new QRadioButton(tr("向下(&D)"), m_grpDirection);
    m_radioDown->setChecked(true);
    directionLayout->addWidget(m_radioUp);
    directionLayout->addWidget(m_radioDown);

    m_chkCaseSensitive = new QCheckBox(tr("区分大小写"), this);
    m_btnCancel = new QPushButton(tr("取消"), this);

    if (m_replaceMode) {
        // 第三行 = 区分大小写 | 方向 | 全部替换
        m_btnReplaceAll = new QPushButton(tr("全部替换(&A)"), this);
        grid->addWidget(m_chkCaseSensitive, row, 0);
        grid->addWidget(m_grpDirection, row, 1);
        grid->addWidget(m_btnReplaceAll, row, 2);
        ++row;
        grid->addWidget(m_btnCancel, row, 2);   // 第四行 = 取消
        ++row;
    } else {
        grid->addWidget(m_grpDirection, row, 1);
        grid->addWidget(m_btnCancel, row, 2);
        ++row;
        grid->addWidget(m_chkCaseSensitive, row, 0, 1, 2);
        ++row;
    }

    grid->setSizeConstraint(QLayout::SetFixedSize);

    connect(m_btnFind, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &FindDialog::reject);
    if (m_btnReplace)
        connect(m_btnReplace, &QPushButton::clicked, this, &FindDialog::onReplaceClicked);
    if (m_btnReplaceAll)
        connect(m_btnReplaceAll, &QPushButton::clicked, this, &FindDialog::onReplaceAllClicked);

    m_editFind->setFocus();
}

void FindDialog::setFindText(const QString &text)
{
    m_editFind->setText(text);
    m_editFind->selectAll();
}

void FindDialog::setReplaceText(const QString &text)
{
    if (m_editReplace)
        m_editReplace->setText(text);
}

bool FindDialog::backward() const
{
    return m_radioUp->isChecked();
}

bool FindDialog::validateSearchText()
{
    if (m_editFind->text().isEmpty()) {
        QMessageBox::warning(this, tr("查找"), tr("请输入要查找的内容。"));
        m_editFind->setFocus();
        return false;
    }
    return true;
}

void FindDialog::onFindClicked()
{
    if (!validateSearchText())          // 空串直接提示，不发出信号
        return;
    emit findRequested(m_editFind->text(), m_chkCaseSensitive->isChecked(), backward());
}

void FindDialog::onReplaceClicked()
{
    if (!validateSearchText())
        return;
    emit replaceRequested(m_editFind->text(), m_editReplace->text(),
                          m_chkCaseSensitive->isChecked(), backward());
}

void FindDialog::onReplaceAllClicked()
{
    if (!validateSearchText())
        return;
    emit replaceAllRequested(m_editFind->text(), m_editReplace->text(),
                             m_chkCaseSensitive->isChecked());
}
