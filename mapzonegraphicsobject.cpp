#include "mapzonegraphicsobject.h"

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
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QGraphicsSceneMouseEvent>

#include <limits>

#include "utils.h"
#include "minimalscopeprofiler.h"

MapZoneGraphicsObject::MapZoneGraphicsObject()
{
}

MapZoneGraphicsObject::~MapZoneGraphicsObject()
{
    for (auto [id, path] : m_Paths)
    {
        delete path;
    }

    delete m_UnfinnishedPath;
}

void MapZoneGraphicsObject::SetZoomRange(int min, int max)
{
    m_MinZoom = min;
    m_MaxZoom = max;
}

void MapZoneGraphicsObject::Clear()
{
    for (auto& [id, path] : m_Paths)
    {
        delete path;
    }

    m_Paths.clear();

    delete m_UnfinnishedPath;
    m_UnfinnishedPath = nullptr;
}

const QString& MapZoneGraphicsObject::GetMapPath() const
{
    return m_MapPath;
}

QRectF MapZoneGraphicsObject::boundingRect() const
{
    int height = m_MapImage->height();
    int width = m_MapImage->width();

    return QRectF(0, 0, width, height);
}

void MapZoneGraphicsObject::paint(QPainter *painter,
           const QStyleOptionGraphicsItem *option,
           QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing);

    //float scale = 1.0f;
    //painter->scale(scale, scale);

    for (auto [id, path] : m_Paths)
    {
        if (id != m_EditedZoneId)
        {
            QColor color = m_Selection.contains(id) ? ms_SelectionColor : IDToColor(id);
            color.setAlpha(m_Alpha);

            SetupPainterForColor(*painter, color, true);
            painter->drawPath(*path);
        }
    }

    if (m_UnfinnishedPath != nullptr)
    {
        if (m_EditMode == EditMode::Zone)
        {
            SetupPainterForColor(*painter, ms_CreationColor, true);
        }
        else if (m_EditMode == EditMode::Measure)
        {
            SetupPainterForColor(*painter, ms_MeasureColor, false);
        }

        painter->drawPath(*m_UnfinnishedPath);

        qreal size = 5.0;
        qreal halfSize = size * 0.5;

        QPainterPath::Element start = m_UnfinnishedPath->elementAt(0);
        painter->drawEllipse(start.x - halfSize, start.y - halfSize, size, size);

        if (m_EditMode == EditMode::Zone)
        {
            size = 2.0;
            halfSize = size * 0.5;

            SetupPainterForColor(*painter, ms_EditPointColor, true);
            int elementCount = m_UnfinnishedPath->elementCount();
            for (int i = 1; i < elementCount; ++i)
            {
                if (m_IsShiftPressed && i == elementCount - 1)
                {
                    size = 10.0;
                    halfSize = size * 0.5;

                    SetupPainterForColor(*painter, ms_TeleportColor, true);
                }

                QPainterPath::Element element = m_UnfinnishedPath->elementAt(i);
                painter->drawEllipse(element.x - halfSize, element.y - halfSize, size, size);
            }

            size = 10.0;
            halfSize = size * 0.5;

            if (m_ClosestElement != -1)
            {
                SetupPainterForColor(*painter, ms_TeleportColor, true);
                QPainterPath::Element element = m_UnfinnishedPath->elementAt(m_ClosestElement);
                painter->drawEllipse(element.x - halfSize, element.y - halfSize, size, size);
            }

            if (m_ClosestPositionOnSegmentElement != -1)
            {
                SetupPainterForColor(*painter, ms_InsertColor, true);
                painter->drawEllipse(m_ClosestPointOnSegment.x() - halfSize, m_ClosestPointOnSegment.y() - halfSize, size, size);
            }
        }
    }
}

