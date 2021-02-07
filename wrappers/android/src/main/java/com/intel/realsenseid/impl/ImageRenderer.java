package com.intel.realsenseid.impl;

import android.graphics.Bitmap;
import android.view.TextureView;

public class ImageRenderer {
    public static void render(Bitmap image, TextureView textureView) {
        new ImagePoster(image, textureView).run();
    }
}