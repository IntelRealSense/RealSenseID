package com.intel.realsenseid.f450androidexample;


public class FileModel {
    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public FileType getFileType() {
        return fileType;
    }

    public void setFileType(FileType fileType) {
        this.fileType = fileType;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Double getSizeInMB() {
        return sizeInMB;
    }

    public void setSizeInMB(Double sizeInMB) {
        this.sizeInMB = sizeInMB;
    }

    public String getExtension() {
        return extension;
    }

    public void setExtension(String extension) {
        this.extension = extension;
    }

    public int getSubFiles() {
        return subFiles;
    }

    public void setSubFiles(int subFiles) {
        this.subFiles = subFiles;
    }

    private String path;
    private FileType fileType;
    private String name;
    private Double sizeInMB;
    private String extension;
    private int subFiles;
}