void MapZoneGraphicsObject::SetupPainterForColor(QPainter& painter, const QColor& color, bool fill)
{
    QLinearGradient gradient(0, 0, 0, 100);
    QColor fillColor = fill ? color : QColor(0, 0, 0, 0);
    gradient.setColorAt(0.0, fillColor);
    gradient.setColorAt(1.0, fillColor);
    painter.setBrush(gradient);

    int penWidth = 5;

    painter.setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void MapZoneGraphicsObject::OnFocusOut()
{
    m_ClosestPositionOnSegmentElement = -1;
    m_ClosestElement = -1;
}

void MapZoneGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    super::mousePressEvent(event);

    setFocus();

    const bool isControlPressed = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
    const bool isAltPressed = QApplication::keyboardModifiers().testFlag(Qt::AltModifier);
    const bool isShiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

    if (m_EditMode != EditMode::Measure && isShiftPressed)
    {
        SetEditMode(EditMode::Zone);
    }

    if (m_EditMode != EditMode::Zone && isAltPressed)
    {
        SetEditMode(EditMode::Measure);
    }

    QPoint mousePoint = GetScaledMousePosition();

    if(event->buttons() & Qt::LeftButton)
    {
        if (m_EditAllowed)
        {
            if (m_UnfinnishedPath == nullptr)
            {
                if (m_EditMode == EditMode::Zone)
                {
                    m_UnfinnishedPath = new QPainterPath(mousePoint);
                    m_UnfinnishedPath->setFillRule(Qt::WindingFill);
                    m_EditedZoneColorId = CreateNextColorID();
                    m_EditedZoneId = ColorToID(m_EditedZoneColorId);
                }
                else if (m_EditMode == EditMode::Measure)
                {
                    m_UnfinnishedPath = &m_MeasureDistancePath;
                    m_UnfinnishedPath->clear();
                    m_UnfinnishedPath->moveTo(mousePoint);
                }

                update();
            }
            else
            {
                if (m_EditMode == EditMode::Measure)
                {
                    m_UnfinnishedPath->lineTo(mousePoint);
                }
                else if (isControlPressed)
                {
                    if (m_ClosestElement != -1)
                    {
                        MoveElementToMouse(*m_UnfinnishedPath, m_ClosestElement);
                    }
                }
                else if (isAltPressed)
                {
                    if (m_ClosestPositionOnSegmentElement != -1)
                    {
                        InsertElementAfterindex(*m_UnfinnishedPath, m_ClosestPositionOnSegmentElement, m_ClosestPointOnSegment);
                    }
                }
                else if (isShiftPressed)
                {
                    m_UnfinnishedPath->lineTo(mousePoint);
                }

                if (m_EditMode == EditMode::Measure)
                {
                    emit MeasureUpdated((float)m_UnfinnishedPath->length());
                }

                update();
            }
        }
        else
        {
            m_IsUpdatingSelection = true;

            bool selectionChanged = false;

            if (!isControlPressed)
            {
                selectionChanged = m_Selection.count() > 0;
                m_Selection.clear();
            }

            if (selectionChanged)
            {
                emit SelectionChanged(m_Selection);
                selectionChanged = false;
            }

            for (auto [id, path] : m_Paths)
            {
                if (path->contains(mousePoint))
                {
                    m_Selection.append(id);
                    selectionChanged = true;
                }
            }

            if (selectionChanged)
            {
                emit SelectionChanged(m_Selection);
            }

            m_IsUpdatingSelection = false;

            update();
        }
    }

    if(event->buttons() & Qt::RightButton)
    {
        ClosePath();

        m_Selection.clear();

        emit SelectionChanged(m_Selection);

        for (auto [id, path] : m_Paths)
        {
            if (path->contains(mousePoint))
            {
                m_Selection.append(id);
                break;
            }
        }

        emit SelectionChanged(m_Selection);

        if (!m_Selection.isEmpty())
        {
            QPoint globalPosition = QCursor::pos();
            emit OnRightClickOnZone(globalPosition, m_Selection.first());
            setFocus();
        }
    }
}

