#include "mainwindow.h"
#include <QApplication>
#include <QPalette>
#include <QColor>

int main(int argc, char *argv[])
{
    QApplication::setStyle("Fusion");  
    QApplication a(argc, argv);

    QPalette pal;
    pal.setColor(QPalette::Window,           Qt::white);
    pal.setColor(QPalette::WindowText,       Qt::black);
    pal.setColor(QPalette::Base,             Qt::white);
    pal.setColor(QPalette::AlternateBase,    QColor(247,247,247));
    pal.setColor(QPalette::ToolTipBase,      Qt::white);
    pal.setColor(QPalette::ToolTipText,      Qt::black);
    pal.setColor(QPalette::Text,             Qt::black);
    pal.setColor(QPalette::Button,           Qt::white);
    pal.setColor(QPalette::ButtonText,       Qt::black);
    pal.setColor(QPalette::Highlight,        QColor(76,163,224));
    pal.setColor(QPalette::HighlightedText,  Qt::white);
    a.setPalette(pal);

    MainWindow w;
    w.show();
    return a.exec();
}