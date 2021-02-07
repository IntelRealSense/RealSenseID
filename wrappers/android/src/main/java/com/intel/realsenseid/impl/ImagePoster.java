package com.intel.realsenseid.impl;

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

    private Bitmap mImage;
    private TextureView mTexture;
    private Canvas mCanvas;
    private Matrix mScaleMatrix;

    ImagePoster(Bitmap image, TextureView texture) {
        mImage = image;
        mTexture = texture;
    }

    public void run() {
        if (mTexture != null && mImage != null) {
            mCanvas = mTexture.lockCanvas();
            try {
                RectF bitmapRect = new RectF(0.0F, 0.0F, (float) mImage.getWidth(), (float) mImage.getHeight());

                if (mCanvas != null) {
                    RectF canvasRect = new RectF(-(float) mCanvas.getWidth(), 0.0F, 0.0F, (float) mCanvas.getHeight());

                    mScaleMatrix = new Matrix();
                    mScaleMatrix.setRectToRect(bitmapRect, canvasRect, Matrix.ScaleToFit.CENTER);

                    mCanvas.scale(SCALE_X, SCALE_Y);
                    mCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
                    mCanvas.drawBitmap(mImage, mScaleMatrix, null);
                }

            } catch (Exception e) {
                Log.e(TAG, "Failed to draw bitmap!");
                e.printStackTrace();
                Log.e(TAG, "run: message" + e.toString());
            } finally {
                mTexture.unlockCanvasAndPost(mCanvas);
            }
            mImage.recycle();
        }
    }
}