void MapZoneGraphicsObject::forwardKeyPressEvent(QKeyEvent* event)
{
    super::keyPressEvent(event);

    if (event->key() == Qt::Key_Shift)
    {
        if (m_EditMode != EditMode::Measure)
        {
            SetEditMode(EditMode::Zone);
        }

        m_IsShiftPressed = true;

        update();
    }
    else if (event->key() == Qt::Key_Alt)
    {
        if (m_EditMode != EditMode::Zone)
        {
            SetEditMode(EditMode::Measure);
        }
        else if (m_UnfinnishedPath != nullptr)
        {
            SetEditAllowed(true);

            m_ClosestPositionOnSegmentElement = -1;

            QVector2D position;
            FindClosestPositionOnSegmentFromMouse(*m_UnfinnishedPath, m_ClosestPositionOnSegmentElement, position);
            m_ClosestPointOnSegment = position.toPoint();

            update();
        }
    }
    else if (event->key() == Qt::Key_R)
    {
        const bool isControlPressed = QApplication::keyboardModifiers().testAnyFlags(Qt::ControlModifier);
        if (isControlPressed && m_EditMode == EditMode::Zone && m_UnfinnishedPath != nullptr && m_UnfinnishedPath->elementCount() > 1)
        {
            RemoveLastLine(*m_UnfinnishedPath);
            m_ClosestElement = FindClosestElementFromMouseIndex(*m_UnfinnishedPath);
            update();
        }
    }
    else if (event->key() == Qt::Key_D)
    {
        const bool isControlPressed = QApplication::keyboardModifiers().testAnyFlags(Qt::ControlModifier);
        if (isControlPressed && m_EditMode == EditMode::Zone && m_UnfinnishedPath != nullptr && m_UnfinnishedPath->elementCount() > 1)
        {
            m_ClosestElement = FindClosestElementFromMouseIndex(*m_UnfinnishedPath);
            RemoveElement(*m_UnfinnishedPath, m_ClosestElement);
            m_ClosestElement = FindClosestElementFromMouseIndex(*m_UnfinnishedPath);
            update();
        }
    }
    else if (event->key() == Qt::Key_Control)
    {
        if (m_EditMode == EditMode::Zone && m_UnfinnishedPath != nullptr)
        {
            SetEditAllowed(true);

            int i = FindClosestElementFromMouseIndex(*m_UnfinnishedPath);
            if (i != m_ClosestElement)
            {
                m_ClosestElement = i;

                update();
            }
        }
    }
}

void MapZoneGraphicsObject::forwardKeyReleaseEvent(QKeyEvent *event)
{
    super::keyReleaseEvent(event);

    if (event->key() == Qt::Key_Shift)
    {
        if (m_EditMode == EditMode::Zone)
        {
            if (m_UnfinnishedPath == nullptr)
            {
                SetEditMode(EditMode::None);
            }
            else
            {
                SetEditAllowed(false);
            }
        }

        m_IsShiftPressed = false;
        update();
    }
    else if (event->key() == Qt::Key_Alt)
    {
        if (m_EditMode == EditMode::Measure)
        {
            SetEditAllowed(false);
        }
        else if (m_EditMode == EditMode::Zone)
        {
            if (m_UnfinnishedPath == nullptr)
            {
                SetEditMode(EditMode::None);
            }
            else
            {
                SetEditAllowed(false);
            }
        }

        m_ClosestPositionOnSegmentElement = -1;
        update();
    }
    else if (event->key() == Qt::Key_Control)
    {
        if (m_EditMode == EditMode::Zone)
        {
            if (m_UnfinnishedPath == nullptr)
            {
                SetEditMode(EditMode::None);
            }
            else
            {
                SetEditAllowed(false);
            }
        }

        m_ClosestElement = -1;
        update();
    }
    else if (event->key() == Qt::Key_Escape)
    {
        ClosePath();
    }
}

void MapZoneGraphicsObject::SetEditMode(EditMode mode)
{
    m_EditMode = mode;

    SetEditAllowed(mode != EditMode::None);

    if (mode != EditMode::Zone)
    {
        m_ClosestElement = -1;
    }

    emit OnEditModeChanged(m_EditMode);
}

void MapZoneGraphicsObject::SetEditAllowed(bool allowed)
{
    m_EditAllowed = allowed;

    emit OnEditChanged(m_EditAllowed);
}

void MapZoneGraphicsObject::SetZoom(int zoom)
{
    m_Zoom = zoom;
}

void MapZoneGraphicsObject::SetAlpha(int alpha)
{
    m_Alpha = alpha;
    update();
}

int MapZoneGraphicsObject::GetAlpha() const
{
    return m_Alpha;
}

