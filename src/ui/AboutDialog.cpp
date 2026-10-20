#include "ui/AboutDialog.h"

#include "AppInfo.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace iat {

QString AboutDialog::homepage() { return QStringLiteral(IAT_HOMEPAGE); }
QString AboutDialog::author() { return QStringLiteral(IAT_AUTHOR); }

void AboutDialog::openHomepage()
{
    QDesktopServices::openUrl(QUrl(homepage()));
}

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About %1").arg(QStringLiteral(IAT_APP_NAME)));

    auto* iconLabel = new QLabel(this);
    const QIcon icon = QApplication::windowIcon();
    if (!icon.isNull())
        iconLabel->setPixmap(icon.pixmap(96, 96));
    iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    iconLabel->setMinimumWidth(110);

    auto* title = new QLabel(QStringLiteral(IAT_APP_NAME), this);
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() + 6);
    tf.setBold(true);
    title->setFont(tf);

    auto* version = new QLabel(tr("Version %1").arg(QStringLiteral(IAT_VERSION)), this);

    const QString link = QStringLiteral("<a href=\"%1\">%2</a>").arg(homepage(), author());
    const QString shortUrl = QString(homepage()).remove(QStringLiteral("https://"));
    auto* text = new QLabel(this);
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    text->setOpenExternalLinks(true);
    text->setTextInteractionFlags(Qt::TextBrowserInteraction);
    text->setText(tr("<p>Dark-frame calibration and noise analysis for image stacks stored in MATLAB "
                     ".mat files.</p>"
                     "<p><b>Light frames</b>: one frame of the X × Y × Z stack<br>"
                     "<b>Dark frames</b>: pixel-by-pixel mean over Z<br>"
                     "<b>Calibrated frames</b>: light frame − mean dark frame</p>"
                     "<p>Made by %1<br><a href=\"%2\">%3</a></p>"
                     "<p style=\"color:gray\">Built with Qt %4</p>")
                      .arg(link, homepage(), shortUrl, QString::fromLatin1(qVersion())));
    text->setMinimumWidth(380);

    auto* textColumn = new QVBoxLayout;
    textColumn->setSpacing(4);
    textColumn->addWidget(title);
    textColumn->addWidget(version);
    textColumn->addSpacing(6);
    textColumn->addWidget(text);
    textColumn->addStretch(1);

    auto* body = new QHBoxLayout;
    body->setSpacing(16);
    body->addWidget(iconLabel);
    body->addLayout(textColumn, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton* github = buttons->addButton(tr("Open GitHub page"), QDialogButtonBox::ActionRole);
    QPushButton* aboutQt = buttons->addButton(tr("About Qt"), QDialogButtonBox::HelpRole);
    connect(github, &QPushButton::clicked, this, [] { openHomepage(); });
    connect(aboutQt, &QPushButton::clicked, qApp, &QApplication::aboutQt);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 14);
    layout->addLayout(body);
    layout->addSpacing(10);
    layout->addWidget(buttons);
}

} // namespace iat
