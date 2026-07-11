#include "Processor.h"

using namespace gmpi;

/*
  Same DSP as the 'Gain' example, except the metadata is not embedded in the code.
  It lives in Gain_with_resource_xml.xml, which is compiled into the binary as a
  Windows resource (type GMPXML, ID 1) by Gain_with_resource_xml.rc - the same
  technique used by the legacy (SDK3) modules.

  Because the XML is external, the plugin registers with only its unique ID
  (Register<>::withId). The ID must exactly match the XML's <Plugin id="...">.
*/

struct GainWithResourceXml final : public Processor
{
	// provide connections to the audio I/O
	AudioInPin pinInput;
	AudioOutPin pinOutput;

	// provide a connection to the parameter
	FloatInPin pinGain;

	GainWithResourceXml()
	{
		// specify the member function to process audio.
		setSubProcess(&GainWithResourceXml::subProcess);
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto input = getBuffer(pinInput);
		auto output = getBuffer(pinOutput);

		// get parameter value.
		const float gain = pinGain;

		// Apply audio processing.
		for(int i = 0 ; i < sampleFrames ; ++i)
			output[i] = gain * input[i];
	}
};

namespace
{
auto r = Register<GainWithResourceXml>::withId("GMPI Gain Resource XML");
}
