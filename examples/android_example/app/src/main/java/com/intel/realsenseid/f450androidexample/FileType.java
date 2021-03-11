package com.intel.realsenseid.f450androidexample;

import java.io.File;

public enum FileType {
    FILE,
    FOLDER;

    public static FileType getType(File file) {
        if (file.isDirectory())
            return FOLDER;
        return FILE;
    }
}
