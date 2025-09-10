#include "mainwindow.h"
#include "snapshotviewer.h"
#include <QTimer>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPalette>
#include <QPen>
#include <QColor>
#include <QColorDialog>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_knob.h>
#include <qwt_plot_grid.h>
#include <qwt_text.h>
#include <qwt_scale_widget.h>
#include <QRandomGenerator>
#include <QLabel>
#include <QEvent>
#include <QFrame>
#include <QCheckBox>
#include <QSlider>
#include <QLCDNumber>
#include <QGroupBox>
#include <QDoubleSpinBox>   

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), amplitude(50.0), frequency(5.0), phase(0.0)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

// Fuerza fondo claro en la ventana
{
    QPalette p = this->palette();
    p.setColor(QPalette::Window, Qt::white);
    p.setColor(QPalette::WindowText, Qt::black);
    this->setPalette(p);
}

// Fuerza fondo claro en el central widget
{
    QPalette p = central->palette();
    p.setColor(QPalette::Window, Qt::white);
    central->setAutoFillBackground(true); // importante para que pinte el fondo
    central->setPalette(p);
}

    // --- Plot y curva ---
    plot = new QwtPlot(this);
    plot->setAxisTitle(QwtPlot::xBottom, "Tiempo [s]");
    plot->setAxisTitle(QwtPlot::yLeft, "Amplitud");

    // --- Desactivar autoscale y fijar ejes para que el canvas NO siga la curva ---
    plot->setAxisAutoScale(QwtPlot::xBottom, false);
    plot->setAxisAutoScale(QwtPlot::yLeft,   false);
    plot->setAxisScale(QwtPlot::xBottom, 0.0, 0.5);
    plot->setAxisScale(QwtPlot::yLeft,  -100.0, 100.0);

    // no re-escalar nunca automáticamente
    plot->setAutoReplot(false); 

    // Tema claro en el canvas
    plot->setCanvasBackground(Qt::white);

    // curva suave
    curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true); 
    curve->setPen(QPen(Qt::green, 2)); 
    curve->attach(plot);

    // Grid claro
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setPen(QPen(QColor(220, 220, 220))); // gris suave
    grid->attach(plot);

    // --- Overlay de leyenda sobre el canvas del plot ---
    legendOverlay = new QFrame(plot->canvas());
    legendOverlay->hide(); // = setVisible(false)
    legendOverlay->setObjectName("legendOverlay");

    // QFrame sin marco alguno
    legendOverlay->setFrameShape(QFrame::NoFrame); // usamos CSS para el borde
    legendOverlay->setFrameShadow(QFrame::Plain);
    legendOverlay->setLineWidth(0);
    legendOverlay->setMidLineWidth(0);

    // no pintar fondo automáticamente
    legendOverlay->setAutoFillBackground(false);
    legendOverlay->setAttribute(Qt::WA_StyledBackground, true);
    legendOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Fondo 100% transparente y sin bordes
    legendOverlay->setStyleSheet(
        "#legendOverlay {"
        "  background: transparent;"
        "  border: none; outline: none;"
        "  border-radius: 0px;"      /* si quieres esquinas redondeadas, pon 6px */
        "}"
        "#legendOverlay * {"
        "  background: transparent;"
        "  border: none !important; outline: none !important;"
        "}"
    );

    // Contenido
    auto *legendLayout = new QVBoxLayout(legendOverlay);
    legendLayout->setContentsMargins(10, 8, 10, 8);
    legendLayout->setSpacing(4);

    legendLabel = new QLabel(legendOverlay);
    legendLabel->setAlignment(Qt::AlignCenter);
    legendLabel->setWordWrap(true);
    legendLabel->setStyleSheet("font-weight: 700; background: transparent;"); 
    legendLayout->addWidget(legendLabel);

    // reposicionar cuando cambie el tamaño del canvas
    plot->canvas()->installEventFilter(this);
    positionLegendOverlay();
    updateLegendText();

    // Ejes/títulos en negro
    {
        QwtText tx = plot->axisTitle(QwtPlot::xBottom);
        tx.setColor(Qt::black);
        plot->setAxisTitle(QwtPlot::xBottom, tx);

        QwtText ty = plot->axisTitle(QwtPlot::yLeft);
        ty.setColor(Qt::black);
        plot->setAxisTitle(QwtPlot::yLeft, ty);

        auto axX = plot->axisWidget(QwtPlot::xBottom);
        auto axY = plot->axisWidget(QwtPlot::yLeft);
        if (axX) { QPalette p = axX->palette(); p.setColor(QPalette::WindowText, Qt::black); axX->setPalette(p); }
        if (axY) { QPalette p = axY->palette(); p.setColor(QPalette::WindowText, Qt::black); axY->setPalette(p); }
    }

    // --- Controles ---
    knobAmp = new QwtKnob();
    knobAmp->setLowerBound(0);
    knobAmp->setUpperBound(100);
    knobAmp->setValue(amplitude);
    knobAmp->setKnobWidth(50);

    knobFreq = new QwtKnob();
    knobFreq->setLowerBound(1);
    knobFreq->setUpperBound(50);
    knobFreq->setValue(frequency);
    knobFreq->setKnobWidth(50);

    QLabel *lblAmp  = new QLabel("Amplitude");
    QLabel *lblFreq = new QLabel("Frequency [Hz]"); 

    // centrado y estilo claro
    lblAmp->setAlignment(Qt::AlignHCenter);
    lblFreq->setAlignment(Qt::AlignHCenter);

    // Paleta clara para ambos knobs (tal vez no es necesario)
    {
        QPalette kp = knobAmp->palette();
        kp.setColor(QPalette::Window, Qt::white);
        kp.setColor(QPalette::Base,   Qt::white);
        kp.setColor(QPalette::Button, Qt::white);
        kp.setColor(QPalette::Text,   Qt::black);
        knobAmp->setPalette(kp);
        knobFreq->setPalette(kp);
    }

    // ---botones----
    btnStop        = new QPushButton("Stop");
    btnFoto        = new QPushButton("Foto Actual");
    btnChangeColor = new QPushButton("change color");

    btnStop->setMinimumHeight(36);
    btnFoto->setMinimumHeight(36);
    btnChangeColor->setMinimumHeight(36);

    connect(btnFoto, &QPushButton::clicked, this, &MainWindow::takeSnapshot);

