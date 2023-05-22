
#import "ColibriGui/iOS/Colibri_iOS.h"

#include "ColibriGui/ColibriEditbox.h"
#include "ColibriGui/ColibriManager.h"

#import <UIKit/UIApplication.h>
#import <UIKit/UIButton.h>
#import <UIKit/UITextField.h>

using namespace Colibri;

COLIBRI_ASSUME_NONNULL_BEGIN

@class UiKeyboardHelper;

UiKeyboardHelper *g_textViewHelper = 0;

@interface UiKeyboardHelper : NSObject <UITextFieldDelegate>
{
	UITextField *m_textField;
	ColibriManager *m_manager;
}
@end

@implementation UiKeyboardHelper

- (void)show:(UIView *)uiView editbox:(Editbox *)editbox
{
	m_manager = editbox->getManager();

	CGRect viewFrame = uiView.frame;

	UIButton *exitButton = [[UIButton alloc] initWithFrame:viewFrame];

	exitButton.opaque = NO;
	exitButton.backgroundColor = [[UIColor clearColor] colorWithAlphaComponent:0.25];

	m_textField = [[UITextField alloc] initWithFrame:viewFrame];
	m_textField.delegate = self;

	[exitButton addTarget:self
				   action:@selector( onExitButtonTouched: )
		 forControlEvents:UIControlEventPrimaryActionTriggered];

	[m_textField addTarget:self
					action:@selector( onTextFieldEditingDidEndAction: )
		  forControlEvents:UIControlEventEditingDidEnd];
	[m_textField addTarget:self
					action:@selector( onTextFieldEditingDidEndAction: )
		  forControlEvents:UIControlEventEditingDidEndOnExit];
	[m_textField addTarget:self
					action:@selector( onTextFieldEditingChanged: )
		  forControlEvents:UIControlEventEditingChanged];

	m_textField.text = [NSString stringWithUTF8String:editbox->getText().c_str()];
	m_textField.placeholder = [NSString stringWithUTF8String:editbox->getTextHint().c_str()];

	if( @available( iOS 13, * ) )
	{
		// Consistent with supported theme
		m_textField.textColor = UIColor.labelColor;
		m_textField.backgroundColor = UIColor.secondarySystemBackgroundColor;
	}
	else
	{
		// Consistent with default keyboard look
		m_textField.textColor = [UIColor blackColor];
		[m_textField setBackgroundColor:[UIColor whiteColor]];
	}

	const Colibri::InputType::InputType inputType = editbox->getInputType();

	switch( inputType )
	{
	case InputType::Text:
		break;
	case InputType::Password:
		m_textField.secureTextEntry = YES;
		if( @available( iOS 11.0, * ) )
		{
			m_textField.textContentType = UITextContentTypePassword;
		}
		break;
	case InputType::Multiline:
		// Unsupported
		break;
	case InputType::Email:
		m_textField.keyboardType = UIKeyboardTypeEmailAddress;
		if( @available( iOS 10.0, * ) )
		{
			m_textField.textContentType = UITextContentTypeEmailAddress;
		}
		break;
	}

	// Set size & position
	[m_textField sizeToFit];
	CGRect frame = m_textField.frame;
	frame.origin.x = 0.0f;
	frame.origin.y = viewFrame.size.height / 4.0f - frame.size.height * 0.5f;
	frame.size.width = viewFrame.size.width;
	frame.size.height *= 2.5f;
	m_textField.opaque = YES;

	[exitButton addSubview:m_textField];
	[uiView addSubview:exitButton];
	m_textField.frame = frame;
	[m_textField becomeFirstResponder];
}

- (void)applyAndClose
{
	if( m_textField )
	{
		m_manager->setTextInput( m_textField.text.UTF8String, true );
		[m_textField.superview removeFromSuperview];  // Remove the button from main view
		[m_textField removeFromSuperview];            // Remove text field from button
		m_textField = 0;
	}
}

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
	if( m_textField == textField )
		m_manager->setTextInput( m_textField.text.UTF8String, true );
	return YES;
}

- (IBAction)onTextFieldEditingDidEndAction:(UITextField *)sender
{
	[self textFieldShouldEndEditing:sender];
}

- (BOOL)textFieldShouldEndEditing:(UITextField *)textField
{
	if( m_textField == textField )
		[self applyAndClose];
	return YES;
}

- (IBAction)onTextFieldEditingChanged:(UITextField *)textField
{
	if( m_textField == textField )
		m_manager->setTextInput( m_textField.text.UTF8String, true );
}

- (IBAction)onExitButtonTouched:(UIButton *)button
{
	if( m_textField.superview == button )
		[self applyAndClose];
}

- (BOOL)textFieldShouldClear:(UITextField *)textField
{
	if( m_textField == textField )
		m_manager->setTextInput( "", true );
	return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	if( m_textField == textField )
		[self applyAndClose];
	return YES;
}
@end

namespace Colibri
{
	void iOS_showTextInput( const void *_uiView, Editbox *editbox )
	{
		if( g_textViewHelper )
			[g_textViewHelper applyAndClose];

		UIView *uiView = CFBridgingRelease( _uiView );

		if( !g_textViewHelper )
			g_textViewHelper = [[UiKeyboardHelper alloc] init];

		[g_textViewHelper show:uiView editbox:editbox];
	}
}

COLIBRI_ASSUME_NONNULL_END
