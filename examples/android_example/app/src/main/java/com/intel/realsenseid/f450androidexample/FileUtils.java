package com.intel.realsenseid.f450androidexample;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FileUtils {

    public static List<File> getFilesFromPath(String path) {
        File file = new File(path);
        return Arrays.asList(file.listFiles());
    }

    public static List<FileModel> getFileModelsFromFiles(List<File> files) {
        List<FileModel> outList = new ArrayList<>();
        for (File it : files) {
            FileModel fm = new FileModel(); // TODO: create builder for FileModel and use it instead
            fm.setPath(it.getPath());
            fm.setFileType(FileType.getType(it));
            fm.setName(it.getName());
            fm.setSizeInMB(convertFileSizeToMB(it.length()));
            fm.setExtension(getExtension(it));
            fm.setSubFiles(getNumberOfSubFiles(it));
            outList.add(fm);
        }
        return outList;
    }

    private static int getNumberOfSubFiles(File f) {
        if (!f.isDirectory())
            return 0;
        return f.listFiles().length;
    }

    private static String getExtension(File f) {
        String filename = f.getName();
        int lastDotIndex = filename.lastIndexOf(".");
        if (lastDotIndex < 0)
            return "";
        return filename.substring(lastDotIndex);
    }

    private static Double  convertFileSizeToMB(Long sizeInBytes) {
        return (sizeInBytes.doubleValue()) / (1024 * 1024);
    }
}
