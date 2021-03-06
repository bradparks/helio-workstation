/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

//[Headers]
#include "Common.h"
//[/Headers]

#include "AuthorizationDialog.h"

//[MiscUserDefs]
#include "HelioTheme.h"

class LabelWithPassword : public Label
{
public:

    LabelWithPassword() : passwordCharacter(0) {}

    void setPasswordCharacter (const juce_wchar newPasswordCharacter)
    {
        if (passwordCharacter != newPasswordCharacter)
        {
            passwordCharacter = newPasswordCharacter;
            this->repaint();
        }
    }

    juce_wchar getPasswordCharacter() const noexcept
    {
        return this->passwordCharacter;
    }

    void paint (Graphics& g) override
    {
        HelioTheme &ht = static_cast<HelioTheme &>(getLookAndFeel());
        ht.drawLabel (g, *this, getPasswordCharacter());
    }

protected:

    static void copyColourIfSpecified (Label& l, TextEditor& ed, int colourID, int targetColourID)
    {
        if (l.isColourSpecified (colourID) || l.getLookAndFeel().isColourSpecified (colourID))
            ed.setColour (targetColourID, l.findColour (colourID));
    }

    TextEditor *createEditorComponent()
    {
        TextEditor* const ed = new TextEditor (getName(), this->passwordCharacter);
        ed->applyFontToAllText (getLookAndFeel().getLabelFont (*this));
        copyAllExplicitColoursTo (*ed);

        copyColourIfSpecified (*this, *ed, textWhenEditingColourId, TextEditor::textColourId);
        copyColourIfSpecified (*this, *ed, backgroundWhenEditingColourId, TextEditor::backgroundColourId);
        copyColourIfSpecified (*this, *ed, outlineWhenEditingColourId, TextEditor::focusedOutlineColourId);

        return ed;
    }

private:

    juce_wchar passwordCharacter;

};

#include "App.h"
#include "AuthorizationManager.h"
#include "ProgressTooltip.h"
#include "SuccessTooltip.h"
#include "FailTooltip.h"
#include "MainLayout.h"
#include "Config.h"
#include "SerializationKeys.h"
//[/MiscUserDefs]

AuthorizationDialog::AuthorizationDialog()
{
    addAndMakeVisible (background = new PanelC());
    addAndMakeVisible (panel = new PanelA());
    addAndMakeVisible (loginButton = new TextButton (String()));
    loginButton->setButtonText (TRANS("dialog::auth::proceed"));
    loginButton->setConnectedEdges (Button::ConnectedOnTop);
    loginButton->addListener (this);

    addAndMakeVisible (emailEditor = new Label (String(),
                                                TRANS("...")));
    emailEditor->setFont (Font (Font::getDefaultSerifFontName(), 37.00f, Font::plain).withTypefaceStyle ("Regular"));
    emailEditor->setJustificationType (Justification::centredLeft);
    emailEditor->setEditable (true, true, false);
    emailEditor->setColour (Label::textColourId, Colours::white);
    emailEditor->setColour (TextEditor::textColourId, Colours::black);
    emailEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    emailEditor->addListener (this);

    addAndMakeVisible (emailLabel = new Label (String(),
                                               TRANS("dialog::auth::email")));
    emailLabel->setFont (Font (Font::getDefaultSerifFontName(), 21.00f, Font::plain).withTypefaceStyle ("Regular"));
    emailLabel->setJustificationType (Justification::centredRight);
    emailLabel->setEditable (false, false, false);
    emailLabel->setColour (Label::textColourId, Colour (0x77ffffff));
    emailLabel->setColour (TextEditor::textColourId, Colours::black);
    emailLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (passwordLabel = new Label (String(),
                                                  TRANS("dialog::auth::password")));
    passwordLabel->setFont (Font (Font::getDefaultSerifFontName(), 21.00f, Font::plain).withTypefaceStyle ("Regular"));
    passwordLabel->setJustificationType (Justification::centredRight);
    passwordLabel->setEditable (false, false, false);
    passwordLabel->setColour (Label::textColourId, Colour (0x77ffffff));
    passwordLabel->setColour (TextEditor::textColourId, Colours::black);
    passwordLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (shadow = new ShadowDownwards());
    addAndMakeVisible (cancelButton = new TextButton (String()));
    cancelButton->setButtonText (TRANS("dialog::auth::cancel"));
    cancelButton->setConnectedEdges (Button::ConnectedOnTop);
    cancelButton->addListener (this);

    addAndMakeVisible (passwordEditor = new LabelWithPassword());


    //[UserPreSize]
    const String lastLogin = Config::get(Serialization::Core::lastUsedLogin);
#if HELIO_DESKTOP
    const String defaultLogin = TRANS("dialog::auth::defaultlogin::desktop");
#elif HELIO_MOBILE
    const String defaultLogin = TRANS("dialog::auth::defaultlogin::mobile");
#endif

    this->emailEditor->setText(lastLogin.isEmpty() ? defaultLogin : lastLogin, sendNotificationSync);
    this->emailEditor->setKeyboardType(TextInputTarget::emailAddressKeyboard);
    //this->messageLabel->setText(message, dontSendNotification);

    this->passwordEditor->setFont (Font (Font::getDefaultSerifFontName(), 37.00f, Font::plain).withTypefaceStyle ("Regular"));
    this->passwordEditor->setJustificationType (Justification::centredLeft);
    this->passwordEditor->setEditable (true, true, false);
    this->passwordEditor->setColour (Label::textColourId, Colours::white);
    this->passwordEditor->setColour (TextEditor::textColourId, Colours::black);
    this->passwordEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    this->passwordEditor->addListener (this);
    this->passwordEditor->setPasswordCharacter(static_cast<juce_wchar>(0x00B7));
    this->passwordEditor->setText("111", sendNotification);
    //[/UserPreSize]

    setSize (530, 230);

    //[Constructor]
    this->rebound();
    //[/Constructor]
}

