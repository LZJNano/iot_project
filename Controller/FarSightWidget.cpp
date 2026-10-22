// FarSightWidget.cpp
#include "FarSightWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QCoreApplication>
#include <QDir>

FarSightWidget::FarSightWidget(QWidget *parent)
    : QWidget(parent),
      m_imagePath("assets/Far_Sight.png")
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // Try a few candidate locations so it works both during dev (next to .pro)
    // and when deployed (next to binary in bin/).
    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    candidates << m_imagePath
               << appDir + "/assets/Far_Sight.png"
               << appDir + "/../assets/Far_Sight.png"
               << appDir + "/../../assets/Far_Sight.png"
               << QDir::current().absoluteFilePath("assets/Far_Sight.png");
    for (int i = 0; i < candidates.size(); ++i) {
        if (m_pixmap.load(candidates[i])) {
            m_imagePath = candidates[i];
            break;
        }
    }
    setFixedSize(500, 406);
}

QSize FarSightWidget::sizeHint() const        { return QSize(500, 406); }
QSize FarSightWidget::minimumSizeHint() const { return QSize(500, 406); }
QSize FarSightWidget::maximumSizeHint() const { return QSize(500, 406); }

void FarSightWidget::setImagePath(const QString &path)
{
    m_imagePath = path;
    m_pixmap = QPixmap();
    m_pixmap.load(path);
    update();
}

void FarSightWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    // White background so transparent PNGs render predictably
    p.fillRect(rect(), Qt::white);
    if (!m_pixmap.isNull()) {
        // Center the pixmap; if larger than widget, draw top-left and let
        // widget clip to bounds.
        int x = (width()  - m_pixmap.width())  / 2;
        int y = (height() - m_pixmap.height()) / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        p.drawPixmap(x, y, m_pixmap);
    } else {
        p.setPen(Qt::red);
        p.drawText(rect(), Qt::AlignCenter,
                   QString("Far_Sight.png not found\ntried: %1").arg(m_imagePath));
    }

    // ---- LCD overlay rect ----
    // Geometry measured from Far_Sight.png in Figma: 228x123, top 22, left 20.
    // Only visible when a known LCD color was set.
    if (m_lcdColor.isValid()) {
        const QRect lcdRect(20, 22, 228, 123);
        p.fillRect(lcdRect, m_lcdColor);
        // Optional thin border so the rect is visible against same-color background
        p.setPen(QPen(QColor(0, 0, 0, 80), 1));
        p.drawRect(lcdRect);
    }
}

void FarSightWidget::setLcdColor(const QString &color)
{
    m_lcdColor = namedColor(color);
    update();   // schedule repaint
}

QColor FarSightWidget::namedColor(const QString &name) const
{
    // Match src/lcd.h device-side color macros 1:1 (pure RGB).
    // Qt's named colors happen to be the same for these, but we map
    // explicitly to avoid future drift.
    if (name == "red")    return QColor(0xFF, 0x00, 0x00);
    if (name == "green")  return QColor(0x00, 0xFF, 0x00);
    if (name == "blue")   return QColor(0x00, 0x00, 0xFF);
    if (name == "white")  return QColor(0xFF, 0xFF, 0xFF);
    if (name == "black")  return QColor(0x00, 0x00, 0x00);
    if (name == "yellow") return QColor(0xFF, 0xFF, 0x00);
    return QColor();      // invalid => hidden
}