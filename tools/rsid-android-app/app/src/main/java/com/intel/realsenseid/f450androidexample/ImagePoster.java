package com.intel.realsenseid.f450androidexample;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.RectF;
import android.util.Log;
import android.view.TextureView;


class ImagePoster implements Runnable {

    private static final String TAG = "ImagePoster";

    private static final Paint LANDMARKS_PAINT;
    private static final Paint DOT_PAINT;
    private static final float SCALE_X;
    private static final float SCALE_Y;

    static {
        SCALE_X = -1;
        SCALE_Y = 1;

        LANDMARKS_PAINT = new Paint();
        LANDMARKS_PAINT.setColor(Color.RED);

        DOT_PAINT = new Paint();
        DOT_PAINT.setColor(Color.YELLOW);
    }

    private Bitmap m_image;
    private TextureView m_texture;
    private Canvas m_canvas;
    private Matrix m_scaleMatrix;

    ImagePoster(Bitmap image, TextureView texture) {
        m_image = image;
        m_texture = texture;
    }

    public void run() {
        if (m_texture != null && m_image != null) {
            m_canvas = m_texture.lockCanvas();
            try {
                RectF bitmap_rect = new RectF(0.0F, 0.0F, (float) m_image.getWidth(), (float) m_image.getHeight());
                if (m_canvas != null) {
                    RectF canvas_rect = new RectF(-(float) m_canvas.getWidth(), 0.0F, 0.0F, (float) m_canvas.getHeight());
                    m_scaleMatrix = new Matrix();
                    m_scaleMatrix.setRectToRect(bitmap_rect, canvas_rect, Matrix.ScaleToFit.CENTER);
                    m_canvas.scale(SCALE_X, SCALE_Y);
                    m_canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
                    m_canvas.drawBitmap(m_image, m_scaleMatrix, null);
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to draw bitmap!");
                e.printStackTrace();
                Log.e(TAG, "run: message" + e.toString());
            } finally {
                m_texture.unlockCanvasAndPost(m_canvas);
            }
            m_image.recycle();
        }
    }
}