void MapZoneGraphicsObject::ClosePath()
{
    if (m_EditMode == EditMode::Zone && m_UnfinnishedPath != nullptr)
    {
        bool editExistingPath = (m_Paths.find(m_EditedZoneId) != m_Paths.end());

        m_UnfinnishedPath->closeSubpath();

        int id = ColorToID(m_EditedZoneColorId);
        m_Paths[id] = m_UnfinnishedPath;
        m_UnfinnishedPath = nullptr;

        if (!editExistingPath)
        {
            emit ZoneCreated(id);
        }

        m_EditedZoneId = -1;
    }
    else if (m_EditMode == EditMode::Measure)
    {
        m_UnfinnishedPath = nullptr;
    }

    SetEditMode(EditMode::None);

    update();
}

QColor MapZoneGraphicsObject::CreateNextColorID()
{
    int red = m_MaxColorID.red();

    if (red < 255)
    {
        m_MaxColorID.setRed(red + 1);
    }
    else
    {
        m_MaxColorID.setRed(0);
        m_MaxColorID.setGreen(m_MaxColorID.green() + 1);
    }

    return m_MaxColorID;
}

int MapZoneGraphicsObject::ColorToID(const QColor& color) const
{
    return color.red() + color.green() * 1000;
}

QColor MapZoneGraphicsObject::IDToColor(int id) const
{
    QColor color(0, 0, 0, 122);

    color.setGreen(id / 1000);
    color.setRed(id - color.green() * 1000);

    return color;
}

void MapZoneGraphicsObject::DeleteZone(int zoneId)
{
    auto iterator = m_Paths.find(zoneId);
    if (iterator != m_Paths.end())
    {
        QPainterPath* path = iterator->second;
        m_Paths.erase(iterator);
        delete path;
    }
}

void MapZoneGraphicsObject::SetSelected(int id)
{
    if (!m_IsUpdatingSelection)
    {
        m_Selection.clear();
        m_Selection.append(id);

        emit SelectionChanged(m_Selection);

        update();
    }
}

void MapZoneGraphicsObject::SetSelected(const QList<int> selection)
{
    if (!m_IsUpdatingSelection)
    {
        m_Selection = selection;

        emit SelectionChanged(m_Selection);

        update();
    }
}

void MapZoneGraphicsObject::DrawMap()
{
    prepareGeometryChange();
}

