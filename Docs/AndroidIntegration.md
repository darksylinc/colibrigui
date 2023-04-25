# Android Integration for Editboxes

Android is troublesome because its main input is often through soft keyboards
operated by touch.

To make it worse each manufacturer has its own flavour and may have custom keyboards.

Additionally, supporting features (e.g. copy/paste gestures, voice to text, emojis) while
also being Unicode friendly can be a monumental task.

Although [it is possible to process keyboard events directly](https://stackoverflow.com/questions/21124051/receive-complete-android-unicode-input-in-c-c), a much safer solution is to create a Native UI widget and let it handle everything for us.

We just then route the text to Colibri

For that you'll need:

 - The file under src/Android/com/colibri/TextInputActivity.kt
 - Extend NativeActivity with your own, e.g. `MyExampleNativeActivity`
    - Don't forget to update your AndroidManifest.xml to point to `MyExampleNativeActivity` instead of `NativeActivity`!
 - The cpp changes below

## Kotlin code to extend NativeActivity

```kotlin
package com.myexample.core

import android.app.NativeActivity
import android.content.DialogInterface
import android.os.Bundle
import android.view.View

class MyExampleNativeActivity : NativeActivity() {
    private external fun jniColibriReceiveText(
        text: String, userData: Long,
    )

    fun init(libraryName: String) {
        System.loadLibrary(libraryName)
    }

    fun showTextInput(currentText: String, hint: String, colibriInputType: Int, userData: Long) {
        runOnUiThread {

            val progressDialog = com.colibri.TextInputDialog(this,
                currentText,
                hint,
                com.colibri.ColibriInputType.values()[colibriInputType])

            val listener: DialogInterface.OnDismissListener =
                DialogInterface.OnDismissListener { dialog ->
                    val textInputDialog = dialog as com.colibri.TextInputDialog
                    jniColibriReceiveText(textInputDialog.editText.text.toString(), userData)
                }

            progressDialog.setOnDismissListener(listener)
            progressDialog.show()
        }
    }
}
```

## C++ code header

```cpp
// Header
struct NativeColibriInputResponse
{
	bool                   bIsNativeTextInputUp;
	bool                   bResponseReceived;  // GUARDED_BY(mutex)
	std::string            responseText;       // GUARDED_BY(mutex)
	Ogre::LightweightMutex mutex;

	NativeColibriInputResponse() : bIsNativeTextInputUp( false ), bResponseReceived( false ) {}
};

class GraphicsSystem
{
	// ...
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
	NativeColibriInputResponse m_nativeColibriInputResponse;
#endif
};
```

## C++ code source

```cpp
// Source
void Android_showTextInput( Colibri::Editbox *editbox,
							NativeColibriInputResponse *nativeColibriInputResponse );

class ColibriListener final : public Colibri::ColibriListener
{
public:
	// ...

	NativeColibriInputResponse *m_nativeColibriInputResponse;
	void showTextInput( Colibri::Editbox *editbox ) override
	{
		// Note: The editbox can safely become dangling pointer after Android_showTextInput returns.
		Android_showTextInput( editbox, m_nativeColibriInputResponse );
	}
}

void GraphicsSystem::initialize()
{
	// ...

	// If we don't call MyExampleNativeActivity.init( libraryName )
	// we'll later get a crash that
	// Java_com_myexample_core_MyExampleNativeActivity_jniColibriReceiveText
	// couldn't be found
	ANativeActivity *nativeActivity = AndroidSystems::getNativeActivity();
	jclass classDef = g_jni->GetObjectClass( nativeActivity->clazz );
	/// !!!
	// IMPORTANT: REPLACE "LIBRARY_NAME" WITH THE NAME OF YOUR LIBRARY CREATED BY CMAKE
	// !!!
	jstring libNameJStr = g_jni->NewStringUTF( "LIBRARY_NAME" );
	jmethodID retryRefreshPurchasesMethod =
		g_jni->GetMethodID( classDef, "init", "(Ljava/lang/String;)V" );
	g_jni->CallVoidMethod( nativeActivity->clazz, retryRefreshPurchasesMethod,
							libNameJStr );
	g_jni->DeleteLocalRef( libNameJStr );
	g_jni->DeleteLocalRef( classDef );
}

void GraphicsSystem::update()
{
	// ...
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
	if( m_nativeColibriInputResponse.bIsNativeTextInputUp )
	{
		// Check input coming from Android / iOS
		Ogre::ScopedLock lock( m_nativeColibriInputResponse.mutex );
		if( m_nativeColibriInputResponse.bResponseReceived )
		{
			// This MUST be called before m_colibriManager->update
			m_colibriManager->setTextInput( m_nativeColibriInputResponse.responseText.c_str(), true );
			m_nativeColibriInputResponse.responseText.clear();
			m_nativeColibriInputResponse.bResponseReceived = false;
			m_nativeColibriInputResponse.bIsNativeTextInputUp = false;
		}
	}
#endif

	// Must be called AFTER setTextInput
	m_colibriManager->update( timeSinceLast );
}

void Android_showTextInput( Colibri::Editbox *editbox,
							NativeColibriInputResponse *nativeColibriInputResponse )
{
	if( nativeColibriInputResponse->bIsNativeTextInputUp )
		return;

	nativeColibriInputResponse->bIsNativeTextInputUp = true;

	ANativeActivity *nativeActivity = AndroidSystems::getNativeActivity();

	const Colibri::InputType::InputType inputType = editbox->getInputType();

	jstring currentTextJStr = g_jni->NewStringUTF( editbox->getText().c_str() );
	jstring textHintJStr = g_jni->NewStringUTF( editbox->getTextHint().c_str() );

	const uint64_t thisAsU64 = reinterpret_cast<uint64_t>( nativeColibriInputResponse );
	static_assert( sizeof( uint64_t ) == sizeof( jlong ), "jlong must be at least 64 bits" );
	jlong thisAsI64;
	memcpy( &thisAsI64, &thisAsU64, sizeof( thisAsI64 ) );

	jclass classDef = g_jni->GetObjectClass( nativeActivity->clazz );
	jmethodID showTextInputMethod =
		g_jni->GetMethodID( classDef, "showTextInput", "(Ljava/lang/String;Ljava/lang/String;IJ)V" );
	g_jni->CallVoidMethod( nativeActivity->clazz, showTextInputMethod, currentTextJStr, textHintJStr,
						   static_cast<jint>( inputType ), thisAsI64 );
	g_jni->DeleteLocalRef( textHintJStr );
	g_jni->DeleteLocalRef( currentTextJStr );
	g_jni->DeleteLocalRef( classDef );
}

extern "C" {

JNIEXPORT void JNICALL Java_com_myexample_core_MyExampleNativeActivity_jniColibriReceiveText(
	JNIEnv *env, jobject /*classLoaderObj*/, jstring textInput, jlong inputResponsePtr )
{
	NativeColibriInputResponse *inputResponse =
		reinterpret_cast<NativeColibriInputResponse *>( inputResponsePtr );

	const char *extraMsgCStr = env->GetStringUTFChars( textInput, NULL );
	{
		Ogre::ScopedLock lock( inputResponse->mutex );
		inputResponse->bResponseReceived = true;
		if( extraMsgCStr )
			inputResponse->responseText = extraMsgCStr;
	}
	env->ReleaseStringUTFChars( textInput, extraMsgCStr );
}
}
```