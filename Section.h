/*
    Elypson/qt-collapsible-section
    (c) 2016 Michael A. Voelkel - michael.alexander.voelkel@gmail.com
    This file is part of Elypson/qt-collapsible section.
    Elypson/qt-collapsible-section is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Elypson/qt-collapsible-section is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Elypson/qt-collapsible-section. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QWidget>

class QFrame;
class QGridLayout;
class QParallelAnimationGroup;
class QScrollArea;
class QToolButton;
class QListWidgetItem;
class QPushButton;

class Section : public QWidget
{
private:
    Q_OBJECT

    using super = QWidget;

public slots:
    void toggle(bool expand);
    void nameClicked(bool expand);

    void paintEvent(QPaintEvent * ) override;

public:
    explicit Section(QWidget* parent = 0, const QString & title = "", const int animationDuration = 100);

    void setContentLayout(QLayout & contentLayout);
    void OnLayoutChange();
    QString GetTitle() const;
    bool IsExpanded() const;

private:
    QGridLayout* m_MainLayout { nullptr };
    QToolButton* m_ToggleButton { nullptr };
    QPushButton* m_NameLabel { nullptr };
    QFrame* m_HeaderLine { nullptr };
    QParallelAnimationGroup* m_ToggleAnimation { nullptr };
    QScrollArea* m_ContentArea { nullptr };
    QListWidgetItem* m_ListItem { nullptr };
    int m_AnimationDuration { 0 };
    int m_CollapsedHeight { 0 };
};
