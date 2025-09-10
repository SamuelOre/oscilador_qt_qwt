#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QStringList>
#include <QPixmap>

// Adelantos
class QPushButton;
class QListWidget;
class QTimer;
class QwtPlot;
class QwtPlotCurve;
class QwtKnob;
class SnapshotViewer;
class QCheckBox;
class QFrame;
class QLabel;
class QSlider;
class QLCDNumber;
class QDoubleSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateData();
    void changeColor();
    void onItemSelected();
    void toggleTimer();
    void takeSnapshot();
    void onOffsetSpinChanged(double val);
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;     

private:
    
    QPushButton *btnStop        = nullptr;
    QPushButton *btnFoto        = nullptr;   
    QPushButton *btnChangeColor = nullptr;
    QCheckBox   *chkShowLegend  = nullptr; 
    QFrame      *legendOverlay  = nullptr;   
    QLabel      *legendLabel    = nullptr;

    // Widgets
    QwtPlot       *plot       = nullptr;
    QwtPlotCurve  *curve      = nullptr;
    QwtKnob       *knobAmp    = nullptr;
    QwtKnob       *knobFreq   = nullptr;
    QListWidget   *listWidget = nullptr;
    QTimer        *timer      = nullptr;

    QDoubleSpinBox *spnOffset = nullptr;
    double yOffset = 0.0;   // offset vertical actual

    void positionLegendOverlay();         
    void updateLegendText();              

    // Datos del oscilador
    QVector<double> xData;
    QVector<double> yData;
    double amplitude = 0.0;
    double frequency = 0.0;
    double phase     = 0.0;

    // --- Capturas ---
    QStringList        snapshotTitles;   // "Item 1", "Item 2", ...
    QVector<QPixmap>   snapshots;        // imágenes del plot
    QVector<double>    snapshotAmps;
    QVector<double>    snapshotFreqs;
    int                snapshotCount = 0;
    SnapshotViewer    *viewer = nullptr; // ventana galería
};

#endif // MAINWINDOW_H