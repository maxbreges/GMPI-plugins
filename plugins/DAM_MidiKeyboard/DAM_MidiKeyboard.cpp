#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/mp_midi.h"
#include <bitset>

using namespace gmpi;

class DAM_MidiKeyboard final : public MpBase2
{
	static const int MAX_KEYS = 128;

	MidiInPin pinMIDIIn;
	IntInPin pinChannelIn, pinChannelOut;
	MidiOutPin pinMIDIOut;
	BlobOutPin pinDispOut;

	struct SimpleMidiMessage {
		unsigned char data[3];
	};

	gmpi::midi_2_0::MidiConverter2 inputConverter, outputConverter;
	std::vector<SimpleMidiMessage> buffer;

	// Incoming display state, matching your capture example.
	std::bitset<MAX_KEYS> keyStates;
	int keyVelocities[MAX_KEYS] = {};
	int keyChannels[MAX_KEYS] = {};
	std::vector<int> activeInputNotes;

	// Onscreen-generated output state, independent of incoming notes.
	bool activeOutputNotes[16][MAX_KEYS] = {};
	int previousChannelIn = -1, previousChannelOut = -1;

	static int inputChannelValue(int value) {
		return (std::max)(0, (std::min)(16, value)); // 0 = All.
	}

	static int outputChannelValue(int value) {
		return (std::max)(0, (std::min)(15, value)); // Displayed 1–16.
	}

	void packAndSendBlob() {
		std::vector<uint8_t> blobData;
		blobData.reserve(activeInputNotes.size() * 3);

		for (int note : activeInputNotes) {
			blobData.push_back(static_cast<uint8_t>(keyChannels[note]));
			blobData.push_back(static_cast<uint8_t>(note));
			blobData.push_back(static_cast<uint8_t>(keyVelocities[note]));
		}

		// Valid address even when publishing an empty blob.
		const uint8_t empty = 0;
		const void* data = blobData.empty() ? &empty : blobData.data();
		MpBlob blob(static_cast<int>(blobData.size()), data);
		pinDispOut = blob;
		pinDispOut.sendPinUpdate();
	}

	void clearInputNotes() {
		keyStates.reset();
		activeInputNotes.clear();
		std::fill(keyVelocities, keyVelocities + MAX_KEYS, 0);
		std::fill(keyChannels, keyChannels + MAX_KEYS, 0);
		packAndSendBlob();
	}

	void queueOutput(unsigned char status, unsigned char note,
		unsigned char velocity) {
		buffer.push_back({ {status, note, velocity} });
	}

	void flushOutputNotes() {
		for (int channel = 0; channel < 16; ++channel)
			for (int note = 0; note < MAX_KEYS; ++note) {
				if (!activeOutputNotes[channel][note]) continue;
				queueOutput(static_cast<unsigned char>(0x80 | channel),
					static_cast<unsigned char>(note), 0);
				activeOutputNotes[channel][note] = false;
			}
	}

public:
	DAM_MidiKeyboard() :
		inputConverter([this](const midi::message_view& msg, int) {
		onMidi2Message(msg);
			}),
		outputConverter([this](const midi::message_view& msg, int offset) {
		pinMIDIOut.send(msg.begin(), static_cast<int>(msg.size()), offset);
			})
	{
		// Must match the Audio pin order in your XML.
		initializePin(pinMIDIIn);
		initializePin(pinChannelIn);
		initializePin(pinChannelOut);
		initializePin(pinMIDIOut);
		initializePin(pinDispOut);

		activeInputNotes.reserve(MAX_KEYS);
		buffer.reserve(256);
		setSubProcess(&DAM_MidiKeyboard::subProcess);
	}

	void onMidiMessage(int pin, const unsigned char* midiMessage, int size) override
	{
		if (!midiMessage || size <= 0) return;
		const midi::message_view msg(midiMessage, size);
		inputConverter.processMidi(msg, -1);
	}

	void onMidi2Message(const midi::message_view& msg)
	{
		if (msg.size() != 8) return;
		const auto header = gmpi::midi_2_0::decodeHeader(msg);
		if (header.messageType != gmpi::midi_2_0::ChannelVoice64) return;
		if (header.status != gmpi::midi_2_0::NoteOn &&
			header.status != gmpi::midi_2_0::NoteOff) return;

		const int channel = header.channel;
		if (channel < 0 || channel >= 16) return;

		const int selectedChannel = inputChannelValue(pinChannelIn.getValue());
		if (selectedChannel && channel != selectedChannel - 1) return;

		const auto note = gmpi::midi_2_0::decodeNote(msg);
		const int number = note.noteNumber;
		if (number < 0 || number >= MAX_KEYS) return;

		if (header.status == gmpi::midi_2_0::NoteOn) {
			if (keyStates.test(number)) return;

			keyStates.set(number);
			keyVelocities[number] = (std::max)(0, (std::min)(
				127, static_cast<int>(127.0f * note.velocity)));
			keyChannels[number] = channel;
			activeInputNotes.push_back(number);
			packAndSendBlob();
		}
		else if (header.status == gmpi::midi_2_0::NoteOff) {
			if (!keyStates.test(number)) return;

			keyStates.reset(number);
			keyVelocities[number] = 0;
			activeInputNotes.erase(std::remove(
				activeInputNotes.begin(), activeInputNotes.end(), number),
				activeInputNotes.end());
			packAndSendBlob();
		}
	}

	int32_t MP_STDCALL receiveMessageFromGui(
		int32_t id, int32_t size, const void* messageData) override
	{
		// Only onscreen-generated notes use this message path.
		if (id != 0 || size != 3 || !messageData) return gmpi::MP_OK;
		const auto* message = static_cast<const unsigned char*>(messageData);
		const int type = message[0] & 0xF0;
		if (type != 0x90 && type != 0x80) return gmpi::MP_OK;
		if (message[1] >= MAX_KEYS || message[2] > 127) return gmpi::MP_OK;

		const int channel = outputChannelValue(pinChannelOut.getValue());
		const int note = message[1];
		const bool noteOn = type == 0x90 && message[2] != 0;

		if (noteOn) {
			if (activeOutputNotes[channel][note]) return gmpi::MP_OK;
			activeOutputNotes[channel][note] = true;
		}
		else {
			if (!activeOutputNotes[channel][note]) return gmpi::MP_OK;
			activeOutputNotes[channel][note] = false;
		}

		queueOutput(static_cast<unsigned char>(type | channel),
			message[1], message[2]);
		setSleep(false);
		return gmpi::MP_OK;
	}

	void subProcess(int sampleFrames)
	{
		if (sampleFrames <= 0) return;
		const int offset = getBlockPosition();

		for (const auto& msg : buffer)
			outputConverter.processMidi({ msg.data, sizeof(msg.data) }, offset);

		buffer.clear();
		setSleep(true);
	}

	void onSetPins() override
	{
		if (pinChannelIn.isUpdated()) {
			const int channel = inputChannelValue(pinChannelIn.getValue());
			if (channel != previousChannelIn) {
				previousChannelIn = channel;
				clearInputNotes();
			}
		}

		if (pinChannelOut.isUpdated()) {
			const int channel = outputChannelValue(pinChannelOut.getValue());
			if (channel != previousChannelOut) {
				flushOutputNotes();
				previousChannelOut = channel;
			}
		}

		if (!buffer.empty()) setSleep(false);
	}
};

namespace
{
	auto r = Register<DAM_MidiKeyboard>::withId(L"My DAM_MidiKeyboard");
}
