package com.intel.realsenseid.f450androidexample;

import android.graphics.Bitmap;
import android.view.TextureView;

import com.intel.realsenseid.api.Image;
import com.intel.realsenseid.api.PreviewImageReadyCallback;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

public class AndroidPreviewImageReadyCallback extends PreviewImageReadyCallback {
    private static final int BYTES_PER_PIXEL = 4;
    private AtomicBoolean m_flipPreview;
    private TextureView m_previewTxv;
    private byte[] m_previewBuffer;

    public AndroidPreviewImageReadyCallback(TextureView previewTxv)
    {
        this.m_previewBuffer = null;
        this.m_previewTxv = previewTxv;
    }

    public void OnPreviewImageReady(Image image){
        if (m_previewBuffer == null) {
            m_previewBuffer = new byte[(int)(image.getWidth()*image.getHeight()*BYTES_PER_PIXEL)];
        }
        image.GetImageBuffer(m_previewBuffer);
        renderImage(m_previewBuffer, (int)image.getWidth(), (int)image.getHeight());
    }
	
    private void renderImage(byte[] rgbBuffer, int width, int height)
    {
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        ByteBuffer buffer = ByteBuffer.wrap(rgbBuffer);
        bmp.copyPixelsFromBuffer(buffer);
        new ImagePoster(bmp, m_previewTxv).run();
    }   
}