void MapZoneGraphicsObject::EditZone(int zoneId)
{
    if (m_UnfinnishedPath == nullptr)
    {
        SetSelected(QList<int>());

        SetEditMode(EditMode::Zone);

        const bool isEditModifierPressed = QApplication::keyboardModifiers().testAnyFlags(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
        SetEditAllowed(isEditModifierPressed);

        m_UnfinnishedPath = m_Paths[zoneId];

        m_EditedZoneId = zoneId;
        m_EditedZoneColorId = IDToColor(zoneId);

        RemoveLastLine(*m_UnfinnishedPath);

        update();

        setFocus();
    }
}

void MapZoneGraphicsObject::RemoveLastLine(QPainterPath& path) const
{
    if (path.elementCount() > 1)
    {
        QPainterPath editedPath;
        QPainterPath::Element element = path.elementAt(0);
        editedPath.moveTo(element.x, element.y);

        for (int i = 1; i < m_UnfinnishedPath->elementCount() - 1; ++i)
        {
            element = path.elementAt(i);
            editedPath.lineTo(element.x, element.y);
        }

        path = editedPath;
    }
}

void MapZoneGraphicsObject::RemoveElement(QPainterPath& path, int elementIndex) const
{
    if (path.elementCount() > 2)
    {
        QPainterPath editedPath;
        QPainterPath::Element element = path.elementAt(0);
        editedPath.moveTo(element.x, element.y);

        for (int i = 1; i < m_UnfinnishedPath->elementCount(); ++i)
        {
            if (i != elementIndex)
            {
                element = path.elementAt(i);
                editedPath.lineTo(element.x, element.y);
            }
        }

        path = editedPath;
    }
}

void MapZoneGraphicsObject::InsertElementAfterindex(QPainterPath& path, int indexBeforeinsert, QPoint position) const
{
    if (path.elementCount() > 1)
    {
        bool inserted = false;

        QPainterPath editedPath;
        QPainterPath::Element element = path.elementAt(0);
        editedPath.moveTo(element.x, element.y);

        for (int i = 1; i < m_UnfinnishedPath->elementCount(); ++i)
        {
            if (inserted || i <= indexBeforeinsert)
            {
                element = path.elementAt(i);
                editedPath.lineTo(element.x, element.y);
            }
            else
            {
                editedPath.lineTo(position.x(), position.y());
                inserted = true;
                i--;
            }
        }

        path = editedPath;
    }
}

void MapZoneGraphicsObject::MoveElementToMouse(QPainterPath& path, int i) const
{
    if (path.elementCount() > i)
    {
        QPoint mousePosition = GetScaledMousePosition();
        path.setElementPositionAt(i, mousePosition.x(), mousePosition.y());
    }
}

int MapZoneGraphicsObject::FindClosestElementFromMouseIndex(const QPainterPath& path) const
{
    int index = -1;

    QPoint mousePosition = GetScaledMousePosition();

    int minLength = std::numeric_limits<int>::max();

    for (int i = 0; i < path.elementCount(); ++i)
    {
        QPainterPath::Element element = path.elementAt(i);
        QPoint elementPoint(element.x, element.y);

        int currentLength = (mousePosition - elementPoint).manhattanLength();
        if (currentLength < minLength)
        {
            minLength = currentLength;
            index = i;
        }
    }

    return index;
}

void MapZoneGraphicsObject::FindClosestPositionOnSegmentFromMouse(const QPainterPath& path, int& segmentStartIndex, QVector2D& position) const
{
    int elementCount = path.elementCount();
    if (elementCount > 1)
    {
        QVector2D mousePosition(GetScaledMousePosition());

        int minLength = std::numeric_limits<int>::max();

        QPainterPath::Element start = path.elementAt(0);
        QVector2D startPoint(start.x, start.y);

        for (int i = 0; i < path.elementCount() - 1; ++i)
        {
            QPainterPath::Element end = path.elementAt(i + 1);
            QVector2D endPoint(end.x, end.y);

            if (startPoint != endPoint)
            {
                QVector2D segment = endPoint - startPoint;
                QVector2D direction = segment.normalized();

                float dotFromStart = QVector2D::dotProduct(direction, mousePosition - startPoint);
                float dotFromEnd = QVector2D::dotProduct(-direction, mousePosition - endPoint);

                const bool isBetweenPoints = (dotFromStart > 0 && dotFromEnd > 0);
                if (isBetweenPoints)
                {
                    float currentLength = mousePosition.distanceToLine(startPoint, direction);
                    if (currentLength < minLength && currentLength > 0.0f)
                    {
                        QLineF normalLine(startPoint.toPointF(), endPoint.toPointF());
                        normalLine = normalLine.normalVector();

                        QVector2D normal(normalLine.dx(), normalLine.dy());
                        normal.normalize();

                        position = startPoint + direction * qBound(0.0f, dotFromStart, segment.length());
                        minLength = currentLength;
                        segmentStartIndex = i;
                    }
                }

            }

            startPoint = endPoint;
        }
    }
}

float MapZoneGraphicsObject::GetScale() const
{
    return (float)m_Zoom / 100.0f;
}

QPoint MapZoneGraphicsObject::GetScaledMousePosition() const
{
    QGraphicsView* view = this->scene()->views()[0];
    QPoint globalPosition = QCursor::pos();
    QPoint viewPosition = view->mapFromGlobal(globalPosition);
    QPoint scenePosition = view->mapToScene(viewPosition).toPoint();
    return mapFromScene(scenePosition).toPoint();
}

QRect MapZoneGraphicsObject::GetSelectionBoundingBox() const
{
    QRect boundingBox(-1, -1, -1, -1);

    for (int zoneId : m_Selection)
    {
        const QPainterPath* path = m_Paths.at(zoneId);
        for (int i = 0; i < path->elementCount(); ++i)
        {
            const QPainterPath::Element& element = path->elementAt(i);
            if (boundingBox.left() < 0 || boundingBox.left() > element.x)
            {
                boundingBox.setLeft(element.x);
            }

            if (boundingBox.right() < 0 || boundingBox.right() < element.x)
            {
                boundingBox.setRight(element.x);
            }

            if (boundingBox.top() < 0 || boundingBox.top() < element.y)
            {
                boundingBox.setTop(element.y);
            }

            if (boundingBox.bottom() < 0 || boundingBox.bottom() > element.y)
            {
                boundingBox.setBottom(element.y);
            }
        }
    }

    return m_View->mapFromScene(boundingBox).boundingRect();
}
