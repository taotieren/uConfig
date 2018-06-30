/**
 ** This file is part of the uConfig project.
 ** Copyright 2018 Robotips sebastien.caux@robotips.fr.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

#include "drawrectitem.h"

#include "componentitem.h"

#include <QPainter>
#include <QDebug>

DrawRectItem::DrawRectItem(DrawRect *draw)
    : DrawItem(draw)
{
    setDraw(draw);
    setZValue(-1);
}

void DrawRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setPen(QPen(QColor(132, 0, 0), 0.254 * draw()->thickness()));

    if (_drawRect->filled() != DrawRect::DrawNotFilled)
        painter->setBrush(QColor(255, 255, 194));

    QFont font = ComponentItem::font();
    painter->setFont(font);

    QRect rect;
    rect.setTopLeft(QPoint(0, 0));
    rect.setSize(_drawRect->rect().size() / ComponentItem::ratio);
    painter->drawRect(rect);
}

QRectF DrawRectItem::boundingRect() const
{
    return _rect.adjusted(-2, -2, 2, 2);
}

void DrawRectItem::setDraw(DrawRect *draw)
{
    _drawRect = draw;
    _rect.setSize(_drawRect->rect().size() / ComponentItem::ratio);
    _rect.setTopLeft(QPoint(0, 0));
    _rect = _rect.normalized();

    setPos(draw->pos() / ComponentItem::ratio);
    update();
}