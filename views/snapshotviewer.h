#pragma once
#include <QWidget>
#include <QStringList>
#include <QVector>
#include <QPixmap>

class QListWidget;
class QLabel;
class QPushButton;  

class SnapshotViewer : public QWidget
{
    Q_OBJECT
public:
    explicit SnapshotViewer(QWidget *parent = nullptr);

    void setData(const QStringList &titles,
                 const QVector<QPixmap> &images,
                 const QVector<double> &amps,
                 const QVector<double> &freqs);

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onRowChanged(int row);
    void onSaveImage();

private:
    QListWidget *list_ = nullptr;
    QLabel      *hdr1_ = nullptr;  // "Item #N, Amplitude: A, Frequency: F Hz"
    QLabel      *hdr2_ = nullptr;  // "Widget Vsita de Imagen"
    QPushButton *btnSave_ = nullptr; 
    QLabel      *image_ = nullptr;

    QStringList      titles_;
    QVector<QPixmap> images_;
    QVector<double>  amps_;
    QVector<double>  freqs_;
    int              currentRow_ = -1;

    void showIndex(int row);
    void updateHeaders(int row);
};