// --- Layouts ---
// =============== ESTRUCTURA FINAL ==========================
// [           PLOT (≈ 50%)            |  (≈ 50%) RIGHT SIDE  ]
//                                     | [ Knobs | Botones | Lista ]
// ===========================================================

// columna1
auto *colKnobs = new QVBoxLayout();
colKnobs->setContentsMargins(0, 0, 0, 0);
colKnobs->setSpacing(6);

colKnobs->addWidget(knobAmp, 0, Qt::AlignHCenter);
colKnobs->addWidget(lblAmp);

colKnobs->addSpacing(8);

colKnobs->addWidget(knobFreq, 0, Qt::AlignHCenter);
colKnobs->addWidget(lblFreq);

colKnobs->addStretch();

// columna2
auto *colButtons = new QVBoxLayout();
colButtons->setContentsMargins(0, 0, 0, 0);
colButtons->setSpacing(10);
colButtons->addWidget(btnStop);
colButtons->addWidget(btnFoto);
colButtons->addWidget(btnChangeColor);

// añade el checkbox "Show Legend"
chkShowLegend = new QCheckBox("Show Legend");
chkShowLegend->setChecked(false);
colButtons->addWidget(chkShowLegend);

// ---- Offset vertical con QDoubleSpinBox (debajo de Show Legend) ----
auto *offsetRow = new QHBoxLayout();
offsetRow->setContentsMargins(0,0,0,0);
offsetRow->setSpacing(8);

auto *lblOffset = new QLabel("Offset");
lblOffset->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

