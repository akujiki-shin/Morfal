/*
 * modified version of:
 *
 * ----------------------------------

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

#include <QPropertyAnimation>
#include <QPainter>

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QListWidgetItem>
#include <QPushButton>

#include <QStyleOption>
#include <QPainter>

#include "Section.h"

Section::Section(QWidget* parent, const QString & title, const int animationDuration)
    : super(parent)
    , m_AnimationDuration(animationDuration)
{
    m_ToggleButton = new QToolButton(this);
    m_NameLabel = new QPushButton(this);
    m_HeaderLine = new QFrame(this);
    m_ToggleAnimation = new QParallelAnimationGroup(this);
    m_ContentArea = new QScrollArea(this);
    m_MainLayout = new QGridLayout(this);

    setStyleSheet("Section {border: none;}");

    m_ToggleButton->setStyleSheet("QToolButton {border: none;}");
    m_ToggleButton->setArrowType(Qt::ArrowType::RightArrow);
    m_ToggleButton->setCheckable(true);
    m_ToggleButton->setChecked(false);

    m_NameLabel->setStyleSheet("QPushButton {border: none;}");
    m_NameLabel->setMinimumWidth(100);
    m_NameLabel->setText(title);

    m_HeaderLine->setFrameShape(QFrame::HLine);
    m_HeaderLine->setFrameShadow(QFrame::Sunken);
    m_HeaderLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_ContentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_ContentArea->setMaximumHeight(0);
    m_ContentArea->setMinimumHeight(0);

    m_ToggleAnimation->addAnimation(new QPropertyAnimation(this, "minimumHeight", this));
    m_ToggleAnimation->addAnimation(new QPropertyAnimation(this, "maximumHeight", this));
    m_ToggleAnimation->addAnimation(new QPropertyAnimation(m_ContentArea, "maximumHeight", this));

    m_MainLayout->setVerticalSpacing(0);
    m_MainLayout->setContentsMargins(0, 0, 0, 0);

    int row = 0;
    m_MainLayout->addWidget(m_ToggleButton, row, 0, 1, 1, Qt::AlignLeft);
    m_MainLayout->addWidget(m_NameLabel, row, 1, 1, 1, Qt::AlignLeft);
    m_MainLayout->addWidget(m_HeaderLine, row++, 2, 1, 1);

    m_MainLayout->addWidget(m_ContentArea, row, 0, 1, 3);
    setLayout(m_MainLayout);

    m_CollapsedHeight = m_MainLayout->sizeHint().height() - m_ContentArea->maximumHeight();

    connect(m_ToggleButton, &QToolButton::toggled, this, &Section::toggle);
    connect(m_NameLabel, &QPushButton::clicked, this, &Section::nameClicked);
}

void Section::nameClicked(bool)
{
    m_ToggleButton->toggle();
}

QString Section::GetTitle() const
{
    return m_NameLabel->text();
}

void Section::toggle(bool expand)
{
    m_ToggleButton->setChecked(expand);
    m_ToggleButton->setArrowType(expand ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    m_ToggleAnimation->setDirection(expand ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    m_ToggleAnimation->start();

    if (!m_ToggleButton->isChecked())
    {
        m_ContentArea->setMinimumHeight(0);
        setMinimumHeight(m_CollapsedHeight);
    }
}

bool Section::IsExpanded() const
{
    return m_ToggleButton->isChecked();
}

void Section::setContentLayout(QLayout & contentLayout)
{
    delete m_ContentArea->layout();
    m_ContentArea->setLayout(&contentLayout);
    OnLayoutChange();
}

void Section::OnLayoutChange()
{
    int contentHeight = m_ContentArea->layout()->sizeHint().height();

    for (int i = 0; i < m_ToggleAnimation->animationCount() - 1; ++i)
    {
        QPropertyAnimation* SectionAnimation = static_cast<QPropertyAnimation *>(m_ToggleAnimation->animationAt(i));
        SectionAnimation->setDuration(m_AnimationDuration);
        SectionAnimation->setStartValue(m_CollapsedHeight);
        SectionAnimation->setEndValue(m_CollapsedHeight + contentHeight);
    }

    QPropertyAnimation* contentAnimation = static_cast<QPropertyAnimation *>(m_ToggleAnimation->animationAt(m_ToggleAnimation->animationCount() - 1));
    contentAnimation->setDuration(m_AnimationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);

   if (m_ToggleButton->isChecked())
   {
       m_ContentArea->setMaximumHeight(contentHeight);
       m_ContentArea->setMinimumHeight(contentHeight);
       setMaximumHeight(contentHeight + m_CollapsedHeight);
       setMinimumHeight(contentHeight + m_CollapsedHeight);
   }
   else
   {
       m_ContentArea->setMaximumHeight(0);
       m_ContentArea->setMinimumHeight(0);
       setMaximumHeight(m_CollapsedHeight);
       setMinimumHeight(m_CollapsedHeight);
   }
}

void Section::paintEvent(QPaintEvent * )
{
    QStyleOption styleOption;
    styleOption.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}
