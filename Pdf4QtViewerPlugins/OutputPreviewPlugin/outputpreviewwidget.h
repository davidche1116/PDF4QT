//    Copyright (C) 2021 Jakub Melka
//
//    This file is part of Pdf4Qt.
//
//    Pdf4Qt is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Pdf4Qt is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with Pdf4Qt.  If not, see <https://www.gnu.org/licenses/>.

#ifndef OUTPUTPREVIEWWIDGET_H
#define OUTPUTPREVIEWWIDGET_H

#include "pdftransparencyrenderer.h"

#include <QWidget>

namespace pdfplugin
{

class OutputPreviewWidget : public QWidget
{
    Q_OBJECT

private:
    using BaseClass = QWidget;

public:
    explicit OutputPreviewWidget(QWidget* parent);

    enum DisplayMode
    {
        Separations,
        ColorWarningInkCoverage,
        ColorWarningRichBlack,
        InkCoverage
    };

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    /// Clears all widget contents
    void clear();

    /// Set active image
    void setPageImage(QImage image, pdf::PDFFloatBitmapWithColorSpace originalProcessBitmap, QSizeF pageSizeMM);

    const pdf::PDFInkMapper* getInkMapper() const;
    void setInkMapper(const pdf::PDFInkMapper* inkMapper);

    /// Returns page image size hint (ideal size of page image)
    QSize getPageImageSizeHint() const;

    QColor getAlarmColor() const;
    void setAlarmColor(const QColor& alarmColor);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    QMargins getDrawMargins() const;
    QRect getContentRect() const;
    QRect getPageImageRect(QRect contentRect) const;

    int getInfoBoxWidth() const;
    int getInfoBoxContentHorizontalMargin() const;
    void buildInfoBoxItems();

    void addInfoBoxHeader(QString caption);
    void addInfoBoxSeparator();
    void addInfoBoxColoredItem(QColor color, QString caption, QString value);
    void addInfoBoxColoredRect(QColor color);

    enum InfoBoxStyle
    {
        Header,
        Separator,
        ColoredItem,
        ColorOnly
    };

    struct InfoBoxItem
    {
        InfoBoxItem(InfoBoxStyle style, QColor color, QString caption, QString value) :
            style(style),
            color(color),
            caption(caption),
            value(value)
        {

        }

        InfoBoxStyle style = InfoBoxStyle::Separator;
        QColor color;
        QString caption;
        QString value;
    };

    const pdf::PDFInkMapper* m_inkMapper;
    DisplayMode m_displayMode;
    std::vector<InfoBoxItem> m_infoBoxItems;
    QColor m_alarmColor;
    std::optional<QPoint> m_imagePointUnderCursor;

    QImage m_pageImage;
    pdf::PDFFloatBitmapWithColorSpace m_originalProcessBitmap;
    QSizeF m_pageSizeMM;
};

}   // pdfplugin

#endif // OUTPUTPREVIEWWIDGET_H
