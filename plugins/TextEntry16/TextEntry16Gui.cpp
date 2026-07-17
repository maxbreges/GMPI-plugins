#pragma once 
#include "helpers/GmpiPluginEditor.h"
#include "helpers/ContextMenuHelper.h"
#include "./deps_/it_enum_list.h"

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

	float corner = 5;
	void onSetCornerRadius()
	{
		corner = pinCornerRadius.value;
	}
	std::string hintUser;
	void onSetHint()
	{
		hintUser = pinHint.value;
	}

	bool disableHint = false;
	void onSetDisableHint()
	{
		disableHint = pinDisableHint.value;
	}

 	Pin<std::string> pinText;
	Pin<std::string> pinFontFace;
	Pin<int> pinFontSize;
	Pin<std::string> pinTextColor;
	Pin<std::string> pinTopColor;
	Pin<std::string> pinBgColor;
	Pin<float> pinCornerRadius;
	Pin<std::string> pinHint;
	Pin<bool> pinDisableHint;
	Pin<bool> pinMouseDown;
	Pin<bool> pinHover;
	Pin<bool> pinEntryOpen;
	Pin<bool> pinTopLeft;
	Pin<bool> pinTopRight;
	Pin<bool> pinBottomRight;
	Pin<bool> pinBottomLeft;
	Pin<std::string> pinMenuItems;
	Pin<int> pinMenuSelection;

