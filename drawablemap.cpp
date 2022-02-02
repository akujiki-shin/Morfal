#include "drawablemap.h"

#include <QApplication>
#include <QPainterPath>
#include <QPainter>
#include <QEvent>
#include <QLayout>
#include <QGridLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVector2D>
#include <QLineF>
#include <QMenu>
#include <QTransform>

#include <limits>

#include "utils.h"

#include "minimalscopeprofiler.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>

DrawableMap::DrawableMap(QWidget *parent)
    : super(parent)
{
    installEventFilter(this);

    QGraphicsScene* scene = new QGraphicsScene();
    setScene(scene);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);

    m_PixmapItem = new QGraphicsPixmapItem();
    m_PixmapItem->setTransformationMode(Qt::SmoothTransformation);
    scene->addItem(m_PixmapItem);

    m_MapZoneGraphicsObject = new MapZoneGraphicsObject();
    m_MapZoneGraphicsObject->SetMapImage(&m_MapImage);

    scene->addItem(m_MapZoneGraphicsObject);

    m_MapZoneGraphicsObject->installSceneEventFilter(m_MapZoneGraphicsObject);

    viewport()->setCursor(Qt::ArrowCursor);

    setEnabled(true);

    m_PixmapItem->setZValue(0);
    m_MapZoneGraphicsObject->setZValue(1);
}

DrawableMap::~DrawableMap()
{
    for (auto [id, path] : m_Paths)
    {
        delete path;
    }

    delete m_UnfinnishedPath;
    delete m_PixmapItem;
}

void DrawableMap::SetImage(const QString& imagePath)
{
    profile();

    m_MapPath = imagePath;

    m_MapImage.load(m_MapPath);
    m_PixmapItem->setPixmap(m_MapImage);

    centerOn(parentWidget()->width() * 0.5f, parentWidget()->height() * 0.5f);

    m_MapZoneGraphicsObject->update();
}

void DrawableMap::SetZoomRange(int min, int max)
{
    m_MinZoom = min;
    m_MaxZoom = max;
}

void DrawableMap::SetZoomIncrement(int increment)
{
    m_ZoomIncrement = increment;
}

const QString& DrawableMap::GetMapPath() const
{
    return m_MapPath;
}

bool DrawableMap::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Wheel)
    {
        if (obj == this || obj == m_MapZoneGraphicsObject)
        {
            QWheelEvent* wheelEvent = (QWheelEvent*)event;
            int delta =  (wheelEvent->angleDelta().y() > 0 ? m_ZoomIncrement : -m_ZoomIncrement);

            const bool isControlPressed = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
            const bool isShiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

            if (isControlPressed)
            {
                delta *= 2.0f;
            }

            if (isShiftPressed)
            {
                delta *= 5.0f;
            }

            ZoomOnMouse(m_Zoom + delta);
        }

        return true;
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = (QMouseEvent*) event;
        if (mouseEvent->buttons() & Qt::MiddleButton)
        {
            SetZoom(100);
        }
    }
    else if (event->type() == QEvent::FocusOut)
    {
        m_MapZoneGraphicsObject->OnFocusOut();
    }

    return super::eventFilter(obj, event);
}

void DrawableMap::ZoomOnMouse(int zoom)
{
    QPoint globalPosition = QCursor::pos();
    QPoint viewPosition = mapFromGlobal(globalPosition);
    QPoint scenePosition = mapToScene(viewPosition).toPoint();

    SetZoom(zoom);

    QPointF deltaViewport = viewPosition - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
    QPointF viewportCenter = mapFromScene(scenePosition) - deltaViewport;
    centerOn(mapToScene(viewportCenter.toPoint()));
}

void DrawableMap::SetZoom(int zoom)
{
    m_Zoom = qBound(m_MinZoom, zoom, m_MaxZoom);
    emit ZoomChanged(m_Zoom);

    m_MapZoneGraphicsObject->SetZoom(m_Zoom);

    DrawMap();
}

void DrawableMap::SetDragEnabled(bool enabled)
{
    QGraphicsView::DragMode dragMode = enabled ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag;
    setDragMode(dragMode);
}

void DrawableMap::DrawMap()
{
    float ratio = GetScale();

    QTransform matrix;
    matrix.scale(ratio, ratio);

    setTransform(matrix);
}

float DrawableMap::GetScale() const
{
    return (float)m_Zoom / 100.0f;
}

QPoint DrawableMap::GetScaledMousePosition() const
{
    return mapFromGlobal(QCursor::pos()) * (1.0f / GetScale());
}