AuthorizationDialog::~AuthorizationDialog()
{
    //[Destructor_pre]
    FadingDialog::fadeOut();
    //[/Destructor_pre]

    background = nullptr;
    panel = nullptr;
    loginButton = nullptr;
    emailEditor = nullptr;
    emailLabel = nullptr;
    passwordLabel = nullptr;
    shadow = nullptr;
    cancelButton = nullptr;
    passwordEditor = nullptr;

    //[Destructor]
    //[/Destructor]
}

void AuthorizationDialog::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x59000000));
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (getWidth() - 0), static_cast<float> (getHeight() - 0), 10.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AuthorizationDialog::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    background->setBounds ((getWidth() / 2) - (520 / 2), 5, 520, getHeight() - 10);
    panel->setBounds (15, 15, getWidth() - 30, getHeight() - 78);
    loginButton->setBounds (((getWidth() / 2) - (520 / 2)) + 520 / 2 + 68 - (346 / 2), getHeight() - 63, 346, 46);
    emailEditor->setBounds ((getWidth() / 2) + 50 - (361 / 2), 5 + 36, 361, 40);
    emailLabel->setBounds ((getWidth() / 2) + -186 - (96 / 2), 5 + 28, 96, 22);
    passwordLabel->setBounds ((getWidth() / 2) + -186 - (96 / 2), 5 + 88, 96, 22);
    shadow->setBounds (((getWidth() / 2) - (520 / 2)) + 520 / 2 - (480 / 2), getHeight() - 66, 480, 24);
    cancelButton->setBounds (((getWidth() / 2) - (520 / 2)) + 520 / 2 + -181 - (120 / 2), getHeight() - 63, 120, 46);
    passwordEditor->setBounds ((getWidth() / 2) + 50 - (361 / 2), 96, 361, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AuthorizationDialog::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == loginButton)
    {
        //[UserButtonCode_loginButton] -- add your button handler code here..
        this->login();
        //[/UserButtonCode_loginButton]
    }
    else if (buttonThatWasClicked == cancelButton)
    {
        //[UserButtonCode_cancelButton] -- add your button handler code here..
        delete this;
        //[/UserButtonCode_cancelButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void AuthorizationDialog::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == emailEditor)
    {
        //[UserLabelCode_emailEditor] -- add your label text handling code here..
        //[/UserLabelCode_emailEditor]
    }

    //[UserlabelTextChanged_Post]
    this->loginButton->setEnabled(this->validateTextFields());
    //[/UserlabelTextChanged_Post]
}

void AuthorizationDialog::parentHierarchyChanged()
{
    //[UserCode_parentHierarchyChanged] -- Add your code here...
    this->rebound();
    //[/UserCode_parentHierarchyChanged]
}

void AuthorizationDialog::parentSizeChanged()
{
    //[UserCode_parentSizeChanged] -- Add your code here...
    this->rebound();
    //[/UserCode_parentSizeChanged]
}

bool AuthorizationDialog::keyPressed (const KeyPress& key)
{
    //[UserCode_keyPressed] -- Add your code here...
    if (key.isKeyCode(KeyPress::returnKey))
    {
        if (this->validateTextFields())
        {
            this->login();
        }

        return true;
    }
    else if (key.isKeyCode(KeyPress::tabKey))
    {
        // don't delete self
        return true;
    }

    return false;  // Return true if your handler uses this key event, or false to allow it to be passed-on.
    //[/UserCode_keyPressed]
}

void AuthorizationDialog::inputAttemptWhenModal()
{
    //[UserCode_inputAttemptWhenModal] -- Add your code here...
    delete this;
    //[/UserCode_inputAttemptWhenModal]
}


//[MiscUserCode]

void AuthorizationDialog::editorShown(Label *targetLabel, TextEditor &editor)
{
    //String::repeatedString(String::charToString (passwordCharacter), atomText.length())

//    if (targetLabel == this->passwordEditor)
//    {
//        editor.setPasswordCharacter((juce_wchar)'*');
//    }
}

void AuthorizationDialog::login()
{
    Config::set(Serialization::Core::lastUsedLogin, this->emailEditor->getText());

    App::Helio()->getAuthManager()->addChangeListener(this);
    App::Helio()->showModalComponent(new ProgressTooltip());

    const String passwordHash = SHA256(this->passwordEditor->getText().toUTF8()).toHexString();
    const String email = this->emailEditor->getText();

    App::Helio()->getAuthManager()->login(email, passwordHash);
}

