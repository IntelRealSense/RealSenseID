package com.intel.realsenseid.impl;

import androidx.annotation.Nullable;

public class UsbCdcDeviceInformation {
    private final int deviceVendorId;
    private final int productId;
    @Nullable
    private final String productName;

    public UsbCdcDeviceInformation(int deviceVendorId, int productId, @Nullable String productName) {
        this.deviceVendorId = deviceVendorId;
        this.productId = productId;
        this.productName = productName;
    }

    @Nullable
    public String getProductName() {
        return productName;
    }

    public int getProductId() {
        return productId;
    }

    public int getDeviceVendorId() {
        return deviceVendorId;
    }


    public static class Builder {
        private int deviceVendorId = -1;
        private int productId = -1;
        private String productName = null;

        public Builder setDeviceVendorId(int deviceVendorId) {
            this.deviceVendorId = deviceVendorId;
            return this;
        }

        public Builder setProductId(int productId) {
            this.productId = productId;
            return this;
        }

        public Builder setProductName(String productName) {
            this.productName = productName;
            return this;
        }

        public UsbCdcDeviceInformation build() {
            return new UsbCdcDeviceInformation(deviceVendorId, productId, productName);
        }
    }
}
