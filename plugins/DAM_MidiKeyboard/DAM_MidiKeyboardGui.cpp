#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "../shared/unicode_conversion.h"
#include "mp_midi.h"

#include <array>

using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;

class DAM_DSPKeyboardMIDI_Gui : public MpGuiGfxBase
{
	static const int MAX_KEYS = 128;
	static const int baseWhiteKeyNum = 21;

protected:
	BlobGuiPin pinDispIn;
	std::vector<bool> keyStates; // Combined state used by your renderer.
	std::array<bool, MAX_KEYS> localKeyStates = {};
	std::array<bool, MAX_KEYS> inputKeyStates = {};

	int midiNote_ = -1;
	int baseKey_ = 36;
	int midiNote = -1;
	int baseKey = 36;

public:
	DAM_DSPKeyboardMIDI_Gui()
	{
		keyStates.assign(MAX_KEYS, false);
		initializePin(pinDispIn,
			static_cast<MpGuiBaseMemberPtr2>(
				&DAM_DSPKeyboardMIDI_Gui::onDisplayChanged));
	}

	void onDisplayChanged()
	{
		const auto& blob = pinDispIn.getValue();
		const int size = blob.getSize();

		// Each record contains channel, note, velocity.
		if (size < 0 || size % 3 != 0) return;
		const auto* data =
			reinterpret_cast<const unsigned char*>(blob.getData());
		if (size > 0 && !data) return;

		std::array<bool, MAX_KEYS> nextInputStates = {};
		for (int offset = 0; offset < size; offset += 3) {
			const int channel = data[offset];
			const int note = data[offset + 1];
			if (channel >= 16 || note >= MAX_KEYS) continue;

			// Record presence means held, even if velocity is zero.
			nextInputStates[note] = true;
		}

		inputKeyStates = nextInputStates;
		bool changed = false;

		for (int note = 0; note < MAX_KEYS; ++note) {
			const bool down = localKeyStates[note] || inputKeyStates[note];
			if (keyStates[note] == down) continue;
			keyStates[note] = down;
			changed = true;
		}

		if (changed) invalidateRect();
	}

	void PlayNote(int note, bool down)
	{
		if (note < 0 || note >= MAX_KEYS) return;
		unsigned char message[] = {
			static_cast<unsigned char>(down ? 0x90 : 0x80),
			static_cast<unsigned char>(note), 64
		};
		getHost()->sendMessageToAudio(0, sizeof(message), message);
	}

	void DoPlayNote(int note, bool down)
	{
		if (note < 0 || note >= MAX_KEYS) return;
		if (down) midiNote_ = note;
		if (localKeyStates[note] == down) return;

		localKeyStates[note] = down;
		const bool visibleDown = down || inputKeyStates[note];
		const bool changed = keyStates[note] != visibleDown;
		keyStates[note] = visibleDown;

		PlayNote(note, down);
		if (changed) invalidateRect();
	}

	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		// Let host handle right-clicks.
		if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
		{
			return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
		}

		setCapture();

		auto r = getRect();

		float keyHeight = r.bottom - r.top;
		float keyWidth = floorf(keyHeight * 0.2f);

		// first assume mouse over white key
		int white_key = (int)(float)(2.f * floorf(point.x / keyWidth));
		int k_num = white_key;

		float blackKeyHeight = (2 * keyHeight + 1) / 3;
		if (point.y < blackKeyHeight)
		{
			// nearest white-key-gap (not all contain black keys)
			int black_key = (int)(float)(-1 + 2 * floorf((point.x + keyWidth / 2) / keyWidth));

			// ignore gaps without black keys
			int t = (black_key + 14) % 14;
			if (t != 5 && t != 13)
			{
				// Offset black key left or right depending on note.
				float blackKeyCenter = (black_key + 1) * keyWidth / 2;
				if (t == 1 || t == 7)
				{
					blackKeyCenter -= keyWidth / 3;
				}
				if (t == 3 || t == 11)
				{
					blackKeyCenter += keyWidth / 3;
				}

				// test agaist width of black keys.
				if (fabsf(point.x - blackKeyCenter) < keyWidth * 0.5)
					k_num = black_key;
			}
		}

		int octave = k_num / 14;
		int key = k_num % 14;

		if (key > 4)
			key--;
		if (key > 11)
			key--;

		int lBaseKey = baseKey;

		{
			// don't draw partialy visible keys.
			const int maxWhiteKeys = 75;
			int visibleWhiteKeyCount = (int)(float)(r.getWidth() / keyWidth);
			visibleWhiteKeyCount = (std::min)(visibleWhiteKeyCount, maxWhiteKeys);

			if (visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys)
			{
				int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys) / 7;
				lBaseKey -= extraOctaves * 12;
			}
		}

		int note = lBaseKey + octave * 12 + key;

		if (note < 0)
			note = 0;
		if (note > 127)
			note = 127;

		key = note;

