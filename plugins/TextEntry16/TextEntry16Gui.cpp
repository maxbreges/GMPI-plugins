#pragma once 
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class TextEntry16 final : public PluginEditor
{
	sdk::TextEditCallback callback;
	gmpi::shared_ptr<gmpi::api::IUnknown> unknown;

 	void onSetText()
	{
		drawingHost->invalidateRect({});
	}
	void onSetTextColor()
	{
		drawingHost->invalidateRect({});
	}
	void onSetTopColor()
	{
		drawingHost->invalidateRect({});
	}
	void onSetBgColor()
	{
		drawingHost->invalidateRect({});
	}

	std::string fontFace;
	void onSetFontFace()
	{
		fontFace = pinFontFace.value;
	}

	int fontSize = 16;
	void onSetFontSize()
	{
		fontSize = pinFontSize.value;
	}

	int corner = 5;
	void onSetCornerRadius()
	{
		corner = pinCornerRadius.value;
	}

 	Pin<std::string> pinText;
	Pin<std::string> pinTextColor;
	Pin<std::string> pinTopColor;
	Pin<std::string> pinBgColor;
	Pin<std::string> pinFontFace;
	Pin<int> pinFontSize;
	Pin<bool> pinMouseDown;
	Pin<int> pinCornerRadius;
	Pin<bool> pinHover;
	Pin<int> pinDebug;

public:
	TextEntry16()
	{
		pinText.onUpdate = [this](PinBase*) { onSetText(); };
		pinTextColor.onUpdate = [this](PinBase*) { onSetTextColor(); };
		pinTopColor.onUpdate = [this](PinBase*) { onSetTopColor(); };
		pinBgColor.onUpdate = [this](PinBase*) { onSetBgColor(); };
		pinFontFace.onUpdate = [this](PinBase*) { onSetFontFace(); };
		pinFontSize.onUpdate = [this](PinBase*) { onSetFontSize(); };
		pinMouseDown;
		pinCornerRadius.onUpdate = [this](PinBase*) { onSetCornerRadius(); };
		pinHover;
		pinDebug;

		callback.onSuccess = [this](const std::string& text)
			{
				pinText = text;
			};
	}

	gmpi::ReturnCode setHover(bool isMouseOverMe) override
	{
		pinHover = isMouseOverMe;

		return gmpi::ReturnCode::Unhandled;
	}

	ReturnCode onPointerDown(Point point, int32_t flags) override
	{
		if ((flags & static_cast<int32_t>(gmpi::api::PointerFlags::FirstButton)) == 0)
			return ReturnCode::Unhandled;					

		inputHost->setCapture();
		return ReturnCode::Ok;
	}

	ReturnCode onPointerUp(Point, int32_t) override
	{
		bool isCaptured{};
		inputHost->getCapture(isCaptured);
		if (!isCaptured)			
			return ReturnCode::Unhandled;

		inputHost->releaseCapture();
		pinMouseDown = false;

		dialogHost->createTextEdit(&bounds, unknown.put());
		auto textEdit = unknown.as<gmpi::api::ITextEdit>();

		if (textEdit)
		{
			textEdit->setText(pinText.value.c_str());
			textEdit->showAsync(&callback);
		}

		return ReturnCode::Ok;
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);
		ClipDrawingToBounds x(g, bounds);

		// Draw a simple background rectangle
		//auto brushRect = g.createSolidColorBrush(gmpi::drawing::colorFromHex(colorHexBg));
		//g.fillRectangle(bounds, brushRect);

		//an advanced rectangle with the gradient				
		auto r = bounds;
		int width = r.right - r.left;
		int height = r.bottom - r.top;

		auto topCol = pinBgColor;
		auto botCol = topCol;

		int radius = corner;

		radius = (std::min)(radius, width / 2);
		radius = (std::min)(radius, height / 2);

		auto geometry = g.getFactory().createPathGeometry();
		auto sink = geometry.open();

		// define a corner 
		const float rightAngle = 3.14159265358979323846 * 0.5f;
		// top left
		if (5)
		{
			sink.beginFigure(Point(0, radius), FigureBegin::Filled);
			ArcSegment as(Point(radius, 0), Size(radius, radius), rightAngle);
			sink.addArc(as);
		}
		else
		{
			sink.beginFigure(Point(0, 0), FigureBegin::Filled);
		}

		// top right
		if (5)
		{
			sink.addLine(Point(width - radius, 0));
			//		sink.AddArc(Corner, 270, 90);
			ArcSegment as(Point(width, radius), Size(radius, radius), rightAngle);
			sink.addArc(as);
		}
		else
		{
			sink.addLine(Point(width, 0));
		}

		// bottom right
		if (5)
		{
			sink.addLine(Point(width, height - radius));
			//		sink.AddArc(Corner, 0, 90);
			ArcSegment as(Point(width - radius, height), Size(radius, radius), rightAngle);
			sink.addArc(as);
		}
		else
		{
			sink.addLine(Point(width, height));
		}

		// bottom left
		if (5)
		{
			sink.addLine(Point(radius, height));
			ArcSegment as(Point(0, height - radius), Size(radius, radius), rightAngle);
			sink.addArc(as);
		}
		else
		{
			sink.addLine(Point(0, height));
		}

		// end path
		sink.endFigure();
		sink.close();

		Point point1(1, 0);
		Point point2(1, height);


		Gradientstop gradientStops[]
		{
		{ 0.0f, colorFromHexString(pinTopColor.value)},
		{ 1.0f, colorFromHexString(pinBgColor.value) },
		};

		auto gradientBrush = g.createLinearGradientBrush(gradientStops, point1, point2);

		g.fillGeometry(geometry, gradientBrush);

		//===================================================================================

		// Convert font size to float
		float fontSizeFloat = static_cast<float>(fontSize);

		// Create span for font face
		if (fontFace.empty())
			fontFace = "Times New Roman"; // fallback font
		std::string_view fontFaceView = fontFace; // std::string to string_view
		std::span<const std::string_view> fontFamilies{ &fontFaceView, 1 };

		// Create text format with new API
		auto tf = g.getFactory().createTextFormat(
			fontSizeFloat,
			fontFamilies
		);

		tf.setParagraphAlignment(ParagraphAlignment::Center);
		tf.setTextAlignment(TextAlignment::Center);

		// Draw the text
		auto brush = g.createSolidColorBrush(gmpi::drawing::colorFromHexString(pinTextColor.value));
		g.drawTextU(pinText.value, tf, bounds, brush);

		return ReturnCode::Ok;
	}
};

namespace
{
auto r = Register<TextEntry16>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="TextEntry16" name="Text Entry16" category="mx/Sub-Controls">
    <GUI graphicsApi="GmpiUi">
        <Pin name="Text" datatype="string" default="Text"/>
		<Pin name="Text Color" datatype="string" default="ff000000"/>
		<Pin name="Top Color" datatype="string" default="ffff9900"/>
		<Pin name="Bg Color" datatype="string" default="ffffffff"/>
		<Pin name="Font Face" datatype="string" default="Times New Roman" isMinimised="true"/>
		<Pin name="Font Size" datatype="int" default="20" isMinimised="true"/>
		<Pin name="Mouse Down" datatype="bool"/>
		<Pin name="Corner" datatype="int" default="5" isMinimised="true"/>
		<Pin name="Mouse Over" datatype="bool" direction="out"/>
		<Pin name="Debug" datatype="int" direction="out" private="true"/>
    </GUI>
</Plugin>
)XML");
}
