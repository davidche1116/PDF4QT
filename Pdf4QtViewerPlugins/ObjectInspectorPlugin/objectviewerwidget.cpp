//    Copyright (C) 2021 Jakub Melka
//
//    This file is part of Pdf4Qt.
//
//    Pdf4Qt is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    with the written consent of the copyright owner, any later version.
//
//    Pdf4Qt is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with Pdf4Qt.  If not, see <https://www.gnu.org/licenses/>.

#include "objectviewerwidget.h"
#include "ui_objectviewerwidget.h"

#include "pdfimage.h"
#include "pdfencoding.h"
#include "pdfdocumentwriter.h"
#include "pdfexception.h"

namespace pdfplugin
{

ObjectViewerWidget::ObjectViewerWidget(QWidget *parent) :
    ObjectViewerWidget(false, parent)
{

}

ObjectViewerWidget::ObjectViewerWidget(bool isPinned, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::ObjectViewerWidget),
    m_isPinned(isPinned),
    m_cms(nullptr),
    m_document(nullptr),
    m_isRootObject(false)
{
    ui->setupUi(this);

    m_printableCharacters = pdf::PDFEncoding::getPrintableCharacters();
    m_printableCharacters.push_back('\n');

    connect(ui->pinButton, &QPushButton::clicked, this, &ObjectViewerWidget::pinRequest);
    connect(ui->unpinButton, &QPushButton::clicked, this, &ObjectViewerWidget::pinRequest);

    updateUi();
    updatePinnedUi();
}

ObjectViewerWidget::~ObjectViewerWidget()
{
    delete ui;
}

void ObjectViewerWidget::setPinned(bool isPinned)
{
    if (m_isPinned != isPinned)
    {
        m_isPinned = isPinned;
        updatePinnedUi();
    }
}

void ObjectViewerWidget::setData(pdf::PDFObjectReference currentReference, pdf::PDFObject currentObject, bool isRootObject)
{
    if (m_currentReference != currentReference ||
        m_currentObject != currentObject ||
        m_isRootObject != isRootObject)
    {
        m_currentReference = currentReference;
        m_currentObject = currentObject;
        m_isRootObject = isRootObject;

        updateUi();
    }
}

void ObjectViewerWidget::updateUi()
{
    if (m_currentReference.isValid())
    {
        QString referenceText = tr("%1 %2 R").arg(m_currentReference.objectNumber).arg(m_currentReference.generation);

        if (m_isRootObject)
        {
            ui->referenceEdit->setText(referenceText);
        }
        else
        {
            ui->referenceEdit->setText(tr("Part of object %1").arg(referenceText));
        }
    }
    else
    {
        ui->referenceEdit->setText(tr("<none>"));
    }

    switch (m_currentObject.getType())
    {
        case pdf::PDFObject::Type::Null:
            ui->typeEdit->setText(tr("Null"));
            break;
        case pdf::PDFObject::Type::Bool:
            ui->typeEdit->setText(tr("Bool"));
            break;
        case pdf::PDFObject::Type::Int:
            ui->typeEdit->setText(tr("Integer"));
            break;
        case pdf::PDFObject::Type::Real:
            ui->typeEdit->setText(tr("Real"));
            break;
        case pdf::PDFObject::Type::String:
            ui->typeEdit->setText(tr("String"));
            break;
        case pdf::PDFObject::Type::Name:
            ui->typeEdit->setText(tr("Name"));
            break;
        case pdf::PDFObject::Type::Array:
            ui->typeEdit->setText(tr("Array"));
            break;
        case pdf::PDFObject::Type::Dictionary:
            ui->typeEdit->setText(tr("Dictionary"));
            break;
        case pdf::PDFObject::Type::Stream:
            ui->typeEdit->setText(tr("Stream"));
            break;
        case pdf::PDFObject::Type::Reference:
            ui->typeEdit->setText(tr("Reference"));
            break;

        default:
            Q_ASSERT(false);
            break;
    }

    QLocale locale;

    switch (m_currentObject.getType())
    {
        case pdf::PDFObject::Type::Null:
            ui->descriptionEdit->setText(tr("null"));
            break;

        case pdf::PDFObject::Type::Bool:
            ui->descriptionEdit->setText(m_currentObject.getBool() ? tr("true") : tr("false"));
            break;

        case pdf::PDFObject::Type::Int:
            ui->descriptionEdit->setText(locale.toString(m_currentObject.getInteger()));
            break;

        case pdf::PDFObject::Type::Real:
            ui->descriptionEdit->setText(locale.toString(m_currentObject.getReal()));
            break;

        case pdf::PDFObject::Type::String:
            ui->descriptionEdit->setText(QString("\"%1\"").arg(pdf::PDFEncoding::convertSmartFromByteStringToRepresentableQString(m_currentObject.getString())));
            break;

        case pdf::PDFObject::Type::Name:
            ui->descriptionEdit->setText(QString("/%1").arg(QString::fromLatin1(m_currentObject.getString().toPercentEncoding(m_printableCharacters))));
            break;

        case pdf::PDFObject::Type::Array:
            ui->descriptionEdit->setText(tr("Array [%1 items]").arg(locale.toString(m_currentObject.getArray()->getCount())));
            break;

        case pdf::PDFObject::Type::Dictionary:
            ui->descriptionEdit->setText(tr("Dictionary [%1 items]").arg(locale.toString(m_currentObject.getDictionary()->getCount())));
            break;

        case pdf::PDFObject::Type::Stream:
            ui->descriptionEdit->setText(tr("Stream [%1 items, %2 data bytes]").arg(locale.toString(m_currentObject.getStream()->getDictionary()->getCount()), locale.toString(m_currentObject.getStream()->getContent()->size())));
            break;

        case pdf::PDFObject::Type::Reference:
            ui->descriptionEdit->setText(QString("%1 %2 R").arg(m_currentObject.getReference().objectNumber).arg(m_currentObject.getReference().generation));
            break;

        default:
            Q_ASSERT(false);
            break;
    }

    if (m_currentObject.isStream())
    {
        try
        {
            pdf::PDFDocumentDataLoaderDecorator loader(m_document);
            const pdf::PDFStream* stream = m_currentObject.getStream();
            const pdf::PDFDictionary* dictionary = stream->getDictionary();

            if (loader.readNameFromDictionary(dictionary, "Type") == "XObject" &&
                loader.readNameFromDictionary(dictionary, "Subtype") == "Image")
            {
                pdf::PDFColorSpacePointer colorSpace;

                if (dictionary->hasKey("ColorSpace"))
                {
                    const pdf::PDFObject& colorSpaceObject = m_document->getObject(dictionary->get("ColorSpace"));
                    if (colorSpaceObject.isName() || colorSpaceObject.isArray())
                    {
                        pdf::PDFDictionary dummyColorSpaceDictionary;
                        colorSpace = pdf::PDFAbstractColorSpace::createColorSpace(&dummyColorSpaceDictionary, m_document, colorSpaceObject);
                    }
                    else if (!colorSpaceObject.isNull())
                    {
                        throw pdf::PDFException(tr("Invalid color space of the image."));
                    }
                }

                pdf::PDFRenderErrorReporterDummy dummyErrorReporter;
                pdf::PDFImage pdfImage = pdf::PDFImage::createImage(m_document, stream, qMove(colorSpace), false, pdf::RenderingIntent::Perceptual, &dummyErrorReporter);
                QImage image = pdfImage.getImage(m_cms, &dummyErrorReporter);
                ui->stackedWidget->setCurrentWidget(ui->imageBrowserPage);
                ui->imageBrowser->setPixmap(QPixmap::fromImage(image));

                const int width = image.width();
                const int height = image.height();
                const int depth = loader.readIntegerFromDictionary(dictionary, "BitsPerComponent", 8);

                QString textDescription = tr("Image Stream [%1 items, %2 data bytes, %3 x %4 pixels, %5 bits per component]");
                ui->descriptionEdit->setText(textDescription.arg(locale.toString(m_currentObject.getStream()->getDictionary()->getCount()),
                                                                 locale.toString(m_currentObject.getStream()->getContent()->size()),
                                                                 locale.toString(width),
                                                                 locale.toString(height),
                                                                 locale.toString(depth)));
            }
            else
            {
                QByteArray data = m_document->getDecodedStream(stream);
                data.replace('\r', ' ');
                QByteArray percentEncodedData = data.toPercentEncoding(m_printableCharacters);
                ui->contentTextBrowser->setText(QString::fromLatin1(percentEncodedData));
                ui->stackedWidget->setCurrentWidget(ui->contentTextBrowserPage);
            }
        }
        catch (pdf::PDFException exception)
        {
            ui->contentTextBrowser->setText(exception.getMessage());
            ui->stackedWidget->setCurrentWidget(ui->contentTextBrowserPage);
        }
    }
    else
    {
        QByteArray serializedObject = pdf::PDFDocumentWriter::getSerializedObject(m_currentObject);
        QByteArray percentEncodedData = serializedObject.toPercentEncoding(m_printableCharacters);
        ui->contentTextBrowser->setText(QString::fromLatin1(percentEncodedData));
        ui->stackedWidget->setCurrentWidget(ui->contentTextBrowserPage);
    }
}

void ObjectViewerWidget::updatePinnedUi()
{
    ui->pinButton->setEnabled(!m_isPinned);
    ui->unpinButton->setEnabled(m_isPinned);
}

const pdf::PDFCMS* ObjectViewerWidget::getCms() const
{
    return m_cms;
}

void ObjectViewerWidget::setCms(const pdf::PDFCMS* cms)
{
    m_cms = cms;
}

const pdf::PDFDocument* ObjectViewerWidget::getDocument() const
{
    return m_document;
}

void ObjectViewerWidget::setDocument(const pdf::PDFDocument* document)
{
    m_document = document;
}

}   // namespace pdfplugin