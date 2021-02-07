package com.intel.realsenseid.impl;

import android.graphics.Bitmap;
import android.view.TextureView;

import com.intel.realsenseid.api.Image;
import com.intel.realsenseid.api.PreviewImageReadyCallback;

import java.nio.ByteBuffer;

public class AndroidPreviewImageReadyCallback extends PreviewImageReadyCallback {
    private static final String LOG_TAG = "AndroidPreviewImageReadyCallback";

    TextureView previewTxv;
    byte[] m_previewBuffer;

    public AndroidPreviewImageReadyCallback(TextureView previewTxv)
    {
        this.previewTxv = previewTxv;
    }

    public void OnPreviewImageReady(Image image){
        if (m_previewBuffer == null) {
            m_previewBuffer = new byte[(int)(image.getWidth()*image.getHeight()*4)];
        }
        image.GetImageBuffer(m_previewBuffer);
        renderImage(m_previewBuffer);
    }

    void renderImage(byte[] rgbBuffer)
    {
        int w = 352;
        int h = 640;

        Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        ByteBuffer buffer = ByteBuffer.wrap(rgbBuffer);
        bmp.copyPixelsFromBuffer(buffer);
        ImageRenderer.render(bmp, previewTxv);
    }

}