bool AuthorizationDialog::validateTextFields() const
{
    // simple asf
    const bool hasValidEmail = (this->emailEditor->getText().isNotEmpty() &&
                                this->emailEditor->getText().contains("@") &&
                                this->emailEditor->getText().contains("."));

    const bool hasValidPassword = (this->passwordEditor->getText().length() >= 6);

    return hasValidEmail && hasValidPassword;
}

void AuthorizationDialog::changeListenerCallback(ChangeBroadcaster *source)
{
    AuthorizationManager *authManager = App::Helio()->getAuthManager();
    authManager->removeChangeListener(this);

    Component *progressIndicator = App::Layout().findChildWithID(ProgressTooltip::componentId);

    if (progressIndicator)
    {
        delete progressIndicator;

        if (authManager->getLastRequestState() == AuthorizationManager::RequestSucceed)
        {
            App::Helio()->showModalComponent(new SuccessTooltip());
            delete this;
        }
        else if (authManager->getLastRequestState() == AuthorizationManager::RequestFailed)
        {
            App::Helio()->showModalComponent(new FailTooltip());
        }
        if (authManager->getLastRequestState() == AuthorizationManager::ConnectionFailed)
        {
            App::Helio()->showModalComponent(new FailTooltip());
        }
    }
}


//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AuthorizationDialog" template="../../Template"
                 componentName="" parentClasses="public FadingDialog, private ChangeListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="530"
                 initialHeight="230">
  <METHODS>
    <METHOD name="parentHierarchyChanged()"/>
    <METHOD name="parentSizeChanged()"/>
    <METHOD name="keyPressed (const KeyPress&amp; key)"/>
    <METHOD name="inputAttemptWhenModal()"/>
  </METHODS>
  <BACKGROUND backgroundColour="0">
    <ROUNDRECT pos="0 0 0M 0M" cornerSize="10" fill="solid: 59000000" hasStroke="0"/>
  </BACKGROUND>
  <JUCERCOMP name="" id="e96b77baef792d3a" memberName="background" virtualName=""
             explicitFocusOrder="0" pos="0Cc 5 520 10M" posRelativeH="ac3897c4f32c4354"
             sourceFile="../Themes/PanelC.cpp" constructorParams=""/>
  <JUCERCOMP name="" id="c55a4a1bfd41a78f" memberName="panel" virtualName=""
             explicitFocusOrder="0" pos="15 15 30M 78M" sourceFile="../Themes/PanelA.cpp"
             constructorParams=""/>
  <TEXTBUTTON name="" id="7855caa7c65c5c11" memberName="loginButton" virtualName=""
              explicitFocusOrder="0" pos="68Cc 63R 346 46" posRelativeX="e96b77baef792d3a"
              buttonText="dialog::auth::proceed" connectedEdges="4" needsCallback="1"
              radioGroupId="0"/>
  <LABEL name="" id="9c63b5388edfe183" memberName="emailEditor" virtualName=""
         explicitFocusOrder="0" pos="50.5Cc 36 361 40" posRelativeY="e96b77baef792d3a"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="..."
         editableSingleClick="1" editableDoubleClick="1" focusDiscardsChanges="0"
         fontname="Default serif font" fontsize="37" kerning="0" bold="0"
         italic="0" justification="33"/>
  <LABEL name="" id="cf32360d33639f7f" memberName="emailLabel" virtualName=""
         explicitFocusOrder="0" pos="-186Cc 28 96 22" posRelativeY="e96b77baef792d3a"
         textCol="77ffffff" edTextCol="ff000000" edBkgCol="0" labelText="dialog::auth::email"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default serif font" fontsize="21" kerning="0" bold="0"
         italic="0" justification="34"/>
  <LABEL name="" id="c134a00c2bb2de66" memberName="passwordLabel" virtualName=""
         explicitFocusOrder="0" pos="-186Cc 88 96 22" posRelativeY="e96b77baef792d3a"
         textCol="77ffffff" edTextCol="ff000000" edBkgCol="0" labelText="dialog::auth::password"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default serif font" fontsize="21" kerning="0" bold="0"
         italic="0" justification="34"/>
  <JUCERCOMP name="" id="ab3649d51aa02a67" memberName="shadow" virtualName=""
             explicitFocusOrder="0" pos="0Cc 66R 480 24" posRelativeX="e96b77baef792d3a"
             sourceFile="../Themes/ShadowDownwards.cpp" constructorParams=""/>
  <TEXTBUTTON name="" id="27c5d30533a1f7a9" memberName="cancelButton" virtualName=""
              explicitFocusOrder="0" pos="-181Cc 63R 120 46" posRelativeX="e96b77baef792d3a"
              buttonText="dialog::auth::cancel" connectedEdges="4" needsCallback="1"
              radioGroupId="0"/>
  <GENERICCOMPONENT name="" id="ac81a17122003703" memberName="passwordEditor" virtualName=""
                    explicitFocusOrder="0" pos="50.5Cc 96 361 40" class="LabelWithPassword"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
