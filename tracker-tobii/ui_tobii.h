/********************************************************************************
** Form generated from reading UI file 'tobii.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOBII_H
#define UI_TOBII_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_tobii_ui
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *tobii_ui)
    {
        if (tobii_ui->objectName().isEmpty())
            tobii_ui->setObjectName("tobii_ui");
        tobii_ui->setWindowModality(Qt::NonModal);
        tobii_ui->resize(278, 58);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/tobii_logo.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        tobii_ui->setWindowIcon(icon);
        tobii_ui->setLayoutDirection(Qt::LeftToRight);
        tobii_ui->setAutoFillBackground(false);
        verticalLayout = new QVBoxLayout(tobii_ui);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(tobii_ui);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(label);

        buttonBox = new QDialogButtonBox(tobii_ui);
        buttonBox->setObjectName("buttonBox");
        sizePolicy.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(tobii_ui);

        QMetaObject::connectSlotsByName(tobii_ui);
    } // setupUi

    void retranslateUi(QWidget *tobii_ui)
    {
        tobii_ui->setWindowTitle(QCoreApplication::translate("tobii_ui", "Tobii Eye Tracker", nullptr));
        label->setText(QCoreApplication::translate("tobii_ui", "Please make sure the Tobii Experience application is running and tracking is active.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class tobii_ui: public Ui_tobii_ui {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOBII_H
