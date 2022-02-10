#pragma once

#include <QLabel>
#include <QGraphicsView>
#include <QPixmap>
#include <QPainterPath>

#include "saveutils.h"

#include "mapzonegraphicsobject.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MapZoneGraphicsObject;

class DrawableMap : public QGraphicsView
{
private:
    Q_OBJECT

    using super = QGraphicsView;

public:
    enum class EditMode : short
    {
        Zone,
        Measure,
        None
    };

public:
    explicit DrawableMap(QWidget *parent = nullptr);
    ~DrawableMap();

    void SetImage(const QString& imagePath);
    void SetZoom(int zoom);
    void SetDragEnabled(bool enabled);
    void ZoomOnMouse(int zoom);
    void SetAlpha(int alpha);
    int GetAlpha() const;
    void DeleteZone(int zoneId);
    void EditZone(int zoneId);
    const QString& GetMapPath() const;
    void SetZoomRange(int min, int max);
    void SetZoomIncrement(int increment);

    void Initialize(Ui::MainWindow* ui);

    void SetSelected(int zoneId);
    void SetSelected(const QList<int>& selection);

    template<typename SaveStreamType>
    void Save(SaveStreamType& saveStream, int version) const
    {
        m_MapZoneGraphicsObject->Save<SaveStreamType>(saveStream, version);
    }

    template<typename LoadStreamType>
    void Load(LoadStreamType& loadStream)
    {
        m_MapZoneGraphicsObject->Load<LoadStreamType>(loadStream);
    }

    MapZoneGraphicsObject* GetMapZoneGraphicsObject() const { return m_MapZoneGraphicsObject; }

signals:
    void OnEditChanged(bool editStarted);
    void OnEditModeChanged(DrawableMap::EditMode editmode);
    void ZoneCreated(int id);
    void SelectionChanged(const QList<int>& selection);
    void MeasureUpdated(float distance);
    void ZoomChanged(int zoom);

protected:
    bool eventFilter(QObject *obj, QEvent *evt) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void DrawMap();
    float GetScale() const;
    QPoint GetScaledMousePosition() const;
    void ZoomExtend();

private:
    QColor m_MaxColorID;
    std::map<int, QPainterPath*> m_Paths;

    QPainterPath m_MeasureDistancePath;
    QPixmap m_MapImage;
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

    QGraphicsPixmapItem* m_PixmapItem { nullptr };
    MapZoneGraphicsObject* m_MapZoneGraphicsObject { nullptr };

    int m_MinZoom { 1 };
    int m_MaxZoom { 100 };
    int m_ZoomIncrement { 20 };
};
