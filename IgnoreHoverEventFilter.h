#pragma once

#include <QObject>
#include <QEvent>

class IgnoreHoverEventFilter : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        Q_UNUSED(obj)
        const QEvent::Type type = event->type();
        return (type == QEvent::HoverEnter || type == QEvent::MouseMove);
    }
};
