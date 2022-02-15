#pragma once

#include <QGraphicsObject>
#include <QPainterPath>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>

#include "saveutils.h"

class QPaintEvent;

class MapZoneGraphicsObject : public QGraphicsObject
{
private:
    Q_OBJECT

    using super = QGraphicsObject;

public:
    enum class EditMode : short
    {
        Zone,
        Measure,
        None
    };

public:
    MapZoneGraphicsObject();
    ~MapZoneGraphicsObject();

    QRectF boundingRect() const override;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    //void SetImage(const QString& imagePath);
    void SetSelected(int id);
    void SetSelected(const QList<int> selection);
    int GetSelectedZoneCount() const;
    void SetZoom(int zoom);
    void SetAlpha(int alpha);
    int GetAlpha() const;
    void DeleteZone(int zoneId);
    const QString& GetMapPath() const;
    void EditZone(int zoneId);
    void SetZoomRange(int min, int max);

    void SetView(QGraphicsView* view) { m_View = view; }
    void SetMapImage(QPixmap* map) { m_MapImage = map;  prepareGeometryChange(); }

    template<typename SaveStreamType>
    void Save(SaveStreamType& saveStream, int version) const;

    template<typename LoadStreamType>
    void Load(LoadStreamType& loadStream);

    void forwardKeyPressEvent(QKeyEvent *event);
    void forwardKeyReleaseEvent(QKeyEvent *event);

    void OnFocusOut();

    bool IsUpdatingSelection() const { return m_IsUpdatingSelection; }

    QRect GetSelectionBoundingBox() const;

signals:
    void OnEditChanged(bool editStarted);
    void OnEditModeChanged(MapZoneGraphicsObject::EditMode editmode);
    void ZoneCreated(int id);
    void SelectionChanged(const QList<int>& selection);
    void MeasureUpdated(float distance);
    void OnRightClickOnZone(QPoint globalMousePosition, QList<int> zoneIds);
    void ZoomChanged(int zoom);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void Clear();
    void ClosePath();
    void SetEditAllowed(bool allowed);
    QColor CreateNextColorID();
    int ColorToID(const QColor& color) const;
    QColor IDToColor(int id) const;
    void SetupPainterForColor(QPainter& painter, const QColor& color, bool fill);
    void DrawMap();
    void SetEditMode(EditMode mode);
    int FindClosestElementFromMouseIndex(const QPainterPath& path) const;
    void FindClosestPositionOnSegmentFromMouse(const QPainterPath& path, int& segmentStartIndex, QVector2D& position) const;
    void RemoveLastLine(QPainterPath& path) const;
    void RemoveElement(QPainterPath& path, int elementIndex) const;
    void InsertElementAfterindex(QPainterPath& path, int indexBeforeinsert, QPoint position) const;
    void MoveElementToMouse(QPainterPath& path, int i) const;
    float GetScale() const;
    QPoint GetScaledMousePosition() const;

private:
    static const constexpr QColor ms_SelectionColor { 64, 115, 208 };
    static const constexpr QColor ms_CreationColor { 64, 115, 255, 122 };
    static const constexpr QColor ms_MeasureColor { 255, 127, 39, 200 };
    static const constexpr QColor ms_TeleportColor { 55, 205, 75, 200 };
    static const constexpr QColor ms_InsertColor { 185, 85, 175, 200 };
    static const constexpr QColor ms_EditPointColor { 127, 127, 127, 200 };
    static const constexpr QColor ms_NeutralColor { 0, 0, 0 };

private:
    QColor m_MaxColorID;
    std::map<int, QPainterPath*> m_Paths;

    QPainterPath m_MeasureDistancePath;
    QPixmap* m_MapImage { nullptr };
    int m_Zoom { 100 };
    QPainterPath* m_UnfinnishedPath { nullptr };
    bool m_EditAllowed { false };
    bool m_IsShiftPressed { false };
    QList<int> m_Selection;
    int m_Alpha { 122 };
    bool m_IsUpdatingSelection { false };
    EditMode m_EditMode{ EditMode::None };
    QString m_MapPath{""};
    QColor m_EditedZoneColorId;
    int m_EditedZoneId{ -1 };
    int m_ClosestElement{ -1 };
    int m_ClosestPositionOnSegmentElement{ -1 };
    QPoint m_ClosestPointOnSegment;

    QGraphicsView* m_View { nullptr };
    QGraphicsPixmapItem* m_PixmapItem { nullptr };

    int m_MinZoom { 1 };
    int m_MaxZoom { 100 };
};

template<typename SaveStreamType>
void MapZoneGraphicsObject::Save(SaveStreamType& saveStream, int version) const
{
    using namespace SaveUtils;
    using ParentScope = typename ParentSaveScope<SaveStreamType>::type;

    SaveUtils::Save(saveStream, "Version", version);
    SaveUtils::SaveAsByte(saveStream, "m_ColorID", m_MaxColorID);
    SaveUtils::Save(saveStream, "m_Alpha", m_Alpha);

    ParentScope pathParent;
    SaveUtils::Save(saveStream, "m_PathsSize", (int)m_Paths.size());
    for (const auto& [id, path] : m_Paths)
    {
        SaveStreamType* pathStream = OpenSaveScope(saveStream);

        SaveUtils::Save(*pathStream, "id", id);

        SaveUtils::SaveAsByte(*pathStream, "path", *path);

        SaveScope(pathParent, *pathStream);
        CloseSaveScope(pathStream);
    }

    SaveUtils::SaveParent(saveStream, "m_Paths", pathParent);

    SaveUtils::Save(saveStream, "horizontalScrollBar", m_View->horizontalScrollBar()->value());
    SaveUtils::Save(saveStream, "verticalScrollBar", m_View->verticalScrollBar()->value());
}

template<typename LoadStreamType>
void MapZoneGraphicsObject::Load(LoadStreamType& loadStream)
{
    using namespace SaveUtils;

    using ParentScope = typename ParentSaveScope<LoadStreamType>::type;
    using LoadScopeObject = typename LoadScopeObject<LoadStreamType>::type;

    Clear();

    int version;
    SaveUtils::Load(loadStream, "Version", version);

    SaveUtils::LoadFromByte(loadStream, "m_ColorID", m_MaxColorID);

    SaveUtils::Load(loadStream, "m_Alpha", m_Alpha);

    ParentScope pathParent = LoadParent(loadStream, "m_Paths");
    int pathCount;
    SaveUtils::Load(loadStream, "m_PathsSize", pathCount);
    for (int i = 0; i < pathCount; ++i)
    {
        LoadScopeObject pathScope;
        LoadStreamType& pathStream = AsStream(loadStream, pathScope, pathParent, i);

        int id;
        QPainterPath* path = new QPainterPath();

        SaveUtils::Load(pathStream, "id", id);
        SaveUtils::LoadFromByte(pathStream, "path", *path);

        m_Paths[id] = path;
    }

    int horizontalScrollBar;
    int verticalScrollBar;
    SaveUtils::Load(loadStream, "horizontalScrollBar", horizontalScrollBar);
    SaveUtils::Load(loadStream, "verticalScrollBar", verticalScrollBar);

    m_View->horizontalScrollBar()->setValue(horizontalScrollBar);
    m_View->verticalScrollBar()->setValue(verticalScrollBar);
}