spnOffset = new QDoubleSpinBox();
spnOffset->setRange(-100.0, 100.0);
spnOffset->setDecimals(2);
spnOffset->setSingleStep(1.0);
spnOffset->setValue(0.0);            // ✅ arranca en 0
spnOffset->setAlignment(Qt::AlignRight);

offsetRow->addWidget(lblOffset);
offsetRow->addWidget(spnOffset, /*stretch*/ 1);

// lo añadimos a la columna de botones
colButtons->addLayout(offsetRow);

// conexión
connect(spnOffset, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::onOffsetSpinChanged);

colButtons->addStretch();

// columna3
auto *colList = new QVBoxLayout();
colList->setContentsMargins(0, 0, 0, 0);
colList->setSpacing(10);
colList->addWidget(listWidget);

auto *rightSide = new QWidget(central);
auto *rightRow  = new QHBoxLayout(rightSide);
rightRow->setContentsMargins(0, 0, 0, 0);
rightRow->setSpacing(12);
rightRow->addLayout(colKnobs);
rightRow->addLayout(colButtons);
rightRow->addLayout(colList);

auto *mainLayout = new QHBoxLayout(central);
mainLayout->setContentsMargins(8, 8, 8, 8);
mainLayout->setSpacing(12);
mainLayout->addWidget(plot,     /*stretch*/ 1);
mainLayout->addWidget(rightSide,/*stretch*/ 1);

// (Opcional) asegurar que el plot no colapse
plot->setMinimumWidth(420);

// --- Timer ---
timer = new QTimer(this);
connect(timer, &QTimer::timeout, this, &MainWindow::updateData);
timer->start(50); // Actualización cada 50 ms

connect(btnStop, &QPushButton::clicked, this, &MainWindow::toggleTimer);

connect(btnChangeColor, &QPushButton::clicked, this, &MainWindow::changeColor);

connect(chkShowLegend, &QCheckBox::toggled, this, [this](bool on){
    if (!legendOverlay) return;
    legendOverlay->setVisible(on);
    positionLegendOverlay();
    updateLegendText();
});

connect(knobAmp,  &QwtKnob::valueChanged, this, [this](double){
    if (legendOverlay && legendOverlay->isVisible()) updateLegendText();
});
connect(knobFreq, &QwtKnob::valueChanged, this, [this](double){
    if (legendOverlay && legendOverlay->isVisible()) updateLegendText();
});
}

MainWindow::~MainWindow() {}

void MainWindow::updateData()
{
    amplitude = knobAmp->value();
    frequency = knobFreq->value();

    int N = 200;
    xData.resize(N);
    yData.resize(N);

    double dt = 0.01;
    for (int i = 0; i < N; i++) {
    xData[i] = i * dt;
    yData[i] = amplitude * sin(2 * M_PI * frequency * xData[i] + phase) + yOffset; 
}
    phase += 0.1;

    curve->setSamples(xData, yData);
    plot->replot();

}

void MainWindow::changeColor()
{
    // recordamos el último color elegido para que el diálogo abra con ese
    static QColor lastColor = Qt::green;

    QColor chosen = QColorDialog::getColor(
        lastColor,                 // color inicial del diálogo
        this,                      // parent
        "Selecciona color de curva",
        QColorDialog::ShowAlphaChannel
    );

    if (!chosen.isValid())
        return; // canceló el diálogo

    lastColor = chosen;
    curve->setPen(QPen(chosen, 2));
    plot->replot();
}

void MainWindow::toggleTimer()
{
    if (!timer) return;
    if (timer->isActive()) { timer->stop(); btnStop->setText("Reanudar"); }
    else { timer->start(50); btnStop->setText("Stop"); }
}

