#include "aboutdialog.h"
#include "appinfo.h"

#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("关于"));
    setModal(true);

    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(16, 12, 16, 12);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    QLabel *titleLabel = new QLabel(QString::fromUtf8(APP_NAME), this);   // 程序名
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    grid->addWidget(titleLabel, 0, 0, 1, 3);

    QLabel *iconLabel = new QLabel(this);                                 // 图标
    iconLabel->setPixmap(QPixmap(QStringLiteral(":/icons/app.png"))
                             .scaled(96, 96, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
    grid->addWidget(iconLabel, 1, 0);

    QLabel *infoLabel = new QLabel(this);                                 // 开发者信息
    infoLabel->setText(QStringLiteral(
        "<p style=\"margin:0;\">开发者：%1</p>"
        "<p style=\"margin:6px 0 0 0;\">学&nbsp;&nbsp;&nbsp;号：%2</p>"
        "<p style=\"margin:6px 0 0 0;\">email：%3</p>")
        .arg(QString::fromUtf8(APP_DEVELOPER),
             QString::fromUtf8(APP_STUDENT_ID),
             QString::fromUtf8(APP_EMAIL)));
    grid->addWidget(infoLabel, 1, 2);

    QLabel *versionLabel = new QLabel(QString::fromUtf8(APP_VERSION_FULL), this);
    grid->addWidget(versionLabel, 2, 0);                                  // V1.0 build 202611

    QPushButton *okButton = new QPushButton(tr("OK"), this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    grid->addWidget(okButton, 2, 2, Qt::AlignRight);                      // OK

    grid->setSizeConstraint(QLayout::SetFixedSize);                       // 按内容固定大小
}
