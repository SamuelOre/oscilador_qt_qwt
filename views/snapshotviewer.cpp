#include "snapshotviewer.h"

#include <QListWidget>
#include <QLabel>
#include <QPushButton>      
#include <QFileDialog>     
#include <QStandardPaths>  
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QResizeEvent>
#include <QPixmap>

SnapshotViewer::SnapshotViewer(QWidget *parent)
    : QWidget(parent)
{
    auto *split = new QSplitter(this);

    // Lista a la izquierda
    list_ = new QListWidget;
    split->addWidget(list_);

    // Panel derecho: cabeceras + imagen con scroll
    auto *right = new QWidget;
    auto *rightCol = new QVBoxLayout(right);
    rightCol->setContentsMargins(0,0,0,0);
    rightCol->setSpacing(6);

    hdr1_ = new QLabel;                         // línea 1 (por item)
    hdr2_ = new QLabel("Widget Vista de Imagen"); // línea 2 (constante)
    hdr1_->setWordWrap(true);
    hdr2_->setWordWrap(true);

    // Botón Save a la derecha de los textos
    btnSave_ = new QPushButton("Save Image");
    btnSave_->setMinimumHeight(28);

    // Fila superior: [ (hdr1 + hdr2) |    Save Image   ]
    auto *headerTextCol = new QVBoxLayout;
    headerTextCol->setContentsMargins(0,0,0,0);
    headerTextCol->setSpacing(2);
    headerTextCol->addWidget(hdr1_);
    headerTextCol->addWidget(hdr2_);

    auto *headerRow = new QHBoxLayout;
    headerRow->setContentsMargins(0,0,0,0);
    headerRow->setSpacing(8);
    headerRow->addLayout(headerTextCol, /*stretch*/ 1);
    headerRow->addWidget(btnSave_,      /*stretch*/ 0, Qt::AlignRight);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    image_ = new QLabel;
    image_->setAlignment(Qt::AlignCenter);
    image_->setScaledContents(false); // escalamos manualmente con KeepAspectRatio
    scroll->setWidget(image_);

    rightCol->addLayout(headerRow);
    rightCol->addWidget(scroll, 1);

    split->addWidget(right);

    // Pesos del splitter
    split->setStretchFactor(0, 0); // lista
    split->setStretchFactor(1, 1); // panel derecho

    // Layout raíz
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->addWidget(split);

    connect(list_, &QListWidget::currentRowChanged,
            this, &SnapshotViewer::onRowChanged);
    connect(btnSave_, &QPushButton::clicked,          
            this, &SnapshotViewer::onSaveImage);
}

void SnapshotViewer::setData(const QStringList &titles,
                             const QVector<QPixmap> &images,
                             const QVector<double> &amps,
                             const QVector<double> &freqs)
{
    titles_ = titles;
    images_ = images;
    amps_   = amps;
    freqs_  = freqs;

    // Igualar tamaños por seguridad
    int m = titles_.size();
    m = qMin(m, images_.size());
    m = qMin(m, amps_.size());
    m = qMin(m, freqs_.size());
    titles_ = titles_.mid(0, m);
    images_.resize(m);
    amps_.resize(m);
    freqs_.resize(m);

    list_->clear();
    list_->addItems(titles_);

    if (m > 0) {
        list_->setCurrentRow(m - 1); // selecciona el último añadido
    } else {
        image_->clear();
        hdr1_->clear();
        // hdr2_ permanece fijo
        currentRow_ = -1;
    }
}

void SnapshotViewer::onRowChanged(int row)
{
    showIndex(row);
}

void SnapshotViewer::updateHeaders(int row)
{
    if (row < 0 || row >= titles_.size()
        || row >= amps_.size() || row >= freqs_.size())
    {
        hdr1_->clear();
        return;
    }

    QString itemName = titles_[row];

    const double A = amps_[row];
    const double F = freqs_[row];

    hdr1_->setText(QString("Imagen: %1, Amplitude: %2, Frequency: %3 Hz")
                   .arg(itemName)
                   .arg(A)
                   .arg(F));
    // hdr2_ ya contiene "Widget Vista de Imagen"
}

void SnapshotViewer::showIndex(int row)
{
    currentRow_ = row;

    if (row < 0 || row >= images_.size()) {
        image_->clear();
        hdr1_->clear();
        return;
    }

    updateHeaders(row);

    // Escalado seguro al tamaño disponible del viewport
    QWidget *viewport = image_->parentWidget();
    if (!viewport) {
        image_->setPixmap(images_[row]);
        return;
    }

    const QSize avail = viewport->size();
    if (avail.width() <= 1 || avail.height() <= 1) {
        image_->setPixmap(images_[row]);
        return;
    }

    QPixmap scaled = images_[row].scaled(avail, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    image_->setPixmap(scaled);
}

void SnapshotViewer::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (currentRow_ >= 0 && currentRow_ < images_.size()) {
        showIndex(currentRow_);
    }
}

void SnapshotViewer::onSaveImage()
{
    if (currentRow_ < 0 || currentRow_ >= images_.size())
        return;

    // Lo que ves en pantalla (si hay pixmap escalado), o el original
    QPixmap shown = image_->pixmap();
    QPixmap toSave = !shown.isNull() ? shown : images_[currentRow_];

    // Nombre sugerido: Item_X.png en la carpeta Imágenes del usuario
    QString base = (currentRow_ < titles_.size()) ? titles_[currentRow_] : QString("Item_%1").arg(currentRow_+1);
    base.replace('#', '_').replace(' ', '_');

    const QString defDir  = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString defName = defDir.isEmpty() ? QString("%1.png").arg(base)
                                             : QString("%1/%2.png").arg(defDir, base);

    QString file = QFileDialog::getSaveFileName(this,
                    tr("Guardar imagen"),
                    defName,
                    tr("PNG Image (*.png)"));

    if (file.isEmpty())
        return;

    if (!file.endsWith(".png", Qt::CaseInsensitive))
        file += ".png";

    toSave.save(file, "PNG");
}