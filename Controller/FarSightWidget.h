// FarSightWidget.h - custom widget that displays assets/Far_Sight.png
// plus an LCD overlay rect that mirrors the device's LCD fill color.
// Size is fixed at 500x406, non-shrinkable, non-stretchable.
#ifndef FARSIGHTWIDGET_H
#define FARSIGHTWIDGET_H

#include <QtGui/QWidget>
#include <QPixmap>
#include <QColor>

class FarSightWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FarSightWidget(QWidget *parent = 0);

    // Fixed geometry contract
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    QSize maximumSizeHint() const;

    // Image path (relative to app dir, or absolute). Default: assets/Far_Sight.png
    void setImagePath(const QString &path);

    // LCD overlay state. color is one of:
    //   "red" / "green" / "blue" / "white" / "black" / "yellow"  -> opaque rect
    //   empty / unknown                                       -> hidden (transparent)
    void setLcdColor(const QString &color);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QColor  namedColor(const QString &name) const;

    QPixmap m_pixmap;
    QString m_imagePath;
    QColor  m_lcdColor;   // invalid => hidden
};

#endif // FARSIGHTWIDGET_H