#ifdef _WIN32
		if (GetKeyState(VK_CONTROL) < 0) // <ctrl> key toggles.
#endif
		{
			DoPlayNote(key, !localKeyStates[key]);
		}
#ifdef _WIN32
		else
		{
			DoPlayNote(key, true);
		}
#endif

		return gmpi::MP_HANDLED;

	}

	int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (!getCapture())
			return gmpi::MP_OK;


		auto r = getRect();

		float keyHeight = r.bottom - r.top;
		float keyWidth = floorf(keyHeight * 0.2f);

		// first assume mouse over white key
		int white_key = (int)(float)(2 * floorf(point.x / keyWidth));
		int k_num = white_key;

		float blackKeyHeight = (2 * keyHeight + 1) / 3;
		if (point.y < blackKeyHeight)
		{
			// nearest white-key-gap (not all contain black keys)
			int black_key = (int)(float)(-1 + 2 * floorf((point.x + keyWidth / 2) / keyWidth));

			// ignore gaps without black keys
			int t = (black_key + 14) % 14;
			if (t != 5 && t != 13)
			{
				// Offset black key left or right depending on note.
				float blackKeyCenter = (black_key + 1) * keyWidth / 2;
				if (t == 1 || t == 7)
				{
					blackKeyCenter -= keyWidth / 3;
				}
				if (t == 3 || t == 11)
				{
					blackKeyCenter += keyWidth / 3;
				}

				// test agaist width of black keys.
				if (fabsf(point.x - blackKeyCenter) < keyWidth * 0.5)
					k_num = black_key;
			}
		}

		int octave = k_num / 14;
		int key = k_num % 14;

		if (key > 4)
			key--;
		if (key > 11)
			key--;

		int lBaseKey = baseKey;

		{
			// don't draw partialy visible keys.
			const int maxWhiteKeys = 75;
			int visibleWhiteKeyCount = (int)(float)(r.getWidth() / keyWidth);
			visibleWhiteKeyCount = (std::min)(visibleWhiteKeyCount, maxWhiteKeys);

			if (visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys)
			{
				int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys) / 7;
				lBaseKey -= extraOctaves * 12;
			}
		}

		int note = lBaseKey + octave * 12 + key;

		if (note < 0)
			note = 0;
		if (note > 127)
			note = 127;


		int newMidiNote = note;

		if (midiNote_ != newMidiNote)
		{
			int oldMidiNote = midiNote_;

			DoPlayNote(newMidiNote, true);

			// turn off current note
			if (/*!toggleMode_ && */oldMidiNote > -1)
			{
				DoPlayNote(oldMidiNote, false);
			}
		}


		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{

		if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
		{
			return gmpi::MP_UNHANDLED;
		}

		if (!getCapture())
			return gmpi::MP_OK;


#ifdef _WIN32
		if (GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0 && midiNote_ >= 0) // <ctrl> key toggles. <shft> adds to selection.
#endif
		{
			DoPlayNote(midiNote_, false);
			midiNote_ = -1;
		}

		releaseCapture();

		return gmpi::MP_OK;
	}


	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		auto r = getRect();

		g.PushAxisAlignedClip(r);

		int width = (int)(float)(r.right - r.left);
		float keyHeight = r.bottom - r.top;
		float keyWidth = floor(keyHeight * 0.2f);

		bool prevWhiteKeyState = false;
		auto brushShadow = g.CreateSolidColorBrush(Color::SlateGray);
		auto highlightBrush = g.CreateSolidColorBrush(Color::White);

		int lBaseKey = baseKey;

		// don't draw partialy visible keys.
		const int maxWhiteKeys = 75;
		int visibleWhiteKeyCount = width / (int)(float)keyWidth;
		visibleWhiteKeyCount = (std::min)(visibleWhiteKeyCount, maxWhiteKeys);

		if (visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys)
		{
			int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys) / 7;
			lBaseKey -= extraOctaves * 12;
		}

		int usableWidth = (int)(float)(visibleWhiteKeyCount * keyWidth);

		auto blueBrush = g.CreateSolidColorBrush(Color::FromBytes(0, 0, 255));
		g.FillRectangle((float)(int)usableWidth, 0, (float)(int)width - (float)(int)usableWidth, keyHeight, blueBrush);

		// light-grey white-key fill
		Color keyColor = Color::FromBytes(220, 220, 220);
		Color keyGRadColor = Color::FromBytes(200, 200, 200);
		auto wbrush = g.CreateSolidColorBrush(keyColor);
		g.FillRectangle(0, 0, (float)(int)usableWidth, keyHeight, wbrush);

		// top edge shadow.
		int top_shadow = (int)(float)(keyHeight / 16);
		Point grad1(0.f, -1.f);
		Point grad2(0.f, (float)top_shadow);

		GradientStop gradientStops[2];
		gradientStops[0].color = Color::SlateGray;
		gradientStops[0].position = 0.0f;
		gradientStops[1].color = keyColor;
		gradientStops[1].position = 1.0f;

		auto gradientStopCollection = g.CreateGradientStopCollection(gradientStops, 2);
		LinearGradientBrushProperties lgbp1(grad1, grad2);
		auto gradientBrush2 = g.CreateLinearGradientBrush(lgbp1, BrushProperties(), gradientStopCollection);// grad1, grad2, Color::SlateGray, keyColor);
		g.FillRectangle(0.0f, 0.0f, (float)usableWidth, (float)top_shadow - 1.0f, gradientBrush2);

		// white keys highlights + shadows on left
		grad1 = Point(0.f, (float)keyHeight);
		grad2 = Point(0.f, (float)keyHeight / 2);

		auto wkgradientBrush = g.CreateLinearGradientBrush(keyGRadColor, keyColor, grad1, grad2);

		int key = lBaseKey;
		for (float x = 0; x < usableWidth; x += keyWidth)
		{
			if (keyStates[key])
			{
				// gradient on white key
				g.FillRectangle(x, keyHeight / 2 + 1, x + keyWidth, keyHeight, wkgradientBrush);

				if (!prevWhiteKeyState) // key on, draw shadow
				{
					std::vector<Point> points = {
						{ x + 1 , 0 },
					{ x , keyHeight },
					{ x + keyWidth / 4 , keyHeight },
					{ x + keyWidth / 8 , 0 },
					};

					g.FillPolygon(points, brushShadow);
				}
			}
			else
			{
				g.DrawLine(x + 1, (float)(int)top_shadow, x + 1, keyHeight, highlightBrush); // white highlight on left
			}

			prevWhiteKeyState = keyStates[key];

			key += 2;

			int t = key % 12;
			if (t == 1 || t == 6) // skip 'gaps' in black keys
			{
				key--;
			}
		}

		// black key shadows
		int kn = 0;
		if (lBaseKey == 21) // Special case for piano.
		{
			kn = 5;
		}
		key = lBaseKey + 1;
		float blackKeyWidth = (2 * keyWidth + 1) / 3;
		float blackKeyHeight = (2 * keyHeight + 1) / 3;
		float blackKeyTopIndent = (blackKeyWidth + 2) / 4;
		float highlightY = blackKeyHeight / 2;
		for (float wx = 0; wx < usableWidth; wx += keyWidth)
		{
			int t = kn++ % 7;
			if (t != 0 && t != 3)
			{
				float x = wx - blackKeyWidth / 2;

				switch (t)
				{
				case 1:
				case 4:
					x -= (blackKeyWidth + 2) / 6;
					break;
				case 6:
				case 2:
					x += (blackKeyWidth + 2) / 6;
					break;
				};

				float top_indent;
				if (keyStates[key]) // key on, draw highlight different
					top_indent = blackKeyTopIndent;
				else
					top_indent = (blackKeyTopIndent * 2) / 3;

				float shadowLength = blackKeyWidth / 4;
				float bottomshadowLength = shadowLength;
				if (keyStates[key] && !keyStates[key - 1]) // key down, bottom shadow reduced, unless white key down too
				{
					bottomshadowLength = bottomshadowLength / 4;
				}

				if (keyStates[key - 1] && !keyStates[key]) // left white key down, bottom shadow increased
				{
					bottomshadowLength += shadowLength;
				}
				if (bottomshadowLength > wx - x) // prevent shadow overlapping adjacent white key
				{
					bottomshadowLength = wx - x;
				}


				std::vector<Point> points = {
					{ x , blackKeyHeight - 1 },
				{ x + bottomshadowLength , blackKeyHeight + bottomshadowLength },
				{ wx , blackKeyHeight + bottomshadowLength },
				{ wx , blackKeyHeight - 1 },
				};

				g.FillPolygon(points, brushShadow);

				// shadow of black key falling on right white key.
				bottomshadowLength = shadowLength;
				if (keyStates[key] && !keyStates[key + 1]) // key down, bottom shadow reduced, unless white key down too
				{
					bottomshadowLength = bottomshadowLength / 4;
				}
				if (keyStates[key + 1] && !keyStates[key]) // left white key down, bottom shadow increased
				{
					bottomshadowLength += shadowLength;
				}


				std::vector<Point> points2 = {
					{ wx, blackKeyHeight - 1 },
				{ wx, blackKeyHeight + bottomshadowLength },
				{ x + blackKeyWidth + bottomshadowLength, blackKeyHeight + bottomshadowLength },
				{ x + (blackKeyWidth * 5) / 4 + (bottomshadowLength + shadowLength) / 2, blackKeyHeight / 2 + shadowLength },
				{ x + blackKeyWidth + shadowLength, 0 },
				{ x, 0 },
				};

				g.FillPolygon(points2, brushShadow);

				key += 2;

				int t2 = key % 12;
				if (t2 == 0 || t2 == 5) // skip 'gaps' in black keys
				{
					key++;
				}
			}
		}

		auto pen = g.CreateSolidColorBrush(Color::FromBytes(30, 30, 30)); // blackish

		// white keys
		key = lBaseKey;
		for (int x = 0; x < usableWidth; x += (int)(float)keyWidth)
		{
			g.DrawLine((float)(int)x, 0, (float)(int)x, keyHeight, pen); // black line between keys.

			key += 2;

			int t = key % 12;
			if (t == 1 || t == 6) // skip 'gaps' in black keys
			{
				key--;
			}
		}

		// black keys
		kn = 0;
		if (lBaseKey == 21) // Special case for piano.
		{
			kn = 5;
		}
		key = lBaseKey + 1;

		auto brush = g.CreateSolidColorBrush(Color::Black);
		grad1 = Point(0.f, 0.f);
		grad2 = Point(0.f, (float)blackKeyHeight);
		//auto gradientBrush = g.CreateLinearGradientBrush( grad1, grad2, Color::Black, Color::Gray);
		auto gradientBrush = g.CreateLinearGradientBrush(Color::Black, Color::Gray, grad1, grad2);
		//auto gradientBrush3 = g.CreateLinearGradientBrush( grad1, Point(0.f, (float)blackKeyHeight / 2.0f), Color::Gray, Color::Black);
		auto gradientBrush3 = g.CreateLinearGradientBrush(Color::Gray, Color::Black, grad1, Point(0.f, (float)blackKeyHeight / 2.0f));

		// trapezoid on left of black keys
		for (int wx = 0; wx < usableWidth; wx += (int)(float)keyWidth)
		{
			int t = kn++ % 7;
			if (t != 0 && t != 3)
			{
				float x = wx - blackKeyWidth * 0.5f;

				switch (t)
				{
				case 1:
				case 4:
					x -= (blackKeyWidth + 2) / 6.f;
					break;
				case 6:
				case 2:
					x += (blackKeyWidth + 2) / 6.f;
					break;
				};

				float top_indent;
				if (keyStates[key]) // key on, draw highlight different
					top_indent = blackKeyTopIndent;
				else
					top_indent = (blackKeyTopIndent * 2) / 3.f;

				// key background.
				g.FillRectangle(x, -1, x + blackKeyWidth, blackKeyHeight - 1, brush);

				// highlight on upper half.
				if (keyStates[key]) // key on, draw highlight different
				{
					g.FillRectangle(x + blackKeyTopIndent, top_indent, x + blackKeyTopIndent + blackKeyWidth - 2 * blackKeyTopIndent, blackKeyHeight / 2 - top_indent, gradientBrush3);
				}
				else
				{
					g.FillRectangle(x + blackKeyTopIndent, top_indent, x + blackKeyTopIndent + blackKeyWidth - 2 * blackKeyTopIndent, blackKeyHeight / 2 - top_indent, gradientBrush);
				}

				// left side of black key, trapezoid.
				Point points[]{
						{ x + 1, 0 },
						{ x + blackKeyTopIndent, top_indent },
						{ 0, 0 },
						{ x + 1, blackKeyHeight - 2 },
				};

				if (keyStates[key]) // key on, draw highlight different
				{
					points[2].x = x + blackKeyTopIndent / 2;
					points[2].y = blackKeyHeight - (blackKeyTopIndent * 4) / 3;
				}
				else
				{
					points[2].x = x + blackKeyTopIndent;
					points[2].y = blackKeyHeight - blackKeyTopIndent * 2;
				}

				// gradient
				auto gradientBrush2 = g.CreateLinearGradientBrush(Color::Black, Color::Gray, Point(x, 0.f), Point(x + blackKeyTopIndent, 0.f));

				// draw it.
				g.FillPolygon(points, gradientBrush2);

				key += 2;

				int t2 = key % 12;
				if (t2 == 0 || t2 == 5) // skip 'gaps' in black keys
				{
					key++;
				}
			}
		}

		g.PopAxisAlignedClip();

		return gmpi::MP_OK;

	}
	int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		*returnDesiredSize = availableSize;
		const float minSize = 10;
		returnDesiredSize->height = (std::max)(returnDesiredSize->height, minSize);
		returnDesiredSize->width = (std::max)(returnDesiredSize->width, minSize);

		return gmpi::MP_OK;
	}
	int32_t MP_STDCALL hitTest(MP1_POINT point) override
	{

		return MP_OK;

	}
};

namespace
{
	auto r = Register<DAM_DSPKeyboardMIDI_Gui>::withId(L"My DAM_MidiKeyboard");
}
