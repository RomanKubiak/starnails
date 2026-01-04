/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#include "NeonLookAndFeel.h"

//==============================================================================
class starnailsApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    starnailsApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        juce::String pluginPath = commandLine.trim();

        neonLookAndFeel = std::make_unique<NeonLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (neonLookAndFeel.get());

        mainWindow.reset (new MainWindow (getApplicationName(), pluginPath));
    }

    void shutdown() override
    {
        mainWindow = nullptr; // (deletes our window)
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        neonLookAndFeel.reset();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String _name, const juce::String& _pluginPath)
            : DocumentWindow (_name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (juce::ResizableWindow::backgroundColourId),
                              0,
                              true)
        {
            setUsingNativeTitleBar (false);
            setTitleBarHeight (0);
            setResizable (false, false);
            setDropShadowEnabled (true);

            setContentOwned (new MainComponent(), true);
            setSize (1024, 600);
            centreWithSize (getWidth(), getHeight());

            if (auto* mc = dynamic_cast<MainComponent*> (getContentComponent()))
            {
                if (_pluginPath.isNotEmpty())
                {
                    juce::String err;
                    mc->loadPluginFile (juce::File (_pluginPath), err);
                }
            }

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<NeonLookAndFeel> neonLookAndFeel;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (starnailsApplication)