public:
	TextEntry16()
		: callback(
			// onSuccess lambda
			[this](const std::string& text)
			{
				pinText = text;
				pinEntryOpen = false; // Close the entry on success
			},
			// onCancel lambda
			[this](void)
			{
				pinEntryOpen = false; // Close the entry on cancel
			}
		)
	{
		pinText.onUpdate = [this](PinBase*) { onSetText(); };
		pinFontFace.onUpdate = [this](PinBase*) { onSetFontFace(); };
		pinFontSize.onUpdate = [this](PinBase*) { onSetFontSize(); };
		pinTextColor.onUpdate = [this](PinBase*) { onSetTextColor(); };
		pinTopColor.onUpdate = [this](PinBase*) { onSetTopColor(); };
		pinBgColor.onUpdate = [this](PinBase*) { onSetBgColor(); };
		pinCornerRadius.onUpdate = [this](PinBase*) { onSetCornerRadius(); };
		pinHint.onUpdate = [this](PinBase*) { onSetHint(); };
		pinDisableHint.onUpdate = [this](PinBase*) { onSetDisableHint(); };
		//pinMouseDown;
		//pinHover;
		//pinEntryOpen;
		//pinMenuItems;
		//pinMenuSelection;
	}

	gmpi::ReturnCode getToolTip(gmpi::drawing::Point point, gmpi::api::IString* returnString) override
	{
		if (!disableHint)
		{
			if (hintUser.empty())
			{
				// pinHint = pinHintAuto;//<Pin name="HintAuto" datatype="string" parameterId="0" parameterField="ShortName"/>
			}
			else
			{
				pinHint = hintUser;
			}
			auto utf8String = pinHint.value;
			returnString->setData(utf8String.data(), (int32_t)utf8String.size());
			return ReturnCode::Ok;
		}
		else {}
		return ReturnCode::Unhandled;
	}

	gmpi::ReturnCode setHover(bool isMouseOverMe) override
	{
		pinHover = isMouseOverMe;
		return gmpi::ReturnCode::Unhandled;
	}

	private:
		enum class MenuItemType { Normal, Separator, Break, SubMenu, SubMenuEnd };

		static MenuItemType menuItemType(std::string_view text)
		{
			text = trimSpaces(text);
			if (text.size() < 4) return MenuItemType::Normal;
			for (size_t i = 1; i < 4; ++i)
				if (text[i] != text[0]) return MenuItemType::Normal;
			switch (text[0])
			{
			case '-': return MenuItemType::Separator;
			case '|': return MenuItemType::Break;
			case '>': return MenuItemType::SubMenu;
			case '<': return MenuItemType::SubMenuEnd;
			default:  return MenuItemType::Normal;
			}
		}

		static std::string_view trimSpaces(std::string_view text)
		{
			while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
			while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
			return text;
		}

		static std::string menuText(std::string_view text)
		{
			text = trimSpaces(text);
			if (text.size() >= 4)
			{
				const auto t = menuItemType(text);
				if (t == MenuItemType::SubMenu || t == MenuItemType::SubMenuEnd)
				{
					text.remove_prefix(4);
					text = trimSpaces(text);
				}
			}
			return std::string(text);
		}
	// Override method
	gmpi::ReturnCode populateContextMenu(gmpi::drawing::Point /*point*/, gmpi::api::IUnknown* contextMenuItemsSink) override
	{
		if (pinMenuItems.value.empty())
			return ReturnCode::Unhandled;

		gmpi::shared_ptr<gmpi::api::IUnknown> unknown;
		unknown = contextMenuItemsSink;
		auto sink = unknown.as<gmpi::api::IContextItemSink>();
		if (!sink)
			return ReturnCode::Fail;

		ContextMenuHelper menu(sink.get());

		menu.currentCallback =
			[this](int32_t selectedId)
			{
				pinMenuSelection = selectedId;
				pinMenuSelection = -1;
			};

		for (const auto& item : it_enum_list2(pinMenuItems.value))
		{
			switch (menuItemType(item.text))
			{
			case MenuItemType::Separator:  menu.addSeparator();                         break;
			case MenuItemType::SubMenu:    menu.beginSubMenu(menuText(item.text).c_str()); break;
			case MenuItemType::SubMenuEnd: menu.endSubMenu();                            break;
			case MenuItemType::Break:                                                    break;
			case MenuItemType::Normal:     menu.addItem(menuText(item.text).c_str(), item.id); break;
			}
		}
		return ReturnCode::Ok;
	}

	void onContextMenu(int32_t selection)
	{
		pinMenuSelection = selection;
		// Additional handling if needed
	}

	ReturnCode onPointerDown(Point point, int32_t flags) override
	{
		if ((flags & static_cast<int32_t>(gmpi::api::PointerFlags::FirstButton)) == 0)
			return ReturnCode::Unhandled;					
		pinMouseDown = true;
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
		pinEntryOpen = true;
		auto textEdit = unknown.as<gmpi::api::ITextEdit>();

		if (textEdit)
		{
			textEdit->setText(pinText.value.c_str());
			textEdit->showAsync(&callback); //
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
		float width = r.right - r.left;
		float height = r.bottom - r.top;

		auto topCol = pinBgColor;
		auto botCol = topCol;

		float radius = corner;

		radius = (std::min)(radius, width / 2);
		radius = (std::min)(radius, height / 2);

		auto geometry = g.getFactory().createPathGeometry();
		auto sink = geometry.open();

		// define a corner 
		const float rightAngle = 3.14159265358979323846 * 0.5f;
		// top left
		if (pinTopLeft.value)
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
		if (pinTopRight.value)
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
		if (pinBottomRight.value)
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
		if (pinBottomLeft.value)
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
		<Pin name="Font Face" datatype="string" default="Times New Roman" isMinimised="true"/>
		<Pin name="Font Size" datatype="int" default="20" isMinimised="true"/>
		<Pin name="Text Color" datatype="string" default="ff000000"/>
		<Pin name="Top Color" datatype="string" default="ffff9900"/>
		<Pin name="Bg Color" datatype="string" default="ffffffff"/>
		<Pin name="Corner Radius" datatype="float" default="5" isMinimised="true"/>
		<Pin name="Hint" datatype="string" isMinimised="true"/>
		<Pin name="Disable Hint" datatype="bool"/>
		<Pin name="Mouse Down" datatype="bool"/>
		<Pin name="Mouse Over" datatype="bool" direction="out"/>
		<Pin name="Entry Open" datatype="bool" direction="out"/>
		<Pin name="Top Left" datatype="bool" default="1" isMinimised="true"/>
		<Pin name="Top Right" datatype="bool" default="1" isMinimised="true"/>
		<Pin name="Bottom Right" datatype="bool" default="1" isMinimised="true"/>
		<Pin name="Bottom Left" datatype="bool" default="1" isMinimised="true"/>
		<Pin name="Menu Items" datatype="string" private="false"/>
		<Pin name="Menu Selection" datatype="int" private="false"/>
    </GUI>
</Plugin>
)XML");
}
