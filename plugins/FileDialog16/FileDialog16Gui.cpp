#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::editor;

class FileDialog16Gui final : public PluginEditorNoGui
{
    bool m_prev_trigger = false;
    std::function<void(gmpi::api::IUnknown*)> onDialogClosed;

    void onSetTrigger()
    {
        if (!pinTrigger.value && m_prev_trigger == true)
        {
            gmpi::api::IUnknown* dialog = nullptr;
            dialogHost->createFileDialog(static_cast<int32_t>(gmpi::api::FileDialogType::Save), &dialog);
            // Set the callback function for when the dialog is closed
            onDialogClosed = [dialog](gmpi::api::IUnknown* result) {
                // Get the file path from the dialog
                gmpi::api::IUnknown* filePath = nullptr;
                //dialog->setInitialFilename(result, &filePath);
                // Call onSetFilePathOut to notify the editor of the new file path
                //onSetFilePathOut(filePath);
                // Release the dialog's reference to the file path
                //dialog->release(filePath);
                // Release the dialog
                //dialogHost->release(dialog);
                };
        }
    }

    void onSetFilePathOut()
    {
        // pinFilePathOut changed
    }

    void onSetDialogMode()
    {
        // pinStatus changed
    }

    void releaseFileDialog(gmpi::api::IUnknown* dialog) {
        dialogHost->release(dialog);
    }

 	Pin<bool> pinTrigger;
 	Pin<std::wstring> pinFilePathOut;
    Pin<int> pinDialogeMode;

public:
	FileDialog16Gui()
    {
        pinTrigger.onUpdate = [this](PinBase*) { onSetTrigger(); };
        pinFilePathOut.onUpdate = [this](PinBase*) { onSetFilePathOut(); };
        pinDialogeMode.onUpdate = [this](PinBase*) { onSetDialogMode(); };
    }
};

namespace
{
auto r = Register<FileDialog16Gui>::withXml(R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<Plugin id="My FileDialog16" name="FileDialog16" category="mx/Sub-Controls">
    <GUI>
        <Pin name="Trigger" datatype="bool" direction="out"/>
        <Pin name="File Path Out" datatype="string"/>
        <Pin name="open=0,save=1,folder=2" datatype="int"/>
    </GUI>
</Plugin>
)XML");
}
