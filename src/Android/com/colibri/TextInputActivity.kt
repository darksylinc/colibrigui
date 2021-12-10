// Copyright 2021-present Matias N. Goldberg
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.

package com.colibri

import android.app.Activity
import android.app.Dialog
import android.view.WindowManager
import android.widget.EditText
import android.widget.LinearLayout




enum class ColibriInputType {
    Text,
    Multiline,
    Password,
    Email
}

val ColibriToAndroidMappings = intArrayOf(
    // Text
    android.text.InputType.TYPE_CLASS_TEXT,
    // Multiline
    android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE,
    // Password
    android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD,
    // Email
    android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS
)

class TextInputDialog(context: Activity, hintText: String, inputType: ColibriInputType) :
    Dialog(context) {

    val editText : EditText

    init {
        val layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT)

        this.requestWindowFeature(android.view.Window.FEATURE_NO_TITLE);

        val editText = EditText(context)
        editText.textSize = 24f
        editText.layoutParams = layoutParams
        editText.setTextIsSelectable(true)
        editText.hint = "Enter " + hintText + ":"
        editText.inputType = ColibriToAndroidMappings[inputType.ordinal]

        this.editText = editText

        setContentView(editText)

        val lp = WindowManager.LayoutParams()
        lp.copyFrom(this.window?.attributes)
        lp.width = context.resources.displayMetrics.widthPixels * 7 / 8

        this.window?.attributes = lp
    }
}