void MainWindow::takeSnapshot()
{
    if (!plot) return;                  // seguridad

    // Captura del plot
    QPixmap pm = plot->grab();

    // Lee valores actuales 
    const double ampNow  = knobAmp ? knobAmp->value()  : amplitude;
    const double freqNow = knobFreq ? knobFreq->value() : frequency;

    // Genera "Item N"
    ++snapshotCount;
    const QString title = QString("Item %1").arg(snapshotCount);

    // Guarda y refleja en la lista del MainWindow
    snapshotTitles << title;
    snapshots      << pm;
    snapshotAmps   << ampNow;    
    snapshotFreqs  << freqNow;
    if (listWidget) listWidget->addItem(title);

    // Crea (si no existe) y actualiza la galería
    if (!viewer) {
        viewer = new SnapshotViewer(nullptr); // <-- sin padre: ventana top-level
        viewer->setAttribute(Qt::WA_DeleteOnClose);

        // Botones y barra de título como ventana normal
        viewer->setWindowFlags(
            Qt::Window |
            Qt::WindowTitleHint |
            Qt::WindowSystemMenuHint |
            Qt::WindowMinMaxButtonsHint |
            Qt::WindowCloseButtonHint
        );
        viewer->setWindowTitle("Galería de capturas");
        viewer->resize(900, 600);

        // Cuando se cierre, deja el puntero en nullptr
        connect(viewer, &QObject::destroyed, this, [this](){ viewer = nullptr; });
    }
    viewer->setData(snapshotTitles, snapshots, snapshotAmps, snapshotFreqs);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
}

void MainWindow::updateLegendText()
{
    if (!legendLabel) return;
    const double A = knobAmp ? knobAmp->value() : amplitude;
    const double F = knobFreq ? knobFreq->value() : frequency;

    legendLabel->setText(
        QString("Amplitude: %1\nFrequency: %2 Hz")
            .arg(A, 0, 'f', 2)
            .arg(F, 0, 'f', 2)
    );
}

void MainWindow::positionLegendOverlay()
{
    if (!legendOverlay || !plot || !plot->canvas()) return;

    const QSize cs = plot->canvas()->size();
    const int cw = cs.width();
    const int ch = cs.height();
    if (cw <= 0 || ch <= 0) return;

    // cuadrante superior derecho
    const int qx = cw / 2;
    const int qw = cw - qx;   // = cw/2
    const int qh = ch / 2;

    // tamaño del cuadro: ancho = 0.6 * qw, alto = 0.5 * qh
    int w = static_cast<int>(0.6 * qw + 0.5);
    int h = static_cast<int>(0.5 * qh + 0.5);

    // margen superior para no tocar justo el borde (ajusta si quieres)
    const int topMargin = 6;

    // si por algún caso el alto calculado no cabe, se recorta
    if (h > qh - 2 * topMargin) h = qh - 2 * topMargin;

    // horizontal: centrado dentro del cuadrante derecho
    const int x = qx + (qw - w) / 2;

    // vertical: PEGADO arriba del canvas (pero dentro del cuadrante)
    int y = topMargin;  // borde superior del canvas + margen
    // asegura no salir del cuadrante superior
    if (y + h > qh - topMargin) y = qh - topMargin - h;
    if (y < 0) y = 0;

    legendOverlay->setGeometry(x, y, w, h);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (plot && obj == plot->canvas()) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::LayoutRequest:
            positionLegendOverlay();
            break;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onOffsetSpinChanged(double val)
{
    // delta respecto al valor previo
    const double delta = val - yOffset;
    yOffset = val;

    // Si el timer está PARADO, ajustamos la curva visible sin avanzar fase
    if (!timer || !timer->isActive()) {
        if (!xData.isEmpty() && !yData.isEmpty()) {
            for (int i = 0; i < yData.size(); ++i)
                yData[i] += delta;   // desplazamos la curva ya renderizada
            curve->setSamples(xData, yData);
            plot->replot();
        }
        return;
    }

    // Si el timer está corriendo, el cambio se verá en el siguiente tick (50 ms).
    // Si quieres verlo inmediato además, descomenta estas 3 líneas:
    // for (int i = 0; i < yData.size(); ++i) yData[i] += delta;
    // curve->setSamples(xData, yData);
    // plot->replot();
}

void MainWindow::onItemSelected()
{
    // Aquí podrías cargar datos guardados o una imagen previa
}