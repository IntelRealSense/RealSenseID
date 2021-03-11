package com.intel.realsenseid.f450androidexample;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

public class FilesRecyclerAdapter extends RecyclerView.Adapter<FilesRecyclerAdapter.ViewHolder> {
    private final FilesListFragment.OnViewItemClickListener mCallback;
    // TODO: implement in the later.
        /*var onItemClickListener: ((FileModel) -> Unit)? = null
        var onItemLongClickListener: ((FileModel) -> Unit)? = null*/
    List<FileModel> filesList = new ArrayList<>();

    public FilesRecyclerAdapter(FilesListFragment.OnViewItemClickListener callback) {
        mCallback= callback;
    }


    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_recycler_file, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        holder.bindView(position);
    }

    @Override
    public int getItemCount() {
        return filesList.size();
    }

    public void updateData(List<FileModel> filesList) {
        this.filesList = filesList;
        notifyDataSetChanged();
    }

    class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener, View.OnLongClickListener {

        public ViewHolder(@NonNull View itemView) {
            super(itemView);
            itemView.setOnClickListener(this);
            itemView.setOnLongClickListener(this);
        }

        @Override
        public void onClick(View view) {
            mCallback.onClick(filesList.get(getAdapterPosition()));
        }

        @Override
        public boolean onLongClick(View view) {
            mCallback.onLongClick(filesList.get(getAdapterPosition()));
            return true;
        }

        public void bindView(int position) {
            FileModel fileModel = filesList.get(position);
            getViewById(R.id.nameTextView).setText(fileModel.getName());

            if (fileModel.getFileType() == FileType.FOLDER) {
                getViewById(R.id.folderTextView).setVisibility(View.VISIBLE);
                getViewById(R.id.totalSizeTextView).setVisibility(View.GONE);
                getViewById(R.id.folderTextView).setText(String.format("%d files", fileModel.getSubFiles()));
            } else {
                getViewById(R.id.folderTextView).setVisibility(View.GONE);
                getViewById(R.id.totalSizeTextView).setVisibility(View.VISIBLE);
                getViewById(R.id.totalSizeTextView).setText(String.format("%.2f MB", fileModel.getSizeInMB()));
            }
        }

        private TextView getViewById(int id) {
            return (TextView) itemView.findViewById(id);
        }
